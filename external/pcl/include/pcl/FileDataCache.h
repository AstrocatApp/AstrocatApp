//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/FileDataCache.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_FileDataCache_h
#define __PCL_FileDataCache_h

/// \file pcl/FileDataCache.h

#include <pcl/File.h>
#include <pcl/MultiVector.h>
#include <pcl/Mutex.h>
#include <pcl/ReferenceSortedArray.h>
#include <pcl/StringList.h>
#include <pcl/TimePoint.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class FileDataCacheItem
 * \brief Element of a file data cache
 *
 * This class represents a file in a FileDataCache object. This is a basic
 * cache item structure to transport a full file path and, the known time of
 * last file modification, and the time of last cache access.
 */
class PCL_CLASS FileDataCacheItem
{
public:

   String    path;     //!< Full path to the file represented by this item.
   TimePoint time;     //!< Cached file time.
   TimePoint lastUsed; //!< Time this cache item was last used.

   /*!
    * Virtual destructor.
    */
   virtual ~FileDataCacheItem()
   {
   }

   /*!
    * Assigns data from another cache \a item.
    */
   void Assign( const FileDataCacheItem& item )
   {
      path     = item.path;
      time     = item.time;
      lastUsed = item.lastUsed;
   }

   /*!
    * Returs true iff this object represents the same file as another cache
    * \a item.
    */
   bool operator ==( const FileDataCacheItem& item ) const
   {
      return path == item.path;
   }

   /*!
    * Returns true iff this object precedes another cache \a item. File cache
    * items are sorted by full file paths in ascending order.
    */
   bool operator <( const FileDataCacheItem& item ) const
   {
      return path < item.path;
   }

   /*!
    * Returns true iff the file represented by this cache item was last
    * modified before the specified file time \a t.
    *
    * \note This member function ignores the milliseconds component of the
    * specified FileTime instance, by setting it to zero. This is done to
    * prevent wrong cache invalidations caused by unreliable file time
    * milliseconds on Windows.
    */
   bool ModifiedSince( FileTime t ) const
   {
      t.milliseconds = 0;
      return time < TimePoint( t );
   }

   /*!
    * Returns the amount of days elapsed since the time this cache item was
    * last used.
    */
   double DaysSinceLastUsed() const
   {
      return TimePoint::Now() - lastUsed;
   }

protected:

   /*!
    * Assigns additional data stored in another file cache item.
    *
    * The default implementation does nothing. This virtual member function
    * should be reimplemented by derived classes to ensure persistence of
    * reimplementation-specific data.
    */
   virtual void AssignData( const FileDataCacheItem& )
   {
   }

   /*!
    * Returns a string representation of additional data stored in this cache
    * item.
    *
    * The default implementation returns an empty string. This virtual member
    * function should be reimplemented by derived classes to allow access to
    * reimplementation-specific data.
    */
   virtual String DataToString() const
   {
      return String();
   }

   /*!
    * Retrieves additional data from a list of string tokens. Returns true iff
    * the data were successfully retrieved.
    *
    * The default implementation returns true. This virtual member function
    * should be reimplemented by derived classes for retrieval of
    * reimplementation-specific data.
    */
   virtual bool GetDataFromTokens( const StringList& )
   {
      return true;
   }

   /*!
    * Returns true iff the additional data stored in this cache item are valid.
    *
    * The default implementation returns true. This virtual member function
    * should be reimplemented by derived classes for validation of
    * reimplementation-specific data.
    */
   virtual bool ValidateData() const
   {
      return true;
   }

   /*!
    * Returns a string serialization of a floating-point vector. The returned
    * string can be deserialized with the GetVector() static member function.
    */
   static String VectorToString( const DVector& );

   /*!
    * Deserializes a floating-point vector from the specified list of
    * \a tokens, parsing the necessary tokens from the specified \a start
    * iterator.
    */
   static bool GetVector( DVector&, StringList::const_iterator& start, const StringList& tokens );

   /*!
    * Returns a string serialization of a floating-point multivector. The
    * returned string can be deserialized with the GetMultiVector() static
    * member function.
    */
   static String MultiVectorToString( const DMultiVector& );

