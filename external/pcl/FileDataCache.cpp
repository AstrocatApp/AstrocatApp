//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/FileDataCache.cpp - Released 2020-12-17T15:46:35Z
// ----------------------------------------------------------------------------
// This file is part of the PixInsight Class Library (PCL).
// PCL is a multiplatform C++ framework for development of PixInsight modules.
//
// Copyright (c) 2003-2020 Pleiades Astrophoto S.L. All Rights Reserved.
//
// Redistribution and use in both source and binary forms, with or without
// modification, is permitted provided that the following conditions are met:
//
// 1. All redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. All redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the names "PixInsight" and "Pleiades Astrophoto", nor the names
//    of their contributors, may be used to endorse or promote products derived
//    from this software without specific prior written permission. For written
//    permission, please contact info@pixinsight.com.
//
// 4. All products derived from this software, in any form whatsoever, must
//    reproduce the following acknowledgment in the end-user documentation
//    and/or other materials provided with the product:
//
//    "This product is based on software from the PixInsight project, developed
//    by Pleiades Astrophoto and its contributors (https://pixinsight.com/)."
//
//    Alternatively, if that is where third-party acknowledgments normally
//    appear, this acknowledgment must be reproduced in the product itself.
//
// THIS SOFTWARE IS PROVIDED BY PLEIADES ASTROPHOTO AND ITS CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PLEIADES ASTROPHOTO OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, BUSINESS
// INTERRUPTION; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; AND LOSS OF USE,
// DATA OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------

#include <pcl/AutoLock.h>
#include <pcl/AutoPointer.h>
#include <pcl/FileDataCache.h>
#include <pcl/FileInfo.h>
#include <pcl/Settings.h>

