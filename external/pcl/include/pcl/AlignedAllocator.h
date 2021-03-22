//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/AlignedAllocator.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_AlignedAllocator_h
#define __PCL_AlignedAllocator_h

/// \file pcl/AlignedAllocator.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/StandardAllocator.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class AlignedAllocator
 * \brief A block allocator class for aligned memory allocation/deallocation.
 *
 * %AlignedAllocator is a <em>block allocator</em> class. It can be used for
 * all PCL container classes instead of the default StandardAllocator class,
 * from which it derives publicly, providing exactly the same memory allocation
 * policies.
 *
 * %AlignedAllocator guarantees that all allocated memory blocks are aligned
 * for optimal performance of SIMD processor instructions on all supported
 * platforms. Currently all blocks are allocated with 32 byte alignment, which
 * is optimal for both SSE and AVX2 load/store instructions.
 *
 * For a complete description of block allocators and their fundamental role in
 * PCL, read the documentation for the Allocator class.
 *
 * \sa Allocator, StandardAllocator
 */
class PCL_CLASS AlignedAllocator : public StandardAllocator
{
public:

   /*!
    * Constructs an %AlignedAllocator object.
    *
    * \param fastGrowth    Whether to enable the fast block size growing policy
    *                      for this allocator.
    *
    * \param canShrink     Whether to enable the block shrinking policy for
    *                      this allocator.
    *
    * See the IsFastGrowthEnabled() and IsShrinkingEnabled() member functions
    * for more information on block allocation policies.
    */
   AlignedAllocator( bool fastGrowth = true, bool canShrink = true )
      : StandardAllocator( fastGrowth, canShrink )
   {
   }

   /*!
    * Copy constructor.
    */
   AlignedAllocator( const AlignedAllocator& ) = default;

   /*!
    * Custom allocation routine. Allocates a contiguous memory block of the
    * specified \a size in bytes with 32-byte alignment, and returns the
    * address of the first byte in the newly allocated block.
    *
    * \note This member function is mandatory for a block allocator to be
    * usable by the Allocator class.
    *
    * \sa DeallocateBlock()
    */
   void* AllocateBlock( size_type size )
   {
      PCL_PRECONDITION( size != 0 )
      void* p = PCL_ALIGNED_MALLOC( size, 32 );
      if ( unlikely( p == nullptr ) )
         throw std::bad_alloc();
      return PCL_ASSUME_ALIGNED_32( p );
   }

   /*!
    * Custom deallocation routine. Deallocates a previously allocated
    * contiguous memory block that begins at the specified location \a p.
    *
    * \note This member function is mandatory for a block allocator to be
    * usable by the Allocator class.
    *
    * \sa AllocateBlock()
    */
   void DeallocateBlock( void* p )
   {
      PCL_PRECONDITION( p != nullptr )
      PCL_ALIGNED_FREE( p );
   }
};

} // pcl

// ----------------------------------------------------------------------------

/*!
 * Placement new operator for class AlignedAllocator. Returns the specified
 * address \a p.
 */
inline void* operator new( pcl::size_type, void* p, pcl::AlignedAllocator& )
{
   PCL_PRECONDITION( p != nullptr )
   return p;
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4100 ) // 'unreferenced formal parameter'

inline void operator delete( void* p, void*, pcl::AlignedAllocator& )
{
   PCL_PRECONDITION( p != nullptr )
}

#pragma warning( pop )
#endif // _MSC_VER

// ----------------------------------------------------------------------------

#endif  // __PCL_AlignedAllocator_h

// ----------------------------------------------------------------------------
// EOF pcl/AlignedAllocator.h - Released 2020-12-17T15:46:29Z