   /*!
    * Deserializes a floating-point vector from the specified list of
    * \a tokens, parsing the necessary tokens from the specified \a start
    * iterator.
    */
   static bool GetMultiVector( DMultiVector&, StringList::const_iterator& start, const StringList& tokens );

   /*
    * Special constructor used for cache search operations.
    */
   FileDataCacheItem( const String& p = String() ) : path( p )
   {
   }

   /*
    * Copy constructor.
    */
   FileDataCacheItem( const FileDataCacheItem& ) = default;

private:

   String ToString() const;
   bool FromString( const String& s );

   bool Load( const IsoString& keyPrefix, int index );
   void Save( const IsoString& keyPrefix, int index ) const;

   friend class FileDataCache;
};

// ----------------------------------------------------------------------------

/*!
 * \class FileDataCache
 * \brief Abstract base class of file data cache implementations.
 *
 * This class provides the necessary infrastructure to implement a file cache
 * with persistent storage in module settings data. The main cache access
 * functions provided by this class (to add, get and find cache items, as well
 * as to clear the cache and query cache properties) are implemented as
 * thread-safe routines. This supports applications performing parallel disk
 * I/O operations.
 *
 * You'll find examples of use for this class in standard PixInsight modules
 * such as ImageIntegration and SubframeSelector.
 *
 * \sa FileDataCacheItem, Settings
 */
class PCL_CLASS FileDataCache
{
public:

   /*!
    * Constructs a new file data cache with the specified settings \a key and
    * maximum cache item duration in \a days.
    *
    * The specified \a key will be used to store all cache data structures
    * associated with this object persistently in module settings data. See the
    * Settings class for more information on module settings and settings keys.
    *
    * \warning If the specified number of \a days is &le; 0, existing cache
    * items will never expire. This is <em>not recommended</em> and can cause
    * problems by increasing the size of stored core application settings
    * indiscriminately. In general, the default maximum duration of 30 days is
    * quite appropriate for most applications.
    */
   FileDataCache( const IsoString& key, int days = 30 );

   /*!
    * Virtual destructor.
    *
    * Destroys and deallocates all file data cache items and internal
    * structures associated with this object. Note that this refers to data
    * currently stored in memory, not to persistent storage in module settings.
    * To destroy data stored persistently, the Purge() member function must be
    * called explicitly.
    */
   virtual ~FileDataCache()
   {
      Clear();
   }

   /*!
    * Returns an identifying name for this cache object. The default
    * implementation returns "File Cache". Derived classes should reimplement
    * this function to return more specific identifiers.
    */
   virtual String CacheName() const
   {
      return "File Cache";
   }

   /*!
    * Returns the current cache version. The default implementation returns 1.
    *
    * \sa MinSupportedVersion()
    */
   virtual int Version() const
   {
      return 1;
   }

   /*!
    * Returns the minimum supported cache version. The default implementation
    * returns 1.
    *
    * No items will be loaded from existing module settings data if their
    * version is either less than the value returned by this function, or
    * greater than the current cache version. This allows for a basic version
    * control system with a range of valid cache versions.
    *
    * \sa Version()
    */
   virtual int MinSupportedVersion() const
   {
      return 1;
   }

   /*!
    * Returns true iff this cache is currently enabled. A disabled cache does
    * not load existing cache items when the Load() member function is invoked.
    */
   bool IsEnabled() const
   {
      return m_enabled;
   }

   /*!
    * Enables this file data cache.
    *
    * Note that enabling a cache does not force a reload of existing cache
    * items; the Load() member function must be called to perform that action.
    * In the same way, disabling a cache does not remove any cache item,
    * neither from existing internal data structures, nor from persistent
    * settings storage.
    */
   void Enable( bool enable )
   {
      m_enabled = enable;
   }

   /*!
    * Returns the maximum duration in days of a valid cache item.
    *
    * Existing cache items that have not been accessed during a period larger
    * than the value returned by this function will not be loaded from
    * persistent settings data.
    *
    * \sa SetDuration(), NeverExpires()
    */
   int Duration() const
   {
      return m_durationDays;
   }

