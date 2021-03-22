//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/Convolution.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/Convolution.h>
#include <pcl/MultiVector.h>
#include <pcl/Thread.h>

namespace pcl
{

// ----------------------------------------------------------------------------

class PCL_CorrelationEngine
{
public:

   template <class P> static
   void Apply( GenericImage<P>& image, const Convolution& convolution )
   {
      if ( convolution.IsHighPassFilter() )
      {
         if ( P::BitsPerSample() < 32 )
            HighPassIntegerImage( image, convolution, reinterpret_cast<Image*>( 0 ) );
         else
            HighPassIntegerImage( image, convolution, reinterpret_cast<DImage*>( 0 ) );
      }
      else
         DoApply( image, convolution );
   }

   static
   void Apply( Image& image, const Convolution& convolution )
   {
      DoApply( image, convolution );
      if ( convolution.IsHighPassFilter() )
         HighPassRescaleFloatImage( image, convolution );
   }

   static
   void Apply( DImage& image, const Convolution& convolution )
   {
      DoApply( image, convolution );
      if ( convolution.IsHighPassFilter() )
         HighPassRescaleFloatImage( image, convolution );
   }

private:

   template <class P> static
   void DoApply( GenericImage<P>& image, const Convolution& convolution )
   {
      if ( image.IsEmptySelection() )
         return;

      if ( convolution.Filter().IsEmpty() )
         throw Error( "Attempt to perform a convolution with an empty kernel filter." );

      image.EnsureUnique();

      int n = convolution.OverlappingDistance();
      if ( n > image.Height() || n > image.Width() )
      {
         image.Zero();
         return;
      }

      /*
       * We implement a correlation algorithm, so make sure that the
       * convolution filter is rotated by 180 degrees. We'll unrotate it once
       * convolution has finished.
       */
      bool didFlip = false;
      if ( !convolution.Filter().IsFlipped() )
      {
         const_cast<KernelFilter&>( convolution.Filter() ).Flip();
         didFlip = true;
      }

      Array<size_type> L = pcl::Thread::OptimalThreadLoads( image.SelectedRectangle().Height(),
                                       n/*overheadLimit*/,
                                       convolution.IsParallelProcessingEnabled() ? convolution.MaxProcessors() : 1 );
      int numberOfThreads = int( L.Length() );

      size_type N = image.NumberOfSelectedSamples();
      if ( image.Status().IsInitializationEnabled() )
         image.Status().Initialize( "Convolution", N );

      ThreadData<P> data( image, convolution, N );

      ReferenceArray<Thread<P> > threads;
      for ( int i = 0, n = 0, y0 = image.SelectedRectangle().y0; i < numberOfThreads; n += int( L[i++] ) )
         threads.Add( new Thread<P>( data,
                                     y0 + n,
                                     y0 + n + int( L[i] ),
                                     i > 0,
                                     i < numberOfThreads-1 ) );
      try
      {
         AbstractImage::RunThreads( threads, data );
         if ( didFlip )
            const_cast<KernelFilter&>( convolution.Filter() ).Flip();
      }
      catch ( ... )
      {
         if ( didFlip )
            const_cast<KernelFilter&>( convolution.Filter() ).Flip();
         throw;
      }

      image.SetStatusCallback( nullptr );

      int c0 = image.SelectedChannel();
      Point p0 = image.SelectedRectangle().LeftTop();

      for ( int i = 0, n = 0; i < numberOfThreads; n += int( L[i++] ) )
      {
         if ( i > 0 )
            image.Mov( threads[i].UpperOverlappingRegion(),
                       Point( p0.x, p0.y + n ), c0 );
         if ( i < numberOfThreads-1 )
            image.Mov( threads[i].LowerOverlappingRegion(),
                       Point( p0.x, p0.y + n + int( L[i] ) - threads[i].LowerOverlappingRegion().Height() ), c0 );
      }

      image.Status() = data.status;

      threads.Destroy();
   }

   template <class P, class P1> static
   void HighPassIntegerImage( GenericImage<P>& image, const Convolution& convolution, GenericImage<P1>* )
   {
      GenericImage<P1> tmp( image );
      Apply( tmp, convolution );

      StatusMonitor monitor = tmp.Status();
      image.SetStatusCallback( nullptr );

      image.Mov( tmp, image.SelectedRectangle().LeftTop() );

      image.Status() = monitor;
   }

   template <class P> static
   void HighPassRescaleFloatImage( GenericImage<P>& image, const Convolution& convolution )
   {
      if ( !convolution.IsRawHighPassEnabled() )
         if ( convolution.IsHighPassFilter() )
         {
            StatusMonitor monitor = image.Status();
            image.SetStatusCallback( nullptr );

            if ( convolution.IsHighPassRescalingEnabled() )
               image.Normalize();
            else
               image.Truncate();

            image.Status() = monitor;
         }
   }

