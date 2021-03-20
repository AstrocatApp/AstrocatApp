//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/Median.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/Math.h>
#include <pcl/ReferenceArray.h>
#include <pcl/Thread.h>
#include <pcl/Vector.h>

#ifdef MEAN
# undef MEAN
#endif
#define MEAN( a, b ) \
   (double( a ) + double( b ))/2

#ifdef CMPXCHG
# undef CMPXCHG
#endif
#define CMPXCHG( a, b ) \
   if ( t[b] < t[a] ) Swap( t[a], t[b] )

namespace pcl
{

// ----------------------------------------------------------------------------

template <typename T>
class PCL_HistogramThread : public Thread
{
public:

   SzVector H;

   PCL_HistogramThread( const T* A, size_type start, size_type stop, const double& low, const double& high )
      : H( __PCL_MEDIAN_HISTOGRAM_LENGTH )
      , m_A( A )
      , m_start( start )
      , m_stop( stop )
      , m_low( low )
      , m_high( high )
   {

   }

   PCL_HOT_FUNCTION void Run() override
   {
      H = size_type( 0 );
      double range = m_high - m_low;
      const T* __restrict__ a = m_A + m_start;
      const T* __restrict__ b = m_A + m_stop;
      do
      {
         if ( *a >= m_low )
            if ( *a <= m_high )
               ++H[int( (__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) * (*a - m_low)/range )];
      }
      while ( ++a < b );
   }

private:

   const T*      m_A;
   size_type     m_start;
   size_type     m_stop;
   const double& m_low;
   const double& m_high;
};

template <typename T>
class PCL_MinMaxThread : public Thread
{
public:

   T min;
   T max;

   PCL_MinMaxThread( const T* A, size_type start, size_type stop )
      : m_A( A )
      , m_start( start )
      , m_stop( stop )
   {
   }

   PCL_HOT_FUNCTION void Run() override
   {
      const T* __restrict__ a = m_A + m_start;
      const T* __restrict__ b = m_A + m_stop;
      min = max = *a;
      while ( ++a < b )
         if ( *a < min )
            min = *a;
         else if ( *a > max )
            max = *a;
   }

private:

   const T*  m_A;
   size_type m_start;
   size_type m_stop;
};

// ----------------------------------------------------------------------------

template <typename T>
static double PCL_FastMedian( const T* __restrict__ begin, const T* __restrict__ end )
{
   distance_type N = end - begin;
   if ( N <= 2560000 )
   {
      Array<T> A( begin, end );
      double m = double( *pcl::Select( A.Begin(), A.End(), N >> 1 ) );
      if ( N & 1 )
         return m;
      return (m + double( *pcl::Select( A.Begin(), A.End(), (N >> 1)-1 ) ))/2;
   }

   Array<size_type> L = Thread::OptimalThreadLoads( N, 160*1024/*overheadLimit*/ );

   double low, high;
   {
      ReferenceArray<PCL_MinMaxThread<T>> threads;
      for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
         threads << new PCL_MinMaxThread<T>( begin, n, n + L[i] );

      if ( threads.Length() > 1 )
      {
         int i = 0;
         for ( auto& thread : threads )
            thread.Start( ThreadPriority::DefaultMax, i++ );
         for ( auto& thread : threads )
            thread.Wait();
      }
      else
         threads[0].Run();

      T tlow = threads[0].min;
      T thigh = threads[0].max;
      for ( size_type i = 1; i < threads.Length(); ++i )
      {
         if ( threads[i].min < tlow )
            tlow = threads[i].min;
         if ( threads[i].max > thigh )
            thigh = threads[i].max;
      }

      low = double( tlow );
      high = double( thigh );

      threads.Destroy();
   }

   const double eps = std::is_floating_point<T>::value ?
                        2*std::numeric_limits<T>::epsilon() : 0.5/Pow2( double( sizeof( T ) << 3 ) );
   if ( high - low < eps )
      return low;

   ReferenceArray<PCL_HistogramThread<T>> threads;
   for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
      threads << new PCL_HistogramThread<T>( begin, n, n + L[i], low, high );

   /*
    * High median, extremes and initial histogram saved for even length.
    */
   double mh = 0, l0 = low, h0 = high;
   SzVector H0;

   for ( size_type count = 0, n2 = N >> 1, step = 0, it = 0;; ++it )
   {
      SzVector H;
      if ( it == 0 && step )
         H = H0;
      else
      {
         if ( threads.Length() > 1 )
         {
            int i = 0;
            for ( auto& thread : threads )
               thread.Start( ThreadPriority::DefaultMax, i++ );
            for ( auto& thread : threads )
               thread.Wait();
         }
         else
            threads[0].Run();

         H = threads[0].H;
         for ( size_type i = 1; i < threads.Length(); ++i )
            H += threads[i].H;
         if ( it == 0 )
            if ( (N & 1) == 0 )
               H0 = H;
      }

      for ( int i = 0; ; count += H[i++] )
         if ( count + H[i] > n2 )
         {
            double range = high - low;
            high = (range * (i + 1))/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + low;
            low = (range * i)/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + low;
            if ( high - low < eps )
            {
               if ( N & 1 )
               {
                  threads.Destroy();
                  return low;
               }
               if ( step )
               {
                  threads.Destroy();
                  return (low + mh)/2;
               }
               mh = low;
               low = l0;
               high = h0;
               count = 0;
               --n2;
               ++step;
               it = 0;
            }
            break;
         }
   }
}

// ----------------------------------------------------------------------------

template <typename T>
static double PCL_SmallMedian( T* t, distance_type n )
{
   PCL_PRECONDITION( n <= 32 )
   switch ( n )
   {
   case  1:
      return double( *t );
   case  2:
      return MEAN( t[0], t[1] );
   case  3:
      CMPXCHG( 0, 1 ); CMPXCHG( 1, 2 );
      return double( pcl::Max( t[0], t[1] ) );
   case  4:
      CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 ); CMPXCHG( 0, 2 );
      CMPXCHG( 1, 3 );
      return MEAN( t[1], t[2] );
   case  5:
      CMPXCHG( 0, 1 ); CMPXCHG( 3, 4 ); CMPXCHG( 0, 3 );
      CMPXCHG( 1, 4 ); CMPXCHG( 1, 2 ); CMPXCHG( 2, 3 );
      return double( pcl::Max( t[1], t[2] ) );
   case  6:
      CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 ); CMPXCHG( 0, 2 );
      CMPXCHG( 1, 3 ); CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 );
      CMPXCHG( 0, 4 ); CMPXCHG( 1, 5 ); CMPXCHG( 1, 4 );
      CMPXCHG( 2, 4 ); CMPXCHG( 3, 5 ); CMPXCHG( 3, 4 );
      return MEAN( t[2], t[3] );
   case  7:
      CMPXCHG( 0, 5 ); CMPXCHG( 0, 3 ); CMPXCHG( 1, 6 );
      CMPXCHG( 2, 4 ); CMPXCHG( 0, 1 ); CMPXCHG( 3, 5 );
      CMPXCHG( 2, 6 ); CMPXCHG( 2, 3 ); CMPXCHG( 3, 6 );
      CMPXCHG( 4, 5 ); CMPXCHG( 1, 4 ); CMPXCHG( 1, 3 );
      return double( pcl::Min( t[3], t[4] ) );
   case  8:
      CMPXCHG( 0, 4 ); CMPXCHG( 1, 5 ); CMPXCHG( 2, 6 );
      CMPXCHG( 3, 7 ); CMPXCHG( 0, 2 ); CMPXCHG( 1, 3 );
      CMPXCHG( 4, 6 ); CMPXCHG( 5, 7 ); CMPXCHG( 2, 4 );
      CMPXCHG( 3, 5 ); CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 );
      CMPXCHG( 4, 5 ); CMPXCHG( 6, 7 ); CMPXCHG( 1, 4 );
      CMPXCHG( 3, 6 );
      return MEAN( t[3], t[4] );
   case  9:
      CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 ); CMPXCHG( 7, 8 );
      CMPXCHG( 0, 1 ); CMPXCHG( 3, 4 ); CMPXCHG( 6, 7 );
      CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 ); CMPXCHG( 7, 8 );
      CMPXCHG( 0, 3 ); CMPXCHG( 5, 8 ); CMPXCHG( 4, 7 );
      CMPXCHG( 3, 6 ); CMPXCHG( 1, 4 ); CMPXCHG( 2, 5 );
      CMPXCHG( 4, 7 ); CMPXCHG( 4, 2 ); CMPXCHG( 6, 4 );
      return double( pcl::Min( t[2], t[4] ) );
   case 10:
      CMPXCHG( 4, 9 ); CMPXCHG( 3, 8 ); CMPXCHG( 2, 7 );
      CMPXCHG( 1, 6 ); CMPXCHG( 0, 5 ); CMPXCHG( 1, 4 );
      CMPXCHG( 6, 9 ); CMPXCHG( 0, 3 ); CMPXCHG( 5, 8 );
      CMPXCHG( 0, 2 ); CMPXCHG( 3, 6 ); CMPXCHG( 7, 9 );
      CMPXCHG( 0, 1 ); CMPXCHG( 2, 4 ); CMPXCHG( 5, 7 );
      CMPXCHG( 8, 9 ); CMPXCHG( 1, 2 ); CMPXCHG( 4, 6 );
      CMPXCHG( 7, 8 ); CMPXCHG( 3, 5 ); CMPXCHG( 2, 5 );
      CMPXCHG( 6, 8 ); CMPXCHG( 1, 3 ); CMPXCHG( 4, 7 );
      CMPXCHG( 2, 3 ); CMPXCHG( 6, 7 ); CMPXCHG( 3, 4 );
      CMPXCHG( 5, 6 );
      return MEAN( t[4], t[5] );
   case 11:
      CMPXCHG( 0,  1 ); CMPXCHG( 2,  3 ); CMPXCHG( 4,  5 );
      CMPXCHG( 6,  7 ); CMPXCHG( 8,  9 ); CMPXCHG( 1,  3 );
      CMPXCHG( 5,  7 ); CMPXCHG( 0,  2 ); CMPXCHG( 4,  6 );
      CMPXCHG( 8, 10 ); CMPXCHG( 1,  2 ); CMPXCHG( 5,  6 );
      CMPXCHG( 9, 10 ); CMPXCHG( 1,  5 ); CMPXCHG( 6, 10 );
      CMPXCHG( 5,  9 ); CMPXCHG( 2,  6 ); CMPXCHG( 1,  5 );
      CMPXCHG( 6, 10 ); CMPXCHG( 0,  4 ); CMPXCHG( 3,  7 );
      CMPXCHG( 4,  8 ); CMPXCHG( 0,  4 ); CMPXCHG( 1,  4 );
      CMPXCHG( 7, 10 ); CMPXCHG( 3,  8 ); CMPXCHG( 2,  3 );
      CMPXCHG( 8,  9 ); CMPXCHG( 3,  5 ); CMPXCHG( 6,  8 );
      return double( pcl::Min( t[5], t[6] ) );
   case 12:
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG(  1,  3 ); CMPXCHG(  5,  7 ); CMPXCHG(  9, 11 );
      CMPXCHG(  0,  2 ); CMPXCHG(  4,  6 ); CMPXCHG(  8, 10 );
      CMPXCHG(  1,  2 ); CMPXCHG(  5,  6 ); CMPXCHG(  9, 10 );
      CMPXCHG(  1,  5 ); CMPXCHG(  6, 10 ); CMPXCHG(  5,  9 );
      CMPXCHG(  2,  6 ); CMPXCHG(  1,  5 ); CMPXCHG(  6, 10 );
      CMPXCHG(  0,  4 ); CMPXCHG(  7, 11 ); CMPXCHG(  3,  7 );
      CMPXCHG(  4,  8 ); CMPXCHG(  0,  4 ); CMPXCHG(  7, 11 );
      CMPXCHG(  1,  4 ); CMPXCHG(  7, 10 ); CMPXCHG(  3,  8 );
      CMPXCHG(  2,  3 ); CMPXCHG(  8,  9 ); CMPXCHG(  3,  5 );
      CMPXCHG(  6,  8 );
      return MEAN( t[5], t[6] );
   case 13:
      CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 );
      CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 ); CMPXCHG(  0,  4 );
      CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 );
      CMPXCHG(  8, 12 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG(  0,  2 );
      CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 );
      CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 ); CMPXCHG(  2,  8 );
      CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 ); CMPXCHG(  2,  4 );
      CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 );
      CMPXCHG( 10, 12 ); CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 );
      CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 );
      CMPXCHG( 10, 11 ); CMPXCHG(  1,  8 ); CMPXCHG(  3, 10 );
      CMPXCHG(  5, 12 ); CMPXCHG(  3,  6 ); CMPXCHG(  5,  8 );
      return double( pcl::Max( t[5], t[6] ) );
   case 14:
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG(  0,  2 ); CMPXCHG(  4,  6 );
      CMPXCHG(  8, 10 ); CMPXCHG(  1,  3 ); CMPXCHG(  5,  7 );
      CMPXCHG(  9, 11 ); CMPXCHG(  0,  4 ); CMPXCHG(  8, 12 );
      CMPXCHG(  1,  5 ); CMPXCHG(  9, 13 ); CMPXCHG(  2,  6 );
      CMPXCHG(  3,  7 ); CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 );
      CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 );
      CMPXCHG(  5, 13 ); CMPXCHG(  5, 10 ); CMPXCHG(  6,  9 );
      CMPXCHG(  3, 12 ); CMPXCHG(  7, 11 ); CMPXCHG(  1,  2 );
      CMPXCHG(  4,  8 ); CMPXCHG(  7, 13 ); CMPXCHG(  2,  8 );
      CMPXCHG(  5,  6 ); CMPXCHG(  9, 10 ); CMPXCHG(  3,  8 );
      CMPXCHG(  7, 12 ); CMPXCHG(  6,  8 ); CMPXCHG(  3,  5 );
      CMPXCHG(  7,  9 ); CMPXCHG(  5,  6 ); CMPXCHG(  7,  8 );
      return MEAN( t[6], t[7] );
   case 15:
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG(  0,  2 ); CMPXCHG(  4,  6 );
      CMPXCHG(  8, 10 ); CMPXCHG( 12, 14 ); CMPXCHG(  1,  3 );
      CMPXCHG(  5,  7 ); CMPXCHG(  9, 11 ); CMPXCHG(  0,  4 );
      CMPXCHG(  8, 12 ); CMPXCHG(  1,  5 ); CMPXCHG(  9, 13 );
      CMPXCHG(  2,  6 ); CMPXCHG( 10, 14 ); CMPXCHG(  3,  7 );
      CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 );
      CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 );
      CMPXCHG(  6, 14 ); CMPXCHG(  5, 10 ); CMPXCHG(  6,  9 );
      CMPXCHG(  3, 12 ); CMPXCHG( 13, 14 ); CMPXCHG(  7, 11 );
      CMPXCHG(  1,  2 ); CMPXCHG(  4,  8 ); CMPXCHG(  7, 13 );
      CMPXCHG(  2,  8 ); CMPXCHG(  5,  6 ); CMPXCHG(  9, 10 );
      CMPXCHG(  3,  8 ); CMPXCHG(  7, 12 ); CMPXCHG(  6,  8 );
      CMPXCHG(  3,  5 ); CMPXCHG(  7,  9 ); CMPXCHG(  5,  6 );
      CMPXCHG(  7,  8 );
      return double( pcl::Max( t[6], t[7] ) );
   case 16:
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 ); CMPXCHG(  0,  2 );
      CMPXCHG(  4,  6 ); CMPXCHG(  8, 10 ); CMPXCHG( 12, 14 );
      CMPXCHG(  1,  3 ); CMPXCHG(  5,  7 ); CMPXCHG(  9, 11 );
      CMPXCHG( 13, 15 ); CMPXCHG(  0,  4 ); CMPXCHG(  8, 12 );
      CMPXCHG(  1,  5 ); CMPXCHG(  9, 13 ); CMPXCHG(  2,  6 );
      CMPXCHG( 10, 14 ); CMPXCHG(  3,  7 ); CMPXCHG( 11, 15 );
      CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 );
      CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 );
      CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 ); CMPXCHG(  5, 10 );
      CMPXCHG(  6,  9 ); CMPXCHG(  3, 12 ); CMPXCHG( 13, 14 );
      CMPXCHG(  7, 11 ); CMPXCHG(  1,  2 ); CMPXCHG(  4,  8 );
      CMPXCHG(  7, 13 ); CMPXCHG(  2,  8 ); CMPXCHG(  5,  6 );
      CMPXCHG(  9, 10 ); CMPXCHG(  3,  8 ); CMPXCHG(  7, 12 );
      CMPXCHG(  6,  8 ); CMPXCHG( 10, 12 ); CMPXCHG(  3,  5 );
      CMPXCHG(  7,  9 ); CMPXCHG(  5,  6 ); CMPXCHG(  7,  8 );
      CMPXCHG(  9, 10 ); CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 );
      return MEAN( t[7], t[8] );
   case 17:
      CMPXCHG(  0, 16 ); CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 );
      CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 );
      CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 );
      CMPXCHG(  8, 16 ); CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 );
      CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 );
      CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 );
      CMPXCHG(  4, 16 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 );
      CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 );
      CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 ); CMPXCHG(  2, 16 );
      CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 );
      CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 ); CMPXCHG(  2,  4 );
      CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 );
      CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 );
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 ); CMPXCHG(  1, 16 );
      CMPXCHG(  1,  8 ); CMPXCHG(  3, 10 ); CMPXCHG(  5, 12 );
      CMPXCHG(  7, 14 ); CMPXCHG(  5,  8 ); CMPXCHG(  7, 10 );
      return double( pcl::Max( t[7], t[8] ) );
   case 18:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  0,  8 );
      CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 );
      CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 );
      CMPXCHG(  7, 15 ); CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 );
      CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 );
      CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 );
      CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 ); CMPXCHG(  4, 16 );
      CMPXCHG(  5, 17 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 );
      CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 );
      CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 );
      CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 ); CMPXCHG(  2,  8 );
      CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 );
      CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 ); CMPXCHG(  2,  4 );
      CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 );
      CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 );
      CMPXCHG( 15, 17 ); CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 );
      CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 );
      CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 );
      CMPXCHG( 16, 17 ); CMPXCHG(  1, 16 ); CMPXCHG(  1,  8 );
      CMPXCHG(  3, 10 ); CMPXCHG(  5, 12 ); CMPXCHG(  7, 14 );
      CMPXCHG(  9, 16 ); CMPXCHG(  5,  8 ); CMPXCHG(  7, 10 );
      CMPXCHG(  9, 12 ); CMPXCHG(  7,  8 ); CMPXCHG(  9, 10 );
      return MEAN( t[8], t[9] );
   case 19:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 );
      CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 );
      CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 ); CMPXCHG(  8, 16 );
      CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 ); CMPXCHG(  0,  4 );
      CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 );
      CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 );
      CMPXCHG( 11, 15 ); CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 );
      CMPXCHG(  6, 18 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG(  0,  2 );
      CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 );
      CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 );
      CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 ); CMPXCHG(  2, 16 );
      CMPXCHG(  3, 17 ); CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 );
      CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 );
      CMPXCHG( 11, 17 ); CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 );
      CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 );
      CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 );
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 );
      CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 ); CMPXCHG(  3, 10 );
      CMPXCHG(  5, 12 ); CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 );
      CMPXCHG(  7, 10 ); CMPXCHG(  9, 12 );
      return double( pcl::Min( t[9], t[10] ) );
   case 20:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 );
      CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 );
      CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 );
      CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 );
      CMPXCHG( 11, 19 ); CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 );
      CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 );
      CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 );
      CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 );
      CMPXCHG(  7, 19 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 );
      CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 );
      CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 );
      CMPXCHG( 17, 19 ); CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 );
      CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 );
      CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 );
      CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 );
      CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 );
      CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 ); CMPXCHG(  0,  1 );
      CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 );
      CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 );
      CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 );
      CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 ); CMPXCHG(  3, 10 );
      CMPXCHG(  5, 12 ); CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 );
      CMPXCHG(  7, 10 ); CMPXCHG(  9, 12 );
      return MEAN( t[9], t[10] );
   case 21:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  0,  8 );
      CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 );
      CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 );
      CMPXCHG(  7, 15 ); CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 );
      CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 ); CMPXCHG( 12, 20 );
      CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 );
      CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 );
      CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 ); CMPXCHG( 16, 20 );
      CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 );
      CMPXCHG(  7, 19 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 );
      CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 );
      CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 );
      CMPXCHG( 17, 19 ); CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 );
      CMPXCHG(  6, 20 ); CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 );
      CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 );
      CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 ); CMPXCHG(  2,  4 );
      CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 );
      CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 );
      CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 ); CMPXCHG(  0,  1 );
      CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 );
      CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 );
      CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 );
      CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 ); CMPXCHG(  5, 20 );
      CMPXCHG(  3, 10 ); CMPXCHG(  5, 12 ); CMPXCHG(  7, 14 );
      CMPXCHG(  9, 16 ); CMPXCHG(  7, 10 ); CMPXCHG(  9, 12 );
      return double( pcl::Max( t[9], t[10] ) );
   case 22:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 );
      CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 );
      CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 ); CMPXCHG(  8, 16 );
      CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 );
      CMPXCHG( 12, 20 ); CMPXCHG( 13, 21 ); CMPXCHG(  0,  4 );
      CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 );
      CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 );
      CMPXCHG( 11, 15 ); CMPXCHG( 16, 20 ); CMPXCHG( 17, 21 );
      CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 );
      CMPXCHG(  7, 19 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 );
      CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 );
      CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 );
      CMPXCHG( 17, 19 ); CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 );
      CMPXCHG(  6, 20 ); CMPXCHG(  7, 21 ); CMPXCHG(  2,  8 );
      CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 );
      CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 );
      CMPXCHG( 15, 21 ); CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 );
      CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 );
      CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 );
      CMPXCHG( 18, 20 ); CMPXCHG( 19, 21 ); CMPXCHG(  0,  1 );
      CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 );
      CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 );
      CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 );
      CMPXCHG( 20, 21 ); CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 );
      CMPXCHG(  5, 20 ); CMPXCHG(  3, 10 ); CMPXCHG(  5, 12 );
      CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 ); CMPXCHG( 11, 18 );
      CMPXCHG(  7, 10 ); CMPXCHG(  9, 12 ); CMPXCHG( 11, 14 );
      CMPXCHG(  9, 10 ); CMPXCHG( 11, 12 );
      return MEAN( t[10], t[11] );
   case 23:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 );
      CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 );
      CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 );
      CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 );
      CMPXCHG( 11, 19 ); CMPXCHG( 12, 20 ); CMPXCHG( 13, 21 );
      CMPXCHG( 14, 22 ); CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 );
      CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 );
      CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 );
      CMPXCHG( 16, 20 ); CMPXCHG( 17, 21 ); CMPXCHG( 18, 22 );
      CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 );
      CMPXCHG(  7, 19 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 );
      CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 );
      CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 );
      CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 ); CMPXCHG(  2, 16 );
      CMPXCHG(  3, 17 ); CMPXCHG(  6, 20 ); CMPXCHG(  7, 21 );
      CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 );
      CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 );
      CMPXCHG( 14, 20 ); CMPXCHG( 15, 21 ); CMPXCHG(  2,  4 );
      CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 );
      CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 );
      CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 ); CMPXCHG( 19, 21 );
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 );
      CMPXCHG( 18, 19 ); CMPXCHG( 20, 21 ); CMPXCHG(  1, 16 );
      CMPXCHG(  3, 18 ); CMPXCHG(  5, 20 ); CMPXCHG(  7, 22 );
      CMPXCHG(  5, 12 ); CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 );
      CMPXCHG( 11, 18 ); CMPXCHG(  9, 12 ); CMPXCHG( 11, 14 );
      return double( pcl::Min( t[11], t[12] ) );
   case 24:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  0,  8 );
      CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 );
      CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 );
      CMPXCHG(  7, 15 ); CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 );
      CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 ); CMPXCHG( 12, 20 );
      CMPXCHG( 13, 21 ); CMPXCHG( 14, 22 ); CMPXCHG( 15, 23 );
      CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 );
      CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 );
      CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 ); CMPXCHG( 16, 20 );
      CMPXCHG( 17, 21 ); CMPXCHG( 18, 22 ); CMPXCHG( 19, 23 );
      CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 );
      CMPXCHG(  7, 19 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 );
      CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 );
      CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 );
      CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 ); CMPXCHG( 21, 23 );
      CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 ); CMPXCHG(  6, 20 );
      CMPXCHG(  7, 21 ); CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 );
      CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 );
      CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 ); CMPXCHG( 15, 21 );
      CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 );
      CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 );
      CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 );
      CMPXCHG( 19, 21 ); CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 );
      CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 );
      CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 );
      CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 ); CMPXCHG( 20, 21 );
      CMPXCHG( 22, 23 ); CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 );
      CMPXCHG(  5, 20 ); CMPXCHG(  7, 22 ); CMPXCHG(  5, 12 );
      CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 ); CMPXCHG( 11, 18 );
      CMPXCHG(  9, 12 ); CMPXCHG( 11, 14 );
      return MEAN( t[11], t[12] );
   case 25:
      CMPXCHG(  0,  1 ); CMPXCHG(  3,  4 ); CMPXCHG(  2,  4 );
      CMPXCHG(  2,  3 ); CMPXCHG(  6,  7 ); CMPXCHG(  5,  7 );
      CMPXCHG(  5,  6 ); CMPXCHG(  9, 10 ); CMPXCHG(  8, 10 );
      CMPXCHG(  8,  9 ); CMPXCHG( 12, 13 ); CMPXCHG( 11, 13 );
      CMPXCHG( 11, 12 ); CMPXCHG( 15, 16 ); CMPXCHG( 14, 16 );
      CMPXCHG( 14, 15 ); CMPXCHG( 18, 19 ); CMPXCHG( 17, 19 );
      CMPXCHG( 17, 18 ); CMPXCHG( 21, 22 ); CMPXCHG( 20, 22 );
      CMPXCHG( 20, 21 ); CMPXCHG( 23, 24 ); CMPXCHG(  2,  5 );
      CMPXCHG(  3,  6 ); CMPXCHG(  0,  6 ); CMPXCHG(  0,  3 );
      CMPXCHG(  4,  7 ); CMPXCHG(  1,  7 ); CMPXCHG(  1,  4 );
      CMPXCHG( 11, 14 ); CMPXCHG(  8, 14 ); CMPXCHG(  8, 11 );
      CMPXCHG( 12, 15 ); CMPXCHG(  9, 15 ); CMPXCHG(  9, 12 );
      CMPXCHG( 13, 16 ); CMPXCHG( 10, 16 ); CMPXCHG( 10, 13 );
      CMPXCHG( 20, 23 ); CMPXCHG( 17, 23 ); CMPXCHG( 17, 20 );
      CMPXCHG( 21, 24 ); CMPXCHG( 18, 24 ); CMPXCHG( 18, 21 );
      CMPXCHG( 19, 22 ); CMPXCHG(  8, 17 ); CMPXCHG(  9, 18 );
      CMPXCHG(  0, 18 ); CMPXCHG(  0,  9 ); CMPXCHG( 10, 19 );
      CMPXCHG(  1, 19 ); CMPXCHG(  1, 10 ); CMPXCHG( 11, 20 );
      CMPXCHG(  2, 20 ); CMPXCHG(  2, 11 ); CMPXCHG( 12, 21 );
      CMPXCHG(  3, 21 ); CMPXCHG(  3, 12 ); CMPXCHG( 13, 22 );
      CMPXCHG(  4, 22 ); CMPXCHG(  4, 13 ); CMPXCHG( 14, 23 );
      CMPXCHG(  5, 23 ); CMPXCHG(  5, 14 ); CMPXCHG( 15, 24 );
      CMPXCHG(  6, 24 ); CMPXCHG(  6, 15 ); CMPXCHG(  7, 16 );
      CMPXCHG(  7, 19 ); CMPXCHG( 13, 21 ); CMPXCHG( 15, 23 );
      CMPXCHG(  7, 13 ); CMPXCHG(  7, 15 ); CMPXCHG(  1,  9 );
      CMPXCHG(  3, 11 ); CMPXCHG(  5, 17 ); CMPXCHG( 11, 17 );
      CMPXCHG(  9, 17 ); CMPXCHG(  4, 10 ); CMPXCHG(  6, 12 );
      CMPXCHG(  7, 14 ); CMPXCHG(  4,  6 ); CMPXCHG(  4,  7 );
      CMPXCHG( 12, 14 ); CMPXCHG( 10, 14 ); CMPXCHG(  6,  7 );
      CMPXCHG( 10, 12 ); CMPXCHG(  6, 10 ); CMPXCHG(  6, 17 );
      CMPXCHG( 12, 17 ); CMPXCHG(  7, 17 ); CMPXCHG(  7, 10 );
      CMPXCHG( 12, 18 ); CMPXCHG(  7, 12 ); CMPXCHG( 10, 18 );
      CMPXCHG( 12, 20 ); CMPXCHG( 10, 20 );
      return double( pcl::Max( t[10], t[12] ) );
   case 26:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  8, 24 );
      CMPXCHG(  9, 25 ); CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 );
      CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 );
      CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 );
      CMPXCHG( 16, 24 ); CMPXCHG( 17, 25 ); CMPXCHG(  8, 16 );
      CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 );
      CMPXCHG( 12, 20 ); CMPXCHG( 13, 21 ); CMPXCHG( 14, 22 );
      CMPXCHG( 15, 23 ); CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 );
      CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 );
      CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 );
      CMPXCHG( 16, 20 ); CMPXCHG( 17, 21 ); CMPXCHG( 18, 22 );
      CMPXCHG( 19, 23 ); CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 );
      CMPXCHG(  6, 18 ); CMPXCHG(  7, 19 ); CMPXCHG( 12, 24 );
      CMPXCHG( 13, 25 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG( 20, 24 ); CMPXCHG( 21, 25 ); CMPXCHG(  0,  2 );
      CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 );
      CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 );
      CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 ); CMPXCHG( 17, 19 );
      CMPXCHG( 20, 22 ); CMPXCHG( 21, 23 ); CMPXCHG(  2, 16 );
      CMPXCHG(  3, 17 ); CMPXCHG(  6, 20 ); CMPXCHG(  7, 21 );
      CMPXCHG( 10, 24 ); CMPXCHG( 11, 25 ); CMPXCHG(  2,  8 );
      CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 );
      CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 );
      CMPXCHG( 15, 21 ); CMPXCHG( 18, 24 ); CMPXCHG( 19, 25 );
      CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 );
      CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 );
      CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 );
      CMPXCHG( 19, 21 ); CMPXCHG( 22, 24 ); CMPXCHG( 23, 25 );
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 );
      CMPXCHG( 18, 19 ); CMPXCHG( 20, 21 ); CMPXCHG( 22, 23 );
      CMPXCHG( 24, 25 ); CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 );
      CMPXCHG(  5, 20 ); CMPXCHG(  7, 22 ); CMPXCHG(  9, 24 );
      CMPXCHG(  5, 12 ); CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 );
      CMPXCHG( 11, 18 ); CMPXCHG( 13, 20 ); CMPXCHG(  9, 12 );
      CMPXCHG( 11, 14 ); CMPXCHG( 13, 16 ); CMPXCHG( 11, 12 );
      CMPXCHG( 13, 14 );
      return MEAN( t[12], t[13] );
   case 27:
      CMPXCHG( 0, 16 ); CMPXCHG( 1, 17 ); CMPXCHG( 2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  8, 24 );
      CMPXCHG(  9, 25 ); CMPXCHG( 10, 26 ); CMPXCHG(  0,  8 );
      CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 );
      CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 );
      CMPXCHG(  7, 15 ); CMPXCHG( 16, 24 ); CMPXCHG( 17, 25 );
      CMPXCHG( 18, 26 ); CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 );
      CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 ); CMPXCHG( 12, 20 );
      CMPXCHG( 13, 21 ); CMPXCHG( 14, 22 ); CMPXCHG( 15, 23 );
      CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 );
      CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 );
      CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 ); CMPXCHG( 16, 20 );
      CMPXCHG( 17, 21 ); CMPXCHG( 18, 22 ); CMPXCHG( 19, 23 );
      CMPXCHG(  4, 16 ); CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 );
      CMPXCHG(  7, 19 ); CMPXCHG( 12, 24 ); CMPXCHG( 13, 25 );
      CMPXCHG( 14, 26 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG( 20, 24 ); CMPXCHG( 21, 25 ); CMPXCHG( 22, 26 );
      CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 ); CMPXCHG(  4,  6 );
      CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 ); CMPXCHG(  9, 11 );
      CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 ); CMPXCHG( 16, 18 );
      CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 ); CMPXCHG( 21, 23 );
      CMPXCHG( 24, 26 ); CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 );
      CMPXCHG(  6, 20 ); CMPXCHG(  7, 21 ); CMPXCHG( 10, 24 );
      CMPXCHG( 11, 25 ); CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 );
      CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 );
      CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 ); CMPXCHG( 15, 21 );
      CMPXCHG( 18, 24 ); CMPXCHG( 19, 25 ); CMPXCHG(  2,  4 );
      CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 );
      CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 );
      CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 ); CMPXCHG( 19, 21 );
      CMPXCHG( 22, 24 ); CMPXCHG( 23, 25 ); CMPXCHG(  0,  1 );
      CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 );
      CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 );
      CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 );
      CMPXCHG( 20, 21 ); CMPXCHG( 22, 23 ); CMPXCHG( 24, 25 );
      CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 ); CMPXCHG(  5, 20 );
      CMPXCHG(  7, 22 ); CMPXCHG(  9, 24 ); CMPXCHG( 11, 26 );
      CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 ); CMPXCHG( 11, 18 );
      CMPXCHG( 13, 20 ); CMPXCHG( 11, 14 ); CMPXCHG( 13, 16 );
      return double( pcl::Min( t[13], t[14] ) );
   case 28:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  8, 24 );
      CMPXCHG(  9, 25 ); CMPXCHG( 10, 26 ); CMPXCHG( 11, 27 );
      CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 );
      CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 );
      CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 ); CMPXCHG( 16, 24 );
      CMPXCHG( 17, 25 ); CMPXCHG( 18, 26 ); CMPXCHG( 19, 27 );
      CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 );
      CMPXCHG( 11, 19 ); CMPXCHG( 12, 20 ); CMPXCHG( 13, 21 );
      CMPXCHG( 14, 22 ); CMPXCHG( 15, 23 ); CMPXCHG(  0,  4 );
      CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 );
      CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 );
      CMPXCHG( 11, 15 ); CMPXCHG( 16, 20 ); CMPXCHG( 17, 21 );
      CMPXCHG( 18, 22 ); CMPXCHG( 19, 23 ); CMPXCHG(  4, 16 );
      CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 ); CMPXCHG(  7, 19 );
      CMPXCHG( 12, 24 ); CMPXCHG( 13, 25 ); CMPXCHG( 14, 26 );
      CMPXCHG( 15, 27 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG( 20, 24 ); CMPXCHG( 21, 25 ); CMPXCHG( 22, 26 );
      CMPXCHG( 23, 27 ); CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 );
      CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 );
      CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 );
      CMPXCHG( 16, 18 ); CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 );
      CMPXCHG( 21, 23 ); CMPXCHG( 24, 26 ); CMPXCHG( 25, 27 );
      CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 ); CMPXCHG(  6, 20 );
      CMPXCHG(  7, 21 ); CMPXCHG( 10, 24 ); CMPXCHG( 11, 25 );
      CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 );
      CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 );
      CMPXCHG( 14, 20 ); CMPXCHG( 15, 21 ); CMPXCHG( 18, 24 );
      CMPXCHG( 19, 25 ); CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 );
      CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 );
      CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 );
      CMPXCHG( 18, 20 ); CMPXCHG( 19, 21 ); CMPXCHG( 22, 24 );
      CMPXCHG( 23, 25 ); CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 );
      CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 );
      CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 );
      CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 ); CMPXCHG( 20, 21 );
      CMPXCHG( 22, 23 ); CMPXCHG( 24, 25 ); CMPXCHG( 26, 27 );
      CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 ); CMPXCHG(  5, 20 );
      CMPXCHG(  7, 22 ); CMPXCHG(  9, 24 ); CMPXCHG( 11, 26 );
      CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 ); CMPXCHG( 11, 18 );
      CMPXCHG( 13, 20 ); CMPXCHG( 11, 14 ); CMPXCHG( 13, 16 );
      return MEAN( t[13], t[14] );
   case 29:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  8, 24 );
      CMPXCHG(  9, 25 ); CMPXCHG( 10, 26 ); CMPXCHG( 11, 27 );
      CMPXCHG( 12, 28 ); CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 );
      CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 );
      CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 );
      CMPXCHG( 16, 24 ); CMPXCHG( 17, 25 ); CMPXCHG( 18, 26 );
      CMPXCHG( 19, 27 ); CMPXCHG( 20, 28 ); CMPXCHG(  8, 16 );
      CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 );
      CMPXCHG( 12, 20 ); CMPXCHG( 13, 21 ); CMPXCHG( 14, 22 );
      CMPXCHG( 15, 23 ); CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 );
      CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 );
      CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 );
      CMPXCHG( 16, 20 ); CMPXCHG( 17, 21 ); CMPXCHG( 18, 22 );
      CMPXCHG( 19, 23 ); CMPXCHG( 24, 28 ); CMPXCHG(  4, 16 );
      CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 ); CMPXCHG(  7, 19 );
      CMPXCHG( 12, 24 ); CMPXCHG( 13, 25 ); CMPXCHG( 14, 26 );
      CMPXCHG( 15, 27 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG( 20, 24 ); CMPXCHG( 21, 25 ); CMPXCHG( 22, 26 );
      CMPXCHG( 23, 27 ); CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 );
      CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 );
      CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 );
      CMPXCHG( 16, 18 ); CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 );
      CMPXCHG( 21, 23 ); CMPXCHG( 24, 26 ); CMPXCHG( 25, 27 );
      CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 ); CMPXCHG(  6, 20 );
      CMPXCHG(  7, 21 ); CMPXCHG( 10, 24 ); CMPXCHG( 11, 25 );
      CMPXCHG( 14, 28 ); CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 );
      CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 );
      CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 ); CMPXCHG( 15, 21 );
      CMPXCHG( 18, 24 ); CMPXCHG( 19, 25 ); CMPXCHG( 22, 28 );
      CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 );
      CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 );
      CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 );
      CMPXCHG( 19, 21 ); CMPXCHG( 22, 24 ); CMPXCHG( 23, 25 );
      CMPXCHG( 26, 28 ); CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 );
      CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 );
      CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 );
      CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 ); CMPXCHG( 20, 21 );
      CMPXCHG( 22, 23 ); CMPXCHG( 24, 25 ); CMPXCHG( 26, 27 );
      CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 ); CMPXCHG(  5, 20 );
      CMPXCHG(  7, 22 ); CMPXCHG(  9, 24 ); CMPXCHG( 11, 26 );
      CMPXCHG( 13, 28 ); CMPXCHG(  7, 14 ); CMPXCHG(  9, 16 );
      CMPXCHG( 11, 18 ); CMPXCHG( 13, 20 ); CMPXCHG( 11, 14 );
      CMPXCHG( 13, 16 );
      return double( pcl::Max( t[13], t[14] ) );
   case 30:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  8, 24 );
      CMPXCHG(  9, 25 ); CMPXCHG( 10, 26 ); CMPXCHG( 11, 27 );
      CMPXCHG( 12, 28 ); CMPXCHG( 13, 29 ); CMPXCHG(  0,  8 );
      CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 );
      CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 );
      CMPXCHG(  7, 15 ); CMPXCHG( 16, 24 ); CMPXCHG( 17, 25 );
      CMPXCHG( 18, 26 ); CMPXCHG( 19, 27 ); CMPXCHG( 20, 28 );
      CMPXCHG( 21, 29 ); CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 );
      CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 ); CMPXCHG( 12, 20 );
      CMPXCHG( 13, 21 ); CMPXCHG( 14, 22 ); CMPXCHG( 15, 23 );
      CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 );
      CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 );
      CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 ); CMPXCHG( 16, 20 );
      CMPXCHG( 17, 21 ); CMPXCHG( 18, 22 ); CMPXCHG( 19, 23 );
      CMPXCHG( 24, 28 ); CMPXCHG( 25, 29 ); CMPXCHG(  4, 16 );
      CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 ); CMPXCHG(  7, 19 );
      CMPXCHG( 12, 24 ); CMPXCHG( 13, 25 ); CMPXCHG( 14, 26 );
      CMPXCHG( 15, 27 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG( 20, 24 ); CMPXCHG( 21, 25 ); CMPXCHG( 22, 26 );
      CMPXCHG( 23, 27 ); CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 );
      CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 );
      CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 );
      CMPXCHG( 16, 18 ); CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 );
      CMPXCHG( 21, 23 ); CMPXCHG( 24, 26 ); CMPXCHG( 25, 27 );
      CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 ); CMPXCHG(  6, 20 );
      CMPXCHG(  7, 21 ); CMPXCHG( 10, 24 ); CMPXCHG( 11, 25 );
      CMPXCHG( 14, 28 ); CMPXCHG( 15, 29 ); CMPXCHG(  2,  8 );
      CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 );
      CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 );
      CMPXCHG( 15, 21 ); CMPXCHG( 18, 24 ); CMPXCHG( 19, 25 );
      CMPXCHG( 22, 28 ); CMPXCHG( 23, 29 ); CMPXCHG(  2,  4 );
      CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 );
      CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 );
      CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 ); CMPXCHG( 19, 21 );
      CMPXCHG( 22, 24 ); CMPXCHG( 23, 25 ); CMPXCHG( 26, 28 );
      CMPXCHG( 27, 29 ); CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 );
      CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 );
      CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 );
      CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 ); CMPXCHG( 20, 21 );
      CMPXCHG( 22, 23 ); CMPXCHG( 24, 25 ); CMPXCHG( 26, 27 );
      CMPXCHG( 28, 29 ); CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 );
      CMPXCHG(  5, 20 ); CMPXCHG(  7, 22 ); CMPXCHG(  9, 24 );
      CMPXCHG( 11, 26 ); CMPXCHG( 13, 28 ); CMPXCHG(  7, 14 );
      CMPXCHG(  9, 16 ); CMPXCHG( 11, 18 ); CMPXCHG( 13, 20 );
      CMPXCHG( 15, 22 ); CMPXCHG( 11, 14 ); CMPXCHG( 13, 16 );
      CMPXCHG( 15, 18 ); CMPXCHG( 13, 14 ); CMPXCHG( 15, 16 );
      return MEAN( t[14], t[15] );
   case 31:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  8, 24 );
      CMPXCHG(  9, 25 ); CMPXCHG( 10, 26 ); CMPXCHG( 11, 27 );
      CMPXCHG( 12, 28 ); CMPXCHG( 13, 29 ); CMPXCHG( 14, 30 );
      CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 ); CMPXCHG(  2, 10 );
      CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 ); CMPXCHG(  5, 13 );
      CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 ); CMPXCHG( 16, 24 );
      CMPXCHG( 17, 25 ); CMPXCHG( 18, 26 ); CMPXCHG( 19, 27 );
      CMPXCHG( 20, 28 ); CMPXCHG( 21, 29 ); CMPXCHG( 22, 30 );
      CMPXCHG(  8, 16 ); CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 );
      CMPXCHG( 11, 19 ); CMPXCHG( 12, 20 ); CMPXCHG( 13, 21 );
      CMPXCHG( 14, 22 ); CMPXCHG( 15, 23 ); CMPXCHG(  0,  4 );
      CMPXCHG(  1,  5 ); CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 );
      CMPXCHG(  8, 12 ); CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 );
      CMPXCHG( 11, 15 ); CMPXCHG( 16, 20 ); CMPXCHG( 17, 21 );
      CMPXCHG( 18, 22 ); CMPXCHG( 19, 23 ); CMPXCHG( 24, 28 );
      CMPXCHG( 25, 29 ); CMPXCHG( 26, 30 ); CMPXCHG(  4, 16 );
      CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 ); CMPXCHG(  7, 19 );
      CMPXCHG( 12, 24 ); CMPXCHG( 13, 25 ); CMPXCHG( 14, 26 );
      CMPXCHG( 15, 27 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG( 20, 24 ); CMPXCHG( 21, 25 ); CMPXCHG( 22, 26 );
      CMPXCHG( 23, 27 ); CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 );
      CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 );
      CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 );
      CMPXCHG( 16, 18 ); CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 );
      CMPXCHG( 21, 23 ); CMPXCHG( 24, 26 ); CMPXCHG( 25, 27 );
      CMPXCHG( 28, 30 ); CMPXCHG(  2, 16 ); CMPXCHG(  3, 17 );
      CMPXCHG(  6, 20 ); CMPXCHG(  7, 21 ); CMPXCHG( 10, 24 );
      CMPXCHG( 11, 25 ); CMPXCHG( 14, 28 ); CMPXCHG( 15, 29 );
      CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 ); CMPXCHG(  6, 12 );
      CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 ); CMPXCHG( 11, 17 );
      CMPXCHG( 14, 20 ); CMPXCHG( 15, 21 ); CMPXCHG( 18, 24 );
      CMPXCHG( 19, 25 ); CMPXCHG( 22, 28 ); CMPXCHG( 23, 29 );
      CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 ); CMPXCHG(  6,  8 );
      CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 ); CMPXCHG( 11, 13 );
      CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 ); CMPXCHG( 18, 20 );
      CMPXCHG( 19, 21 ); CMPXCHG( 22, 24 ); CMPXCHG( 23, 25 );
      CMPXCHG( 26, 28 ); CMPXCHG( 27, 29 ); CMPXCHG(  0,  1 );
      CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 ); CMPXCHG(  6,  7 );
      CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 ); CMPXCHG( 12, 13 );
      CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 ); CMPXCHG( 18, 19 );
      CMPXCHG( 20, 21 ); CMPXCHG( 22, 23 ); CMPXCHG( 24, 25 );
      CMPXCHG( 26, 27 ); CMPXCHG( 28, 29 ); CMPXCHG(  1, 16 );
      CMPXCHG(  3, 18 ); CMPXCHG(  5, 20 ); CMPXCHG(  7, 22 );
      CMPXCHG(  9, 24 ); CMPXCHG( 11, 26 ); CMPXCHG( 13, 28 );
      CMPXCHG( 15, 30 ); CMPXCHG(  9, 16 ); CMPXCHG( 11, 18 );
      CMPXCHG( 13, 20 ); CMPXCHG( 15, 22 ); CMPXCHG( 13, 16 );
      CMPXCHG( 15, 18 ); CMPXCHG( 15, 16 );
      return double( pcl::Min( t[15], t[16] ) );
   case 32:
      CMPXCHG(  0, 16 ); CMPXCHG(  1, 17 ); CMPXCHG(  2, 18 );
      CMPXCHG(  3, 19 ); CMPXCHG(  4, 20 ); CMPXCHG(  5, 21 );
      CMPXCHG(  6, 22 ); CMPXCHG(  7, 23 ); CMPXCHG(  8, 24 );
      CMPXCHG(  9, 25 ); CMPXCHG( 10, 26 ); CMPXCHG( 11, 27 );
      CMPXCHG( 12, 28 ); CMPXCHG( 13, 29 ); CMPXCHG( 14, 30 );
      CMPXCHG( 15, 31 ); CMPXCHG(  0,  8 ); CMPXCHG(  1,  9 );
      CMPXCHG(  2, 10 ); CMPXCHG(  3, 11 ); CMPXCHG(  4, 12 );
      CMPXCHG(  5, 13 ); CMPXCHG(  6, 14 ); CMPXCHG(  7, 15 );
      CMPXCHG( 16, 24 ); CMPXCHG( 17, 25 ); CMPXCHG( 18, 26 );
      CMPXCHG( 19, 27 ); CMPXCHG( 20, 28 ); CMPXCHG( 21, 29 );
      CMPXCHG( 22, 30 ); CMPXCHG( 23, 31 ); CMPXCHG(  8, 16 );
      CMPXCHG(  9, 17 ); CMPXCHG( 10, 18 ); CMPXCHG( 11, 19 );
      CMPXCHG( 12, 20 ); CMPXCHG( 13, 21 ); CMPXCHG( 14, 22 );
      CMPXCHG( 15, 23 ); CMPXCHG(  0,  4 ); CMPXCHG(  1,  5 );
      CMPXCHG(  2,  6 ); CMPXCHG(  3,  7 ); CMPXCHG(  8, 12 );
      CMPXCHG(  9, 13 ); CMPXCHG( 10, 14 ); CMPXCHG( 11, 15 );
      CMPXCHG( 16, 20 ); CMPXCHG( 17, 21 ); CMPXCHG( 18, 22 );
      CMPXCHG( 19, 23 ); CMPXCHG( 24, 28 ); CMPXCHG( 25, 29 );
      CMPXCHG( 26, 30 ); CMPXCHG( 27, 31 ); CMPXCHG(  4, 16 );
      CMPXCHG(  5, 17 ); CMPXCHG(  6, 18 ); CMPXCHG(  7, 19 );
      CMPXCHG( 12, 24 ); CMPXCHG( 13, 25 ); CMPXCHG( 14, 26 );
      CMPXCHG( 15, 27 ); CMPXCHG(  4,  8 ); CMPXCHG(  5,  9 );
      CMPXCHG(  6, 10 ); CMPXCHG(  7, 11 ); CMPXCHG( 12, 16 );
      CMPXCHG( 13, 17 ); CMPXCHG( 14, 18 ); CMPXCHG( 15, 19 );
      CMPXCHG( 20, 24 ); CMPXCHG( 21, 25 ); CMPXCHG( 22, 26 );
      CMPXCHG( 23, 27 ); CMPXCHG(  0,  2 ); CMPXCHG(  1,  3 );
      CMPXCHG(  4,  6 ); CMPXCHG(  5,  7 ); CMPXCHG(  8, 10 );
      CMPXCHG(  9, 11 ); CMPXCHG( 12, 14 ); CMPXCHG( 13, 15 );
      CMPXCHG( 16, 18 ); CMPXCHG( 17, 19 ); CMPXCHG( 20, 22 );
      CMPXCHG( 21, 23 ); CMPXCHG( 24, 26 ); CMPXCHG( 25, 27 );
      CMPXCHG( 28, 30 ); CMPXCHG( 29, 31 ); CMPXCHG(  2, 16 );
      CMPXCHG(  3, 17 ); CMPXCHG(  6, 20 ); CMPXCHG(  7, 21 );
      CMPXCHG( 10, 24 ); CMPXCHG( 11, 25 ); CMPXCHG( 14, 28 );
      CMPXCHG( 15, 29 ); CMPXCHG(  2,  8 ); CMPXCHG(  3,  9 );
      CMPXCHG(  6, 12 ); CMPXCHG(  7, 13 ); CMPXCHG( 10, 16 );
      CMPXCHG( 11, 17 ); CMPXCHG( 14, 20 ); CMPXCHG( 15, 21 );
      CMPXCHG( 18, 24 ); CMPXCHG( 19, 25 ); CMPXCHG( 22, 28 );
      CMPXCHG( 23, 29 ); CMPXCHG(  2,  4 ); CMPXCHG(  3,  5 );
      CMPXCHG(  6,  8 ); CMPXCHG(  7,  9 ); CMPXCHG( 10, 12 );
      CMPXCHG( 11, 13 ); CMPXCHG( 14, 16 ); CMPXCHG( 15, 17 );
      CMPXCHG( 18, 20 ); CMPXCHG( 19, 21 ); CMPXCHG( 22, 24 );
      CMPXCHG( 23, 25 ); CMPXCHG( 26, 28 ); CMPXCHG( 27, 29 );
      CMPXCHG(  0,  1 ); CMPXCHG(  2,  3 ); CMPXCHG(  4,  5 );
      CMPXCHG(  6,  7 ); CMPXCHG(  8,  9 ); CMPXCHG( 10, 11 );
      CMPXCHG( 12, 13 ); CMPXCHG( 14, 15 ); CMPXCHG( 16, 17 );
      CMPXCHG( 18, 19 ); CMPXCHG( 20, 21 ); CMPXCHG( 22, 23 );
      CMPXCHG( 24, 25 ); CMPXCHG( 26, 27 ); CMPXCHG( 28, 29 );
      CMPXCHG( 30, 31 ); CMPXCHG(  1, 16 ); CMPXCHG(  3, 18 );
      CMPXCHG(  5, 20 ); CMPXCHG(  7, 22 ); CMPXCHG(  9, 24 );
      CMPXCHG( 11, 26 ); CMPXCHG( 13, 28 ); CMPXCHG( 15, 30 );
      CMPXCHG(  9, 16 ); CMPXCHG( 11, 18 ); CMPXCHG( 13, 20 );
      CMPXCHG( 15, 22 ); CMPXCHG( 13, 16 ); CMPXCHG( 15, 18 );
      return MEAN( t[15], t[16] );
   }

   return 0;
}

