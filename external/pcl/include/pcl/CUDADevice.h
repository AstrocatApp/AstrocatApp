//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/CUDADevice.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_CUDADevice_h
#define __PCL_CUDADevice_h

/// \file pcl/CUDADevice.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/String.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class CUDADevice
 * \brief Access to core CUDA device services
 *
 * ### TODO: Write description.
 */
class PCL_CLASS CUDADevice
{
public:

   /*!
    * Default constructor - deleted, not an instantiable class.
    */
   CUDADevice() = delete;

   /*!
    * Copy constructor - deleted, not an instantiable class.
    */
   CUDADevice( const CUDADevice& ) = delete;

   /*!
    * Copy assignment operator - deleted, not an instantiable class.
    */
   CUDADevice& operator =( CUDADevice& ) = delete;

   /*!
    * Returns true iff a valid and operational CUDA device is currently
    * available on the running PixInsight platform.
    */
   static bool IsAvailable() noexcept;

   /*!
    * Returns the identifying name of the active CUDA device, or an empty
    * string if there is no valid CUDA device available on the running
    * PixInsight platform.
    */
   static IsoString Name();

   /*!
    * Returns the total global memory available on the active CUDA device in
    * bytes, or zero if no valid CUDA device is available.
    */
   static size_type TotalGlobalMemory() noexcept;

   /*!
    * Returns the total shared memory available per block on the active CUDA
    * device in bytes, or zero if no valid CUDA device is available.
    */
   static size_type SharedMemoryPerBlock() noexcept;

   /*!
    * Returns the maximum number of threads per block available in the active
    * CUDA device, or zero if no valid CUDA device is available.
    */
   static int MaxThreadsPerBlock() noexcept;
};

// ----------------------------------------------------------------------------

} // pcl

#endif  // __PCL_CUDADevice_h

// ----------------------------------------------------------------------------
// EOF pcl/CUDADevice.h - Released 2020-12-17T15:46:29Z
