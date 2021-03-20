//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/CUDADevice.cpp - Released 2020-12-17T15:46:35Z
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

#ifdef __PCL_HAVE_CUDA
#  undef __PCL_HAVE_CUDA
#endif
#ifndef __PCL_COMPATIBILITY
#  ifdef __PCL_LINUX // ### FIXME - Only available on Linux
#    define __PCL_HAVE_CUDA
#  endif
#endif

#ifdef __PCL_HAVE_CUDA
#  include <cuda.h>
#  include <cuda_runtime_api.h>
#endif

#include <pcl/Atomic.h>
#include <pcl/AutoLock.h>
#include <pcl/CUDADevice.h>

#include <pcl/api/APIException.h>
#include <pcl/api/APIInterface.h>

namespace pcl
{

// ----------------------------------------------------------------------------

static ::cuda_device_handle s_deviceHandle = 0;
static AtomicInt s_initialized;
static Mutex s_mutex;

static void EnsureInitialized() noexcept
{
#ifdef __PCL_HAVE_CUDA

   if ( !s_initialized )
   {
      volatile AutoLock lock( s_mutex );
      if ( s_initialized.Load() == 0 )
      {
         if ( (*API->GPU->IsCUDADeviceAvailable)( ModuleHandle() ) )
            s_deviceHandle = (*API->GPU->GetCUDASelectedDevice)( ModuleHandle() );
         s_initialized.Store( 1 );
      }
   }

#endif
}

// ----------------------------------------------------------------------------

bool CUDADevice::IsAvailable() noexcept
{
   EnsureInitialized();
   return s_deviceHandle != 0;
}

// ----------------------------------------------------------------------------

IsoString CUDADevice::Name()
{
#ifdef __PCL_HAVE_CUDA

   EnsureInitialized();
   if ( s_deviceHandle == 0 )
      return IsoString();
   cudaDeviceProp properties;
   if ( (*API->GPU->GetCUDADeviceProperties)( ModuleHandle(), s_deviceHandle, &properties, sizeof( cudaDeviceProp ) ) == api_false )
      throw APIFunctionError( "GetCUDADeviceProperties" );

   return IsoString( properties.name );

#else

   return IsoString();

#endif
}

// ----------------------------------------------------------------------------

size_type CUDADevice::TotalGlobalMemory() noexcept
{
   EnsureInitialized();
   if ( s_deviceHandle == 0 )
      return 0;
   return (*API->GPU->GetCUDADeviceTotalGlobalMem)( ModuleHandle(), s_deviceHandle );
}

// ----------------------------------------------------------------------------

size_type CUDADevice::SharedMemoryPerBlock() noexcept
{
   EnsureInitialized();
   if ( s_deviceHandle == 0 )
      return 0;
   return (*API->GPU->GetCUDADeviceSharedMemoryPerBlock)( ModuleHandle(), s_deviceHandle );
}

// ----------------------------------------------------------------------------

int CUDADevice::MaxThreadsPerBlock() noexcept
{
   EnsureInitialized();
   if ( s_deviceHandle == 0 )
      return 0;
   return (*API->GPU->GetCUDADeviceMaxThreadsPerBlock)( ModuleHandle(), s_deviceHandle );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/CUDADevice.cpp - Released 2020-12-17T15:46:35Z