   template <class P>
   struct ThreadData : public AbstractImage::ThreadData
   {
      ThreadData( GenericImage<P>& a_image, const Convolution& a_convolution, size_type a_count )
         : AbstractImage::ThreadData( a_image, a_count )
         , image( a_image )
         , convolution( a_convolution )
      {
      }

            GenericImage<P>& image;
      const Convolution&     convolution;
   };

   template <class P>
   class Thread : public pcl::Thread
   {
   public:

      typedef GenericImage<P>                         region;
      typedef GenericMultiVector<typename P::sample>  raw_data;

      Thread( ThreadData<P>& data, int startRow, int endRow, bool upperOvRgn, bool lowerOvRgn )
         : m_data( data )
         , m_firstRow( startRow )
         , m_endRow( endRow )
         , m_haveUpperOvRgn( upperOvRgn )
         , m_haveLowerOvRgn( lowerOvRgn )
      {
      }

      PCL_HOT_FUNCTION void Run() override
      {
         INIT_THREAD_MONITOR()

         Rect r = m_data.image.SelectedRectangle();
         int w = r.Width();
         int dw = m_data.image.Width() - w;

         int m = m_data.convolution.Filter().Size();
         int n = m_data.convolution.OverlappingDistance();
         int n2 = n >> 1;
         int nf0 = w + (n2 << 1);

         int o0 = m_firstRow;
         if ( m_haveUpperOvRgn )
         {
            m_upperOvRgn.AllocateData( w, n2, m_data.image.NumberOfSelectedChannels() );
            o0 += n2;
         }

         int o1 = m_endRow;
         if ( m_haveLowerOvRgn )
         {
            m_lowerOvRgn.AllocateData( w, n2, m_data.image.NumberOfSelectedChannels() );
            o1 -= n2;
         }

         typename P::sample th0 = P::ToSample( m_data.convolution.LowThreshold() );
         typename P::sample th1 = P::ToSample( m_data.convolution.HighThreshold() );

         bool tz0 = 1 + th0 == 1;
         bool tz1 = 1 + th1 == 1;
         bool tz = tz0 && tz1;
         bool unitWeight = m_data.convolution.FilterWeight() == 1;

         double (*innerLoop)( typename raw_data::const_iterator, typename raw_data::const_iterator,
                              const KernelFilter::coefficient* __restrict__, int, int, int ) noexcept;
         if ( m_data.convolution.IsInterlaced() )
         {
            switch ( m )
            {
            case 3:
               innerLoop = InnerLoop_Interlaced_3x3;
               break;
            case 5:
               innerLoop = InnerLoop_Interlaced_5x5;
               break;
            default:
               innerLoop = InnerLoop_Interlaced;
               break;
            }
         }
         else
         {
            switch ( m )
            {
            case 3:
               innerLoop = InnerLoop_Compact_3x3;
               break;
            case 5:
               innerLoop = InnerLoop_Compact_5x5;
               break;
            default:
               innerLoop = InnerLoop_Compact;
               break;
            }
         }

         raw_data f0( P::MinSampleValue(), n, nf0 );

         for ( int c = m_data.image.FirstSelectedChannel(), cn = 0; c <= m_data.image.LastSelectedChannel(); ++c, ++cn )
         {
            typename P::sample* f = m_data.image.PixelAddress( r.x0, m_firstRow, c );
            typename P::sample* g = m_haveUpperOvRgn ? m_upperOvRgn[cn] : nullptr;

            for ( int i = 0, i0 = m_firstRow-n2, i1 = m_firstRow+n2-1; i < n2; ++i, ++i0, --i1 )
               ::memcpy( f0[i].At( n2 ), m_data.image.PixelAddress( r.x0, (i0 < 0) ? i1 : i0, c ), w*P::BytesPerSample() );

            for ( int i = n2, i1 = m_firstRow; i < n; ++i, ++i1 )
               ::memcpy( f0[i].At( n2 ), m_data.image.PixelAddress( r.x0, i1, c ), w*P::BytesPerSample() );

            for ( int i = 0; i < n; ++i )
            {
               typename raw_data::vector_iterator f0i = *f0[i];
               typename raw_data::vector_iterator f1i = f0i + n2+n2;
               PCL_UNROLL( 8 )
               do
                  *f0i++ = *f1i--;
               while ( f0i < f1i );

               f0i = f0[i].At( w-1 );
               f1i = f0i + n2+n2;
               PCL_UNROLL( 8 )
               do
                  *f1i-- = *f0i++;
               while ( f0i < f1i );
            }

            for ( int y = m_firstRow;; )
            {
               if ( likely( tz ) )
               {
                  for ( int x = 0; x < w; ++x, ++f )
                  {
                     double r = (*innerLoop)( f0.Begin(), f0.End(), m_data.convolution.Filter().Begin(),
                                              x, m, m_data.convolution.InterlacingDistance() );
                     if ( !unitWeight )
                        r /= m_data.convolution.FilterWeight();

                     if ( g == nullptr )
                        *f = P::FloatToSample( r );
                     else
                        *g++ = P::FloatToSample( r );
                  }
               }
               else
               {
                  for ( int x = 0; x < w; ++x, ++f )
                  {
                     double r = (*innerLoop)( f0.Begin(), f0.End(), m_data.convolution.Filter().Begin(),
                                              x, m, m_data.convolution.InterlacingDistance() );
                     if ( !unitWeight )
                        r /= m_data.convolution.FilterWeight();

                     if ( r < *f )
                     {
                        if ( !tz0 )
                        {
                           double k = *f - r;
                           if ( k < th0 )
                           {
                              k /= th0;
                              r = k*r + (1 - k)*(*f);
                           }
                        }
                     }
                     else
                     {
                        if ( !tz1 )
                        {
                           double k = r - *f;
                           if ( k < th1 )
                           {
                              k /= th1;
                              r = k*r + (1 - k)*(*f);
                           }
                        }
                     }

                     if ( g == nullptr )
                        *f = P::FloatToSample( r );
                     else
                        *g++ = P::FloatToSample( r );
                  }
               }

               if ( ++y == m_endRow )
                  break;

               f += dw;

               if ( g == nullptr )
               {
                  if ( m_haveLowerOvRgn )
                     if ( y == o1 )
                        g = m_lowerOvRgn[cn];
               }
               else
               {
                  if ( y == o0 )
                     g = nullptr;
               }

               for ( int i = 1; i < n; ++i )
                  pcl::Swap( f0[i-1], f0[i] );

               if ( y+n2 < m_data.image.Height() )
               {
                  ::memcpy( f0[n-1].At( n2 ), m_data.image.PixelAddress( r.x0, y+n2, c ), w*P::BytesPerSample() );

                  typename raw_data::vector_iterator f0n = *f0[n-1];
                  typename raw_data::vector_iterator f1n = f0n + n2+n2;
                  PCL_UNROLL( 8 )
                  do
                     *f0n++ = *f1n--;
                  while ( f0n < f1n );

                  f0n = f0[n-1].At( w-1 );
                  f1n = f0n + n2+n2;
                  PCL_UNROLL( 8 )
                  do
                     *f1n-- = *f0n++;
                  while ( f0n < f1n );
               }
               else
               {
                  ::memcpy( *f0[n-1], *f0[n-2], nf0*P::BytesPerSample() );
                  /*
                   * ### N.B.: Cannot use an assignment operator here because
                   * all of the f0 vectors must be unique.
                   */
                  //f0[n-1] = f0[n-2];
               }
            }

            UPDATE_THREAD_MONITOR( 1024 )
         }
      }