   /*!
    * Sets a new maximum duration in days for valid cache items.
    *
    * \warning If the specified number of \a days is &le; 0, existing cache
    * items will never expire. This is <em>not recommended</em> and can cause
    * problems by increasing the size of stored core application settings
    * indiscriminately. In general, the default maximum duration of 30 days is
    * quite appropriate for most applications.
    *
    * \sa Duration(), NeverExpires()
    */
   void SetDuration( int days )
   {
      m_durationDays = Max( 0, days );
   }

   /*!
    * Returns true iff existing cache items associated with this object will
    * never expire.
    *
    * \sa Duration(), SetDuration()
    */
   bool NeverExpires() const
   {
      return m_durationDays <= 0;
   }

   /*!
    * Returns the total number of cache items associated with this object.
    *
    * The returned value corresponds to the number of cache items currently
    * stored in internal data structures. This includes cache items loaded from
    * existing module settings data as well as items newly created and possibly
    * still not copied to persistent storage.
    *
    * \note This function is thread-safe.
    */
   size_type NumberOfItems() const;

   /*!
    * Returns true iff this cache is empty, i.e. if there are no cache items
    * associated with this object.
    *
    * The returned value is the number of items currently stored in internal
    * memory data structures. This does not necessarily equals the total number
    * of items currently stored in persistent module settings.
    *
    * \note This function is thread-safe.
    */
   bool IsEmpty() const;

   /*!
    * Returns the address of a file cache item corresponding to the specified
    * file \a path, or nullptr if no such cache item could be found.
    *
    * \note This function is thread-safe.
    *
    * \sa Get()
    */
   const FileDataCacheItem* Find( const String& path ) const;

   /*!
    * Destroys and removes all cache items currently associated with this
    * object.
    *
    * Only items stored in internal memory data structures are removed by this
    * function. Persistent storage in module settings data is not altered.
    *
    * \note This function is thread-safe.
    */
   void Clear();

   /*!
    * Adds the specified \a item to this cache.
    *
    * The item will be stored in internal memory data structures, \e not in
    * persistent module settings data. To store cache items persistently, the
    * Save() member function must be called for this object.
    *
    * \note This function is thread-safe.
    */
   void Add( const FileDataCacheItem& item );

   /*!
    * Retrieves a copy of the existing cache data corresponding to the
    * specified file \a path in the specified \a item.
    *
    * Returns true iff a cache item for the specified \a path was found in
    * internal memory data structures, and its data were copied. If false is
    * returned, the specified \a item will not be modified in any way.
    *
    * \note This function is thread-safe.
    *
    * \sa Find()
    */
   bool Get( FileDataCacheItem& item, const String& path );

   /*!
    * Loads existing cache items from persistent module settings data.
    *
    * All previously existing cache items stored in internal memory structures
    * will be destroyed and deallocated before loading new data.
    */
   virtual void Load();

   /*!
    * Writes all cache items associated with this object to persistent module
    * settings data.
    */
   virtual void Save() const;

   /*!
    * Destroys and deallocates all existing cache items, including all items
    * currently in internal memory data structures as well as all items stored
    * in persistent module settings data.
    */
   virtual void Purge() const;

protected:

   /*!
    * Allocates and constructs a new cache item.
    *
    * Returns a pointer to the newly created cache item. The new item will be
    * owned by this object, which will destroy and deallocate it automatically
    * when appropriate.
    *
    * This is a pure virtual member function that must be reimplemented by all
    * derived classes. This is because the data transported by a cache item is
    * application-specific and cannot be known in advance by this class.
    */
   virtual FileDataCacheItem* NewItem() const = 0;

private:

   typedef ReferenceSortedArray<FileDataCacheItem> cache_index;

   mutable Mutex       m_mutex;
           cache_index m_cache;
           IsoString   m_keyPrefix;
           int         m_durationDays = 30; // <= 0 -> never expires
           bool        m_enabled = true;
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_FileDataCache_h

// ----------------------------------------------------------------------------
// EOF pcl/FileDataCache.h - Released 2020-12-17T15:46:29Z