// ----------------------------------------------------------------------------

template <typename T>
static double PCL_Median( const T* __restrict__ i, const T* __restrict__ j )
{
   distance_type n = j - i;
   if ( n > 32 )
      return PCL_FastMedian( i, j );
   if ( n < 1 )
      return 0;
   if ( n == 1 )
      return double( *i );
   if ( n == 2 )
      return MEAN( i[0], i[1] );

   Array<T> A( i, j );
   return PCL_SmallMedian( A.Begin(), n );
}

double PCL_FUNC Median( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const signed char* __restrict__ i, const signed char* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const signed short* __restrict__ i, const signed short* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const signed int* __restrict__ i, const signed int* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const signed long* __restrict__ i, const signed long* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const signed long long* __restrict__ i, const signed long long* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const float* __restrict__ i, const float* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const double* __restrict__ i, const double* __restrict__ j )
{
   return PCL_Median( i, j );
}

double PCL_FUNC Median( const long double* __restrict__ i, const long double* __restrict__ j )
{
   return PCL_Median( i, j );
}

// ----------------------------------------------------------------------------

template <typename T>
static double PCL_OrderStatistic( const T* __restrict__ begin, const T* __restrict__ end, distance_type k )
{
   distance_type N = end - begin;
   if ( k < 0 )
      return 0;
   if ( k >= N )
      return 0;

   if ( N <= 2560000 )
   {
      Array<T> A( begin, end );
      return double( *pcl::Select( A.Begin(), A.End(), k ) );
   }

   Array<size_type> L = Thread::OptimalThreadLoads( N, 160*1024/*overheadLimit*/ );

   double low, high;
   {
      ReferenceArray<PCL_MinMaxThread<T>> threads;
      for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
         threads << new PCL_MinMaxThread<T>( begin, n, n + L[i] );

      if ( threads.Length() > 1 )
      {
         int i = 0;
         for ( auto& thread : threads )
            thread.Start( ThreadPriority::DefaultMax, i++ );
         for ( auto& thread : threads )
            thread.Wait();
      }
      else
         threads[0].Run();

      T tlow = threads[0].min;
      T thigh = threads[0].max;
      for ( size_type i = 1; i < threads.Length(); ++i )
      {
         if ( threads[i].min < tlow )
            tlow = threads[i].min;
         if ( threads[i].max > thigh )
            thigh = threads[i].max;
      }

      low = double( tlow );
      high = double( thigh );

      threads.Destroy();
   }

   if ( k == 0 )
      return low;
   if ( k == N-1 )
      return high;

   const double eps = std::is_floating_point<T>::value ?
                        2*std::numeric_limits<T>::epsilon() : 0.5/Pow2( double( sizeof( T ) << 3 ) );
   if ( high - low < eps )
      return low;

   ReferenceArray<PCL_HistogramThread<T>> threads;
   for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
      threads << new PCL_HistogramThread<T>( begin, n, n + L[i], low, high );

   for ( size_type count = 0;; )
   {
      if ( threads.Length() > 1 )
      {
         int i = 0;
         for ( auto& thread : threads )
            thread.Start( ThreadPriority::DefaultMax, i++ );
         for ( auto& thread : threads )
            thread.Wait();
      }
      else
         threads[0].Run();

      SzVector H = threads[0].H;
      for ( size_type i = 1; i < threads.Length(); ++i )
         H += threads[i].H;

      for ( int i = 0; ; count += H[i++] )
         if ( count + H[i] > size_type( k ) )
         {
            double range = high - low;
            high = (range * (i + 1))/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + low;
            low = (range * i)/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + low;
            if ( high - low < eps )
            {
               threads.Destroy();
               return low;
            }
            break;
         }
   }
}

