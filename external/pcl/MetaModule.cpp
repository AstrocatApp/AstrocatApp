//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/MetaModule.cpp - Released 2020-12-17T15:46:35Z
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

#ifdef __PCL_WINDOWS
# include <windows.h>
#else
# include <unistd.h>
#endif

#if defined( __PCL_MACOSX ) || defined( __PCL_FREEBSD )
# include <sys/types.h>
# include <sys/param.h>
# include <sys/sysctl.h>
#endif

#ifdef __PCL_MACOSX
# include <mach/mach_host.h>
# include <mach/mach_port.h>
#endif

#ifdef __PCL_LINUX
# include <pcl/ExternalProcess.h>
# include <pcl/Thread.h>
#endif

#include <pcl/ErrorHandler.h>
#include <pcl/MetaModule.h>
#include <pcl/ProcessInterface.h>
#include <pcl/Version.h>

#include <pcl/api/APIException.h>
#include <pcl/api/APIInterface.h>

namespace pcl
{

// ----------------------------------------------------------------------------

MetaModule* Module = nullptr;

// ----------------------------------------------------------------------------

MetaModule::MetaModule()
   : MetaObject( nullptr )
{
   if ( Module != nullptr )
      throw Error( "MetaModule: Module redefinition not allowed" );
   Module = this;
}

MetaModule::~MetaModule()
{
   if ( this == Module )
      Module = nullptr;
}

/*
 * ### REMOVE - deprecated function
 */
const char* MetaModule::UniqueId() const
{
   return nullptr;
}

bool MetaModule::IsInstalled() const
{
   return API != nullptr;
}

void MetaModule::ProcessEvents( bool excludeUserInputEvents )
{
   thread_handle thread = (*API->Thread->GetCurrentThread)();
   if ( thread == 0 ) // if root thread
      (*API->Global->ProcessEvents)( excludeUserInputEvents );
   else
   {
      uint32 threadStatus = (*API->Thread->GetThreadStatus)( thread );
      if ( threadStatus & 0x80000000 ) // see Thread.cpp
         throw ProcessAborted();
   }
}

// ----------------------------------------------------------------------------

/*
 * Length of the version marker string: "PIXINSIGHT_MODULE_VERSION_".
 *
 * PIXINSIGHT_MODULE_VERSION_
 * 12345678901234567890123456
 *          1         2
 *
 * Note that we cannot define the marker string in source code, or the module
 * authentication routines would detect our instance, instead of the actual
 * version information string. This would prevent module authentication.
 */
#define LengthOfVersionMarker 26

void MetaModule::GetVersion( int& major, int& minor, int& release, int& build,
                             IsoString& language, IsoString& status ) const
{
   // Set undefined states for all variables, in case of error.
   major = minor = release = build = 0;
   language.Clear();
   status.Clear();

   IsoString vs( Version() );

   // A version string must begin with a version marker
   if ( vs.Length() < LengthOfVersionMarker )
      return;

   // Split the string of version numbers into tokens separated by dots
   StringList tokens;
   vs.Break( tokens, '.', false/*trim*/, LengthOfVersionMarker/*startIndex*/ );

   // Required: MM.mm.rr.bbbb.LLL
   // Optional: .<status>
   if ( tokens.Length() < 5 || tokens.Length() > 6 )
      return;

   // Extract version numbers
   try
   {
      int MM   = tokens[0].ToInt( 10 );
      int mm   = tokens[1].ToInt( 10 );
      int rr   = tokens[2].ToInt( 10 );
      int bbbb = tokens[3].ToInt( 10 );

      major = MM;
      minor = mm;
      release = rr;
      build = bbbb;
   }
   catch ( ... ) // silently eat all parse exceptions here
   {
      return;
   }

   // Language code
   language = tokens[4]; // ### TODO: Verify validity of ISO 639.2 code

   // Optional status word
   if ( tokens.Length() == 6 )
      status = tokens[5];  // ### TODO: Verify validity of the status word
}

IsoString MetaModule::ReadableVersion() const
{
   int major, minor, release, build;
   IsoString dum1, dum2;
   GetVersion( major, minor, release, build, dum1, dum2 );
   IsoString version = Name() + IsoString().Format( " module version %d.%d.%d", major, minor, release );
   if ( build > 0 )
      version.AppendFormat( "-%d", build );
   return version;
}

// ----------------------------------------------------------------------------

bool MetaModule::GetPhysicalMemoryStatus( size_type& totalBytes, size_type& availableBytes ) const
{
#ifdef __PCL_FREEBSD

   totalBytes = availableBytes = 0;
   int mib[ 2 ] = { CTL_HW, HW_PHYSMEM };
   size_t size = sizeof( totalBytes );
   if ( sysctl( mib, 2, &totalBytes, &size, 0, 0 ) == 0 )
   {
      mib[1] = HW_USERMEM;
      size = sizeof( availableBytes );
      if ( sysctl( mib, 2, &availableBytes, &size, 0, 0 ) == 0 )
         return totalBytes > 0 && availableBytes > 0;
   }
   return false;

#endif

#ifdef __PCL_LINUX

   ExternalProcess P;
   for ( int try_ = 0;; )
   {
      P.Start( "cat", StringList() << "/proc/meminfo" );
      if ( P.WaitForStarted() )
         if ( P.WaitForFinished() )
            if ( !P.HasCrashed() )
               if ( P.ExitCode() == 0 )
                  break;
      if ( ++try_ == 3 )
         return false;
      Sleep( 500 );
   }

   IsoString info = IsoString( P.StandardOutput() );
   if ( info.IsEmpty() )
      return false;

   unsigned long memTotalkB = 0, memAvailablekB = 0, memFreekB = 0, cachedkB = 0;
   IsoStringList lines;
   info.Break( lines, '\n', true/*trim*/ );
   for ( const IsoString& line : lines )
      if ( line.StartsWithIC( "MemTotal" ) )
      {
         if ( ::sscanf( line.c_str(), "%*s %lu %*s", &memTotalkB ) != 1 )
            return false;
      }
      else if ( line.StartsWithIC( "MemAvailable" ) )
      {
         // Current kernel versions provide a MemAvailable item since 2014:
         // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
         if ( ::sscanf( line.c_str(), "%*s %lu %*s", &memAvailablekB ) != 1 )
            return false;
         if ( memTotalkB > 0 )
            break;
      }
      else if ( line.StartsWithIC( "MemFree" ) )
      {
         if ( ::sscanf( line.c_str(), "%*s %lu %*s", &memFreekB ) != 1 )
            return false;
      }
      else if ( line.StartsWithIC( "Cached" ) )
      {
         if ( ::sscanf( line.c_str(), "%*s %lu %*s", &cachedkB ) != 1 )
            return false;
      }

   totalBytes = memTotalkB * 1024;

   // On old kernels/distros (e.g. RHEL 6.x), try to guess an approximate value
   // as 'free' + 'cached', which is wrong but should be pessimistic i.e. safe.
   if ( memAvailablekB == 0 )
      memAvailablekB = memFreekB + cachedkB;
   availableBytes = memAvailablekB * 1024;

   return totalBytes > 0 && availableBytes > 0;

#endif // __PCL_LINUX

#ifdef __PCL_MACOSX

   totalBytes = 0;
   int mib[ 2 ] = { CTL_HW, HW_PHYSMEM };
   size_t size = sizeof( totalBytes );
   if ( sysctl( mib, 2, &totalBytes, &size, 0, 0 ) == 0 )
   {
      mach_port_t host = mach_host_self();
      kern_return_t kret;
# ifdef HOST_VM_INFO64
      struct vm_statistics64 vm_stat;
      natural_t count = HOST_VM_INFO64_COUNT;
      kret = host_statistics64( host, HOST_VM_INFO64, (host_info64_t)&vm_stat, &count );
# else
      struct vm_statistics	vm_stat;
      natural_t count = HOST_VM_INFO_COUNT;
      kret = host_statistics( host, HOST_VM_INFO, (host_info_t)&vm_stat, &count );
# endif
      if ( kret != KERN_SUCCESS )
         return false;
      availableBytes = size_type( vm_stat.free_count + vm_stat.active_count + vm_stat.inactive_count )
                     * size_type( sysconf( _SC_PAGESIZE ) );
      mach_port_deallocate( mach_task_self(), host );
      return totalBytes > 0 && availableBytes > 0;
   }

   return false;

#endif // __PCL_MACOSX

#ifdef __PCL_WINDOWS

   MEMORYSTATUSEX m;
   m.dwLength = sizeof( m );
   GlobalMemoryStatusEx( &m );
   totalBytes = size_type( m.ullTotalPhys );
   availableBytes = size_type( m.ullAvailPhys );
   return totalBytes > 0 && availableBytes > 0;

#endif
}

// ----------------------------------------------------------------------------

void MetaModule::LoadResource( const String& filePath, const String& rootPath )
{
   if ( (*API->Module->LoadResource)( ModuleHandle(), filePath.c_str(), rootPath.c_str() ) == api_false )
      throw APIFunctionError( "LoadResource" );
}

void MetaModule::UnloadResource( const String& filePath, const String& rootPath )
{
   if ( (*API->Module->UnloadResource)( ModuleHandle(), filePath.c_str(), rootPath.c_str() ) == api_false )
      throw APIFunctionError( "UnloadResource" );
}

// ----------------------------------------------------------------------------

Variant MetaModule::EvaluateScript( const String& sourceCode, const IsoString& language )
{
   api_property_value result;
   if ( (*API->Module->EvaluateScript)( ModuleHandle(), &result, sourceCode.c_str(), language.c_str() ) == api_false )
      throw APIFunctionError( "EvaluateScript" );
   return VariantFromAPIPropertyValue( result );
}

// ----------------------------------------------------------------------------
// Global Context
// ----------------------------------------------------------------------------

class GlobalContextDispatcher
{
public:

   static void api_func OnLoad()
   {
      try
      {
         Module->OnLoad();
      }
      ERROR_HANDLER
   }

   static void api_func OnUnload()
   {
      try
      {
         Module->OnUnload();

         if ( Module != nullptr )
            for ( size_type i = 0; i < Module->Length(); ++i )
               if ( (*Module)[i] != nullptr )
               {
                  const ProcessInterface* iface = dynamic_cast<const ProcessInterface*>( (*Module)[i] );
                  if ( iface != nullptr )
                     if ( iface->LaunchCount() != 0 )
                     {
                        if ( iface->IsAutoSaveGeometryEnabled() )
                           iface->SaveGeometry();
                        iface->SaveSettings();
                     }
               }
      }
      ERROR_HANDLER
   }

   static void* api_func Allocate( size_type sz )
   {
      try
      {
         return Module->Allocate( sz );
      }
      ERROR_HANDLER
      return nullptr;
   }

   static void api_func Deallocate( void* p )
   {
      try
      {
         Module->Deallocate( p );
      }
      ERROR_HANDLER
   }
}; // GlobalContextDispatcher

// ----------------------------------------------------------------------------

void MetaModule::PerformAPIDefinitions() const
{
   (*API->ModuleDefinition->EnterModuleDefinitionContext)();

   (*API->ModuleDefinition->SetModuleOnLoadRoutine)( GlobalContextDispatcher::OnLoad );
   (*API->ModuleDefinition->SetModuleOnUnloadRoutine)( GlobalContextDispatcher::OnUnload );
   (*API->ModuleDefinition->SetModuleAllocationRoutine)( GlobalContextDispatcher::Allocate );
   (*API->ModuleDefinition->SetModuleDeallocationRoutine)( GlobalContextDispatcher::Deallocate );

   /*
    * Meta object Definitions
    */
   for ( size_type i = 0; i < Module->Length(); ++i )
   {
      const MetaObject* o = (*Module)[i];
      if ( o != nullptr )
         o->PerformAPIDefinitions();
   }

   (*API->ModuleDefinition->ExitModuleDefinitionContext)();
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/MetaModule.cpp - Released 2020-12-17T15:46:35Z