      const region& UpperOverlappingRegion() const
      {
         return m_upperOvRgn;
      }

      const region& LowerOverlappingRegion() const
      {
         return m_lowerOvRgn;
      }

   private:

      ThreadData<P>& m_data;
      int            m_firstRow;
      int            m_endRow;
      region         m_upperOvRgn;
      region         m_lowerOvRgn;
      bool           m_haveUpperOvRgn;
      bool           m_haveLowerOvRgn;

      /*
       * Compact convolution.
       */
      static double InnerLoop_Compact( typename raw_data::const_iterator i,
                                       typename raw_data::const_iterator j,
                                       const KernelFilter::coefficient* __restrict__ h,
                                       int x, int n, int/*d*/ ) noexcept
      {
         double r = 0;
         for ( ; i < j; ++i )
         {
            typename raw_data::const_vector_iterator __restrict__ fi = i->At( x );
            for ( int k = 0; k < n; ++k )
               r += *h++ * *fi++;
         }
         return r;
      }

      static double InnerLoop_Compact_3x3( typename raw_data::const_iterator i,
                                           typename raw_data::const_iterator /*j*/,
                                           const KernelFilter::coefficient* __restrict__ h,
                                           int x, int/*n*/, int/*d*/ ) noexcept
      {
         typename P::sample f[ 9 ] =
            {
               i[0][x  ], i[1][x  ], i[2][x  ],
               i[0][x+1], i[1][x+1], i[2][x+1],
               i[0][x+2], i[1][x+2], i[2][x+2]
            };
         double r = 0;
         for ( int k = 0; k < 9; ++k )
            r += *h++ * f[k];
         return r;
      }