namespace pcl
{

// ----------------------------------------------------------------------------

FileDataCache::FileDataCache( const IsoString& key, int days )
   : m_keyPrefix( key.Trimmed() )
   , m_durationDays( Max( 0, days ) )
{
   PCL_PRECONDITION( !m_keyPrefix.IsEmpty() )
   if ( m_keyPrefix.IsEmpty() )
      throw Error( "FileDataCache: Invalid key" );
   if ( !m_keyPrefix.StartsWith( '/' ) )
      m_keyPrefix.Prepend( '/' );
   if ( !m_keyPrefix.EndsWith( '/' ) )
      m_keyPrefix.Append( '/' );
}

// ----------------------------------------------------------------------------

size_type FileDataCache::NumberOfItems() const
{
   volatile AutoLock lock( m_mutex );
   return m_cache.Length();
}

// ----------------------------------------------------------------------------

bool FileDataCache::IsEmpty() const
{
   volatile AutoLock lock( m_mutex );
   return m_cache.IsEmpty();
}

// ----------------------------------------------------------------------------

const FileDataCacheItem* FileDataCache::Find( const String& path ) const
{
   volatile AutoLock lock( m_mutex );
   cache_index::const_iterator i = m_cache.Search( FileDataCacheItem( path ) );
   return (i == m_cache.End()) ? nullptr : i;
}

// ----------------------------------------------------------------------------

void FileDataCache::Clear()
{
   volatile AutoLock lock( m_mutex );
   m_cache.Destroy();
}

// ----------------------------------------------------------------------------

void FileDataCache::Add( const FileDataCacheItem& item )
{
   FileInfo info( item.path );
   if ( !info.Exists() || !info.IsFile() )
      throw Error( "FileDataCache::Add(): No such file: " + item.path );

   {
      volatile AutoLock lock( m_mutex );

      cache_index::const_iterator i = m_cache.Search( item );
      if ( i == m_cache.End() )
      {
         FileDataCacheItem* newItem = NewItem();
         newItem->path = item.path;
         m_cache << newItem;
         i = m_cache.Search( newItem );
      }
      FileDataCacheItem& newItem = *m_cache.MutableIterator( i );
      FileTime t = info.LastModified();
      t.milliseconds = 0; // prevent wrong cache invalidations on Windows
      newItem.time = t;
      newItem.lastUsed = TimePoint::Now();
      newItem.AssignData( item );
   }
}

// ----------------------------------------------------------------------------

bool FileDataCache::Get( FileDataCacheItem& item, const String& path )
{
   FileInfo info( path );
   bool badItem = !info.Exists() || !info.IsFile();

   {
      volatile AutoLock lock( m_mutex );

      cache_index::const_iterator i = m_cache.Search( FileDataCacheItem( path ) );
      if ( i != m_cache.End() )
      {
         if ( !badItem )
         {
            item.Assign( *i );
            item.AssignData( *i );
            if ( !item.ModifiedSince( info.LastModified() ) )
               return true;
         }

         m_cache.Destroy( m_cache.MutableIterator( i ) );
      }
   }

   if ( badItem )
      throw Error( "FileDataCache::Get(): No such file: " + path );

   return false;
}

// ----------------------------------------------------------------------------

void FileDataCache::Load()
{
   m_cache.Destroy();

   m_durationDays = 30;
   Settings::Read( m_keyPrefix + "Duration", m_durationDays );
   m_durationDays = Max( 0, m_durationDays );

   m_enabled = true;
   Settings::Read( m_keyPrefix + "Enabled", m_enabled );

   int version = 0;
   Settings::Read( m_keyPrefix + "Version", version );
   if ( version >= MinSupportedVersion() )
      if ( version <= Version() )
         if ( IsEnabled() )
         {
            try
            {
               AutoPointer<FileDataCacheItem> item;
               for ( int i = 0; ; ++i )
               {
                  item = NewItem();
                  if ( !item->Load( m_keyPrefix, i ) )
                     break;

                  if ( m_durationDays > 0 && item->DaysSinceLastUsed() > m_durationDays )
                     item.Destroy();
                  else
                     m_cache << item.Release();
               }
            }
            catch ( ... )
            {
               m_cache.Destroy();
               throw Error( "FileDataCache::Load(): Corrupted cache data." );
            }
         }
}

// ----------------------------------------------------------------------------

void FileDataCache::Save() const
{
   if ( IsEnabled() )
   {
      Purge();
      int index = 0;
      for ( const FileDataCacheItem& item : m_cache )
         item.Save( m_keyPrefix, index++ );
   }

   // ### N.B. Make sure this is done after having called Purge() if necessary.
   Settings::Write( m_keyPrefix + "Version", Version() );
   Settings::Write( m_keyPrefix + "Duration", Duration() );
   Settings::Write( m_keyPrefix + "Enabled", IsEnabled() );
}

// ----------------------------------------------------------------------------

void FileDataCache::Purge() const
{
   IsoString key = m_keyPrefix;
   if ( key.EndsWith( '/' ) )
      key.DeleteRight( key.UpperBound() );
   Settings::Remove( key );
}

// ----------------------------------------------------------------------------

String FileDataCacheItem::VectorToString( const DVector& v )
{
   String s = String().Format( "\n%d", v.Length() );
   for ( int i = 0; i < v.Length(); ++i )
      s.AppendFormat( "\n%.8e", v[i] );
   return s;
}

// ----------------------------------------------------------------------------

bool FileDataCacheItem::GetVector( DVector& v, StringList::const_iterator& i, const StringList& s )
{
   if ( i == s.End() )
      return false;
   int n = i->ToInt();
   if ( n < 0 || s.End() - i <= n )
      return false;
   ++i;
   v = DVector( n );
   for ( int j = 0; j < n; ++j, ++i )
      v[j] = i->ToDouble();
   return true;
}

// ----------------------------------------------------------------------------

String FileDataCacheItem::MultiVectorToString( const DMultiVector& m )
{
   String s = String().Format( "\n%u", m.Length() );
   for ( const DVector& v : m )
      s.Append( VectorToString( v ) );
   return s;
}

// ----------------------------------------------------------------------------

bool FileDataCacheItem::GetMultiVector( DMultiVector& m, StringList::const_iterator& i, const StringList& s )
{
   if ( i == s.End() )
      return false;
   int n = i->ToInt();
   if ( n < 0 || s.End() - i <= n )
      return false;
   ++i;
   for ( int j = 0; j < n; ++j )
   {
      DVector v;
      if ( !GetVector( v, i, s ) )
         return false;
      m << v;
   }
   return true;
}

// ----------------------------------------------------------------------------

String FileDataCacheItem::ToString() const
{
   String s = String()
      << "path\n" << path
      << "\ntime\n" << time.ToString( 3/*timeItems*/, 0/*precision*/, 0/*tz*/, false/*timeZone*/ )
      << "\nlastUsed\n" << lastUsed.ToString( 3/*timeItems*/, 0/*precision*/, 0/*tz*/, false/*timeZone*/ );
   String data = DataToString();
   if ( !data.IsEmpty() )
      s << "\ndata\n" << data;
   return s;
}

// ----------------------------------------------------------------------------

bool FileDataCacheItem::FromString( const String& s )
{
   path.Clear();
   time = lastUsed = TimePoint();

   StringList tokens;
   s.Break( tokens, char16_type( '\n' ) );

   for ( StringList::const_iterator i = tokens.Begin(); i != tokens.End(); )
   {
      if ( *i == "path" )
      {
         if ( ++i == tokens.End() )
            return false;
         path = i->Trimmed();
      }
      else if ( *i == "time" )
      {
         if ( ++i == tokens.End() )
            return false;
         time = TimePoint( *i );
      }
      else if ( *i == "lastUsed" )
      {
         if ( ++i == tokens.End() )
            return false;
         lastUsed = TimePoint( *i );
      }
      else if ( *i == "data" )
      {
         if ( !GetDataFromTokens( tokens ) )
            return false;
      }

      ++i;
   }

   return !path.IsEmpty() && time.IsValid() && lastUsed.IsValid() && time <= lastUsed && ValidateData();
}

// ----------------------------------------------------------------------------

bool FileDataCacheItem::Load( const IsoString& keyPrefix, int index )
{
   String s;
   if ( !Settings::Read( keyPrefix + IsoString().Format( "%08d", index+1 ), s ) )
      return false;
   if ( s.IsEmpty() || !FromString( s ) )
      throw CaughtException();
   return true;
}

// ----------------------------------------------------------------------------

void FileDataCacheItem::Save( const IsoString& keyPrefix, int index ) const
{
   Settings::Write( keyPrefix + IsoString().Format( "%08d", index+1 ), ToString() );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/FileDataCache.cpp - Released 2020-12-17T15:46:35Z