double PCL_FUNC OrderStatistic( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const signed char* __restrict__ i, const signed char* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const signed short* __restrict__ i, const signed short* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const signed int* __restrict__ i, const signed int* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const signed long* __restrict__ i, const signed long* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const signed long long* __restrict__ i, const signed long long* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const float* __restrict__ i, const float* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const double* __restrict__ i, const double* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

double PCL_FUNC OrderStatistic( const long double* __restrict__ i, const long double* __restrict__ j, distance_type k )
{
   return PCL_OrderStatistic( i, j, k );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

template <typename T>
class PCL_AbsDevHistogramThread : public Thread
{
public:

   SzVector H;

   PCL_AbsDevHistogramThread( const T* A, size_type start, size_type stop,
                              double center, const double& low, const double& high )
      : H( __PCL_MEDIAN_HISTOGRAM_LENGTH )
      , m_A( A )
      , m_start( start )
      , m_stop( stop )
      , m_center( center )
      , m_low( low )
      , m_high( high )
   {
   }

   PCL_HOT_FUNCTION void Run() override
   {
      H = size_type( 0 );
      double range = m_high - m_low;
      const T* __restrict__ a = m_A + m_start;
      const T* __restrict__ b = m_A + m_stop;
      do
      {
         double d = pcl::Abs( double( *a ) - m_center );
         if ( d >= m_low )
            if ( d <= m_high )
               ++H[TruncInt( (__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) * (d - m_low)/range )];
      }
      while ( ++a < b );
   }

private:

   const T*      m_A;
   size_type     m_start;
   size_type     m_stop;
   double        m_center;
   const double& m_low;
   const double& m_high;
};

template <typename T>
class PCL_AbsDevMinMaxThread : public Thread
{
public:

   double min;
   double max;

   PCL_AbsDevMinMaxThread( const T* A, size_type start, size_type stop, double center )
      : m_A( A )
      , m_start( start )
      , m_stop( stop )
      , m_center( center )
   {
   }

   PCL_HOT_FUNCTION void Run() override
   {
      const T* __restrict__ a = m_A + m_start;
      const T* __restrict__ b = m_A + m_stop;
      min = max = pcl::Abs( double( *a ) - m_center );
      while ( ++a < b )
      {
         double d = pcl::Abs( double( *a ) - m_center );
         if ( d < min )
            min = d;
         else if ( d > max )
            max = d;
      }
   }

private:

   const T*  m_A;
   size_type m_start;
   size_type m_stop;
   double    m_center;
};

template <typename T>
static double PCL_FastMAD( const T* __restrict__ begin, const T* __restrict__ end, double center )
{
   distance_type N = end - begin;
   if ( N <= 2560000 )
   {
      double* d = new double[ N ];
      double* __restrict__ p = d;
      for ( const T* __restrict__ f = begin; f < end; ++f, ++p )
         *p = Abs( double( *f ) - center );
      double m = *pcl::Select( d, p, N >> 1 );
      if ( (N & 1) == 0 )
         m = (m + *pcl::Select( d, p, (N >> 1)-1 ))/2;
      delete [] d;
      return m;
   }

   Array<size_type> L = Thread::OptimalThreadLoads( N, 160*1024/*overheadLimit*/ );

   double low, high;
   {
      ReferenceArray<PCL_AbsDevMinMaxThread<T>> threads;
      for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
         threads << new PCL_AbsDevMinMaxThread<T>( begin, n, n + L[i], center );

      if ( threads.Length() > 1 )
      {
         int i = 0;
         for ( auto& thread : threads )
            thread.Start( ThreadPriority::DefaultMax, i++ );
         for ( auto& thread : threads )
            thread.Wait();
      }
      else
         threads[0].Run();

      low = threads[0].min;
      high = threads[0].max;
      for ( size_type i = 1; i < threads.Length(); ++i )
      {
         if ( threads[i].min < low )
            low = threads[i].min;
         if ( threads[i].max > high )
            high = threads[i].max;
      }

      threads.Destroy();
   }

   const double eps = 2*std::numeric_limits<double>::epsilon();
   if ( high - low < eps )
      return 0;

   ReferenceArray<PCL_AbsDevHistogramThread<T>> threads;
   for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
      threads << new PCL_AbsDevHistogramThread<T>( begin, n, n + L[i], center, low, high );

   /*
    * High median, extremes and initial histogram saved for odd length.
    */
   double mh = 0, l0 = low, h0 = high;
   SzVector H0;

   for ( size_type count = 0, n2 = N >> 1, step = 0, it = 0;; ++it )
   {
      SzVector H;
      if ( it == 0 && step )
         H = H0;
      else
      {
         if ( threads.Length() > 1 )
         {
            int i = 0;
            for ( auto& thread : threads )
               thread.Start( ThreadPriority::DefaultMax, i++ );
            for ( auto& thread : threads )
               thread.Wait();
         }
         else
            threads[0].Run();

         H = threads[0].H;
         for ( size_type i = 1; i < threads.Length(); ++i )
            H += threads[i].H;
         if ( it == 0 )
            if ( (N & 1) == 0 )
               H0 = H;
      }

      for ( int i = 0; ; count += H[i++] )
         if ( count + H[i] > n2 )
         {
            double range = high - low;
            high = (range * (i + 1))/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + low;
            low  = (range * i)/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + low;
            if ( high - low < eps )
            {
               if ( N & 1 )
               {
                  threads.Destroy();
                  return low;
               }
               if ( step )
               {
                  threads.Destroy();
                  return (low + mh)/2;
               }
               mh = low;
               low = l0;
               high = h0;
               count = 0;
               --n2;
               ++step;
               it = 0;
            }
            break;
         }
   }
}

// ----------------------------------------------------------------------------

template <typename T>
static double PCL_MAD( const T* __restrict__ i, const T* __restrict__ j, double center )
{
   distance_type n = j - i;
   if ( n > 32 )
      return PCL_FastMAD( i, j, center );
   if ( n < 2 )
      return 0;

   double* d = new double[ n ];
   double* __restrict__ p = d;
   for ( const T* __restrict__ f = i; f < j; ++f, ++p )
      *p = Abs( double( *f ) - center );
   double m = PCL_SmallMedian( d, n );
   delete [] d;
   return m;
}

double PCL_FUNC MAD( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const signed char* __restrict__ i, const signed char* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const signed short* __restrict__ i, const signed short* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const signed int* __restrict__ i, const signed int* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const signed long* __restrict__ i, const signed long* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const signed long long* __restrict__ i, const signed long long* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const float* __restrict__ i, const float* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const double* __restrict__ i, const double* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

double PCL_FUNC MAD( const long double* __restrict__ i, const long double* __restrict__ j, double center )
{
   return PCL_MAD( i, j, center );
}

// ----------------------------------------------------------------------------

template <typename T>
class PCL_TwoSidedAbsDevHistogramThread : public Thread
{
public:

   SzVector H;

   PCL_TwoSidedAbsDevHistogramThread( const T* A, size_type start, size_type stop,
                                      double center, const int& side, const double& low, const double& high )
      : H( __PCL_MEDIAN_HISTOGRAM_LENGTH )
      , m_A( A )
      , m_start( start )
      , m_stop( stop )
      , m_center( center )
      , m_side( side )
      , m_low( low )
      , m_high( high )
   {
   }

   PCL_HOT_FUNCTION void Run() override
   {
      H = size_type( 0 );
      double range = m_high - m_low;
      const T* __restrict__ a = m_A + m_start;
      const T* __restrict__ b = m_A + m_stop;
      do
      {
         double x = double( *a );
         if ( m_side > 0 == x > m_center )
         {
            double d = m_side ? x - m_center : m_center - x;
            if ( d >= m_low )
               if ( d <= m_high )
                  ++H[TruncInt( (__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) * (d - m_low)/range )];
         }
      }
      while ( ++a < b );
   }

private:

   const T*      m_A;
   size_type     m_start;
   size_type     m_stop;
   double        m_center;
   const int&    m_side;
   const double& m_low;
   const double& m_high;
};

template <typename T>
class PCL_TwoSidedAbsDevMinMaxThread : public Thread
{
public:

   double minLow = 0, minHigh = 0;
   double maxLow = 0, maxHigh = 0;
   size_type nLow = 0;
   size_type nHigh = 0;

   PCL_TwoSidedAbsDevMinMaxThread( const T* A, size_type start, size_type stop, double center )
      : m_A( A )
      , m_start( start )
      , m_stop( stop )
      , m_center( center )
   {
   }

   PCL_HOT_FUNCTION void Run() override
   {
      const T* __restrict__ a = m_A + m_start;
      const T* __restrict__ b = m_A + m_stop;
      do
      {
         double x = double( *a );
         if ( x <= m_center )
         {
            double d = m_center - x;
            if ( nLow++ > 0 )
            {
               if ( d < minLow )
                  minLow = d;
               else if ( d > maxLow )
                  maxLow = d;
            }
            else
               minLow = maxLow = d;
         }
         else
         {
            double d = x - m_center;
            if ( nHigh++ > 0 )
            {
               if ( d < minHigh )
                  minHigh = d;
               else if ( d > maxHigh )
                  maxHigh = d;
            }
            else
               minHigh = maxHigh = d;
         }
      }
      while ( ++a < b );
   }

private:

   const T*  m_A;
   size_type m_start;
   size_type m_stop;
   double    m_center;
};

template <typename T>
static TwoSidedEstimate PCL_TwoSidedFastMAD( const T* __restrict__ begin, const T* __restrict__ end, double center )
{
   distance_type N = end - begin;
   if ( N <= 2560000 )
   {
      double* d = new double[ N ];
      double* __restrict__ p = d;
      double* __restrict__ q = d + N;
      do
      {
         double x = double( *begin++ );
         if ( x <= center )
            *p++ = center - x;
         else
            *--q = x - center;
      }
      while( begin < end );
      double l = pcl::Median( d, p );
      double h = pcl::Median( q, d+N );
      delete [] d;
      return { l, h };
   }

   Array<size_type> L = Thread::OptimalThreadLoads( N, 160*1024/*overheadLimit*/ );

   double minLow = 0, minHigh = 0, maxLow = 0, maxHigh = 0;
   size_type nLow = 0, nHigh = 0;
   {
      ReferenceArray<PCL_TwoSidedAbsDevMinMaxThread<T>> threads;
      for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
         threads << new PCL_TwoSidedAbsDevMinMaxThread<T>( begin, n, n + L[i], center );

      if ( threads.Length() > 1 )
      {
         int i = 0;
         for ( auto& thread : threads )
            thread.Start( ThreadPriority::DefaultMax, i++ );
         for ( auto& thread : threads )
            thread.Wait();
      }
      else
         threads[0].Run();

      for ( size_type i = 0; i < threads.Length(); ++i )
         if ( threads[i].nLow > 0 )
         {
            minLow = threads[i].minLow;
            maxLow = threads[i].maxLow;
            nLow = threads[i].nLow;
            while ( ++i < threads.Length() )
               if ( threads[i].nLow > 0 )
               {
                  if ( threads[i].minLow < minLow )
                     minLow = threads[i].minLow;
                  if ( threads[i].maxLow > maxLow )
                     maxLow = threads[i].maxLow;
                  nLow += threads[i].nLow;
               }
            break;
         }

      for ( size_type i = 0; i < threads.Length(); ++i )
         if ( threads[i].nHigh > 0 )
         {
            minHigh = threads[i].minHigh;
            maxHigh = threads[i].maxHigh;
            nHigh = threads[i].nHigh;
            while ( ++i < threads.Length() )
               if ( threads[i].nHigh > 0 )
               {
                  if ( threads[i].minHigh < minHigh )
                     minHigh = threads[i].minHigh;
                  if ( threads[i].maxHigh > maxHigh )
                     maxHigh = threads[i].maxHigh;
                  nHigh += threads[i].nHigh;
               }
            break;
         }

      threads.Destroy();
   }

   int side;
   double sideLow, sideHigh;
   ReferenceArray<PCL_TwoSidedAbsDevHistogramThread<T>> threads;
   for ( size_type i = 0, n = 0; i < L.Length(); n += L[i++] )
      threads << new PCL_TwoSidedAbsDevHistogramThread<T>( begin, n, n + L[i], center, side, sideLow, sideHigh );

   const double eps = 2*std::numeric_limits<double>::epsilon();
   double mad[ 2 ];
   for ( side = 0; side < 2; ++side )
   {
      distance_type n = side ? nHigh : nLow;
      if ( n < 2 )
      {
         mad[side] = 0;
         continue;
      }

      sideLow = side ? minHigh : minLow;
      sideHigh = side ? maxHigh : maxLow;
      if ( sideHigh < eps )
      {
         mad[side] = 0;
         continue;
      }

      /*
       * High median, extremes and initial histogram saved for odd length.
       */
      double mh = 0, h0 = sideHigh;
      SzVector H0;

      for ( size_type count = 0, n2 = n >> 1, step = 0, it = 0;; ++it )
      {
         SzVector H;
         if ( it == 0 && step )
            H = H0;
         else
         {
            if ( threads.Length() > 1 )
            {
               int i = 0;
               for ( auto& thread : threads )
                  thread.Start( ThreadPriority::DefaultMax, i++ );
               for ( auto& thread : threads )
                  thread.Wait();
            }
            else
               threads[0].Run();

            H = threads[0].H;
            for ( size_type i = 1; i < threads.Length(); ++i )
               H += threads[i].H;
            if ( it == 0 )
               if ( (n & 1) == 0 )
                  H0 = H;
         }

         for ( int i = 0; ; count += H[i++] )
            if ( count + H[i] > n2 )
            {
               double range = sideHigh - sideLow;
               sideHigh = (range * (i + 1))/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + sideLow;
               sideLow  = (range * i)/(__PCL_MEDIAN_HISTOGRAM_LENGTH - 1) + sideLow;
               if ( sideHigh - sideLow < eps )
               {
                  if ( n & 1 )
                  {
                     mad[side] = sideLow;
                     goto __madNextSide;
                  }
                  if ( step )
                  {
                     mad[side] = (sideLow + mh)/2;
                     goto __madNextSide;
                  }
                  mh = sideLow;
                  sideLow = 0;
                  sideHigh = h0;
                  count = 0;
                  --n2;
                  ++step;
                  it = 0;
               }
               break;
            }
      }

__madNextSide:
      ;
   }

   threads.Destroy();
   return { mad[0], mad[1] };
}

template <typename T>
static TwoSidedEstimate PCL_TwoSidedMAD( const T* __restrict__ i, const T* __restrict__ j, double center )
{
   distance_type n = j - i;
   if ( n >= 2 )
      return PCL_TwoSidedFastMAD( i, j, center );
   return { 0, 0 };
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed char* __restrict__ i, const signed char* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed short* __restrict__ i, const signed short* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed int* __restrict__ i, const signed int* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed long* __restrict__ i, const signed long* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed long long* __restrict__ i, const signed long long* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const float* __restrict__ i, const float* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const double* __restrict__ i, const double* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const long double* __restrict__ i, const long double* __restrict__ j, double center )
{
   return PCL_TwoSidedMAD( i, j, center );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/Median.cpp - Released 2020-12-17T15:46:35Z