      static double InnerLoop_Compact_5x5( typename raw_data::const_iterator i,
                                           typename raw_data::const_iterator /*j*/,
                                           const KernelFilter::coefficient* __restrict__ h,
                                           int x, int/*n*/, int/*d*/ ) noexcept
      {
         typename P::sample f[ 25 ] =
            {
               i[0][x  ], i[1][x  ], i[2][x  ], i[3][x  ], i[4][x  ],
               i[0][x+1], i[1][x+1], i[2][x+1], i[3][x+1], i[4][x+1],
               i[0][x+2], i[1][x+2], i[2][x+2], i[3][x+2], i[4][x+2],
               i[0][x+3], i[1][x+3], i[2][x+3], i[3][x+3], i[4][x+3],
               i[0][x+4], i[1][x+4], i[2][x+4], i[3][x+4], i[4][x+4]
            };
         double r = 0;
         for ( int k = 0; k < 25; ++k )
            r += *h++ * f[k];
         return r;
      }

      /*
       * Interlaced convolution (e.g., the à trous algorithm).
       */
      static double InnerLoop_Interlaced( typename raw_data::const_iterator i,
                                          typename raw_data::const_iterator j,
                                          const KernelFilter::coefficient* __restrict__ h,
                                          int x, int n, int d ) noexcept
      {
         double r = 0;
         for ( ; i < j; i += d )
         {
            typename raw_data::const_vector_iterator __restrict__ fi = i->At( x );
            for ( int k = 0, l = 0; k < n; ++k, l += d )
               r += *h++ * fi[l];
         }
         return r;
      }

      static double InnerLoop_Interlaced_3x3( typename raw_data::const_iterator i,
                                              typename raw_data::const_iterator /*j*/,
                                              const KernelFilter::coefficient* __restrict__ h,
                                              int x, int/*n*/, int d ) noexcept
      {
         int d2 = 2*d;
         typename P::sample f[ 9 ] =
            {
               i[0][x   ], i[d][x   ], i[d2][x   ],
               i[0][x+d ], i[d][x+d ], i[d2][x+d ],
               i[0][x+d2], i[d][x+d2], i[d2][x+d2]
            };
         double r = 0;
         for ( int k = 0; k < 9; ++k )
            r += *h++ * f[k];
         return r;
      }

      static double InnerLoop_Interlaced_5x5( typename raw_data::const_iterator i,
                                              typename raw_data::const_iterator /*j*/,
                                              const KernelFilter::coefficient* __restrict__ h,
                                              int x, int/*n*/, int d ) noexcept
      {
         int d2 = 2*d, d3 = 3*d, d4 = 4*d;
         typename P::sample f[ 25 ] =
            {
               i[0][x   ], i[d][x   ], i[d2][x   ], i[d3][x   ], i[d4][x   ],
               i[0][x+d ], i[d][x+d ], i[d2][x+d ], i[d3][x+d ], i[d4][x+d ],
               i[0][x+d2], i[d][x+d2], i[d2][x+d2], i[d3][x+d2], i[d4][x+d2],
               i[0][x+d3], i[d][x+d3], i[d2][x+d3], i[d3][x+d3], i[d4][x+d3],
               i[0][x+d4], i[d][x+d4], i[d2][x+d4], i[d3][x+d4], i[d4][x+d4]
            };
         double r = 0;
         for ( int k = 0; k < 25; ++k )
            r += *h++ * f[k];
         return r;
      }
   };
};

// ----------------------------------------------------------------------------

void Convolution::Apply( Image& image ) const
{
   PCL_PRECONDITION( !m_filter.IsNull() )
   ValidateFilter();
   PCL_CorrelationEngine::Apply( image, *this );
}

void Convolution::Apply( DImage& image ) const
{
   PCL_PRECONDITION( !m_filter.IsNull() )
   ValidateFilter();
   PCL_CorrelationEngine::Apply( image, *this );
}

void Convolution::Apply( UInt8Image& image ) const
{
   PCL_PRECONDITION( !m_filter.IsNull() )
   ValidateFilter();
   PCL_CorrelationEngine::Apply( image, *this );
}

void Convolution::Apply( UInt16Image& image ) const
{
   PCL_PRECONDITION( !m_filter.IsNull() )
   ValidateFilter();
   PCL_CorrelationEngine::Apply( image, *this );
}

void Convolution::Apply( UInt32Image& image ) const
{
   PCL_PRECONDITION( !m_filter.IsNull() )
   ValidateFilter();
   PCL_CorrelationEngine::Apply( image, *this );
}

// ----------------------------------------------------------------------------

void Convolution::ValidateFilter() const
{
   if ( m_filter.IsNull() )
      throw Error( "Invalid access to uninitialized convolution" );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/Convolution.cpp - Released 2020-12-17T15:46:35Z
