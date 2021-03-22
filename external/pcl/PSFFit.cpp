//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/PSFFit.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/CubicSplineInterpolation.h>
#include <pcl/PSFFit.h>
#include <pcl/Thread.h>

#include <cminpack/cminpack.h>

#include <limits>

#ifdef __PCL_WINDOWS
#undef max
#endif

namespace pcl
{

// ----------------------------------------------------------------------------

class PSFFitEngine
{
public:

   // -------------------------------------------------------------------------
   // Elliptical PSF Functions
   // -------------------------------------------------------------------------

#define F      reinterpret_cast<const PSFFit*>( p )
#define B      a[0]
#define A      a[1]
#define x0     a[2]
#define y0     a[3]
#define sx     a[4]
#define sy     a[5]
#define theta  a[6]
#define beta   a[7]

   /*
    * Elliptical Gaussian PSF.
    */
   static int FitGaussian( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A*Exp(-(a*(x - x0)^2 + 2*b*(x - x0)*(y - y0) + c*(y - y0)^2))
       */
      if ( B < 0 || A < 0 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      double st, ct;
      SinCos( theta, st, ct );
      double sct  = st*ct;
      double st2  = st*st;
      double ct2  = ct*ct;
      double ksx2 = 2*sx*sx;
      double ksy2 = 2*sy*sy;
      double p1   = ct2/ksx2 + st2/ksy2;
      double p2   = sct/ksy2 - sct/ksx2;
      double p3   = st2/ksx2 + ct2/ksy2;
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dy = y - h2y0;
         double twop2dy = 2*p2*dy;
         double p3dy2 = p3*dy*dy;
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
         {
            double dx = x - w2x0;
            *fvec++ = *s++ - B - A*Exp( -(p1*dx*dx + twop2dy*dx + p3dy2) );
         }
      }
      return 0;
   }

   /*
    * Elliptical Moffat PSF, variable beta exponent.
    */
   static int FitMoffat( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A/(1 + a*(x - x0)^2 + 2*b*(x - x0)*(y - y0) + c*(y - y0)^2)^beta
       */
      if ( B < 0 || A < 0 || beta < 0 || beta > 10 || Abs( beta - F->m_beta )/F->m_beta > 0.05 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      F->m_beta = beta;

      double st, ct;
      SinCos( theta, st, ct );
      double sct = st*ct;
      double st2 = st*st;
      double ct2 = ct*ct;
      double sx2 = sx*sx;
      double sy2 = sy*sy;
      double p1  = ct2/sx2 + st2/sy2;
      double p2  = sct/sy2 - sct/sx2;
      double p3  = st2/sx2 + ct2/sy2;
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dy = y - h2y0;
         double twop2dy = 2*p2*dy;
         double p3dy2 = p3*dy*dy;
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
         {
            double dx = x - w2x0;
            *fvec++ = *s++ - B - A/Pow( 1 + p1*dx*dx + twop2dy*dx + p3dy2, beta );
         }
      }
      return 0;
   }

#undef beta
#define beta   F->P[7]

   /*
    * Elliptical Moffat PSF, prescribed beta exponent.
    */
   static int FitMoffatWithFixedBeta( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A/(1 + a*(x - x0)^2 + 2*b*(x - x0)*(y - y0) + c*(y - y0)^2)^beta
       */
      if ( B < 0 || A < 0 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      double st, ct;
      SinCos( theta, st, ct );
      double sct = st*ct;
      double st2 = st*st;
      double ct2 = ct*ct;
      double sx2 = sx*sx;
      double sy2 = sy*sy;
      double p1  = ct2/sx2 + st2/sy2;
      double p2  = sct/sy2 - sct/sx2;
      double p3  = st2/sx2 + ct2/sy2;
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dy = y - h2y0;
         double twop2dy = 2*p2*dy;
         double p3dy2 = p3*dy*dy;
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
         {
            double dx = x - w2x0;
            *fvec++ = *s++ - B - A/Pow( 1 + p1*dx*dx + twop2dy*dx + p3dy2, beta );
         }
      }
      return 0;
   }

   /*
    * Elliptical variable shape PSF.
    */
   static int FitVShape( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A*Exp( -((dx^k)/(k*sx^k) + (dy^k)/(k*sy^k)) )
       */
      if ( B < 0 || A < 0 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      double st, ct;
      SinCos( theta, st, ct );
      double ksxk = beta*Pow( Abs( sx ), beta );
      double ksyk = beta*Pow( Abs( sy ), beta );
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dy = y - h2y0;
         double dyst = dy*st;
         double dyct = dy*ct;
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
         {
            double dx = x - w2x0;
            double dy = dx*st + dyct;
            dx = dx*ct - dyst;
            *fvec++ = *s++ - B - A*Exp( -(Pow( Abs( dx ), beta )/ksxk + Pow( Abs( dy ), beta )/ksyk) );
         }
      }
      return 0;
   }

#undef F
#undef B
#undef A
#undef x0
#undef y0
#undef sx
#undef sy
#undef theta
#undef beta

   // -------------------------------------------------------------------------
   // Circular PSF Functions
   // -------------------------------------------------------------------------

#define F      reinterpret_cast<const PSFFit*>( p )
#define B      a[0]
#define A      a[1]
#define x0     a[2]
#define y0     a[3]
#define sx     a[4]
#define beta   a[5]

   /*
    * Circular Gaussian PSF.
    */
   static int FitCircularGaussian( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A*Exp( -((x - x0)^2 + (y - y0)^2)/(2*sigma^2) )
       */
      if ( B < 0 || A < 0 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      double twosx2 = 2*sx*sx;
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dy = y - h2y0;
         double dy2 = dy*dy;
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
         {
            double dx = x - w2x0;
            *fvec++ = *s++ - B - A*Exp( -(dx*dx + dy2)/twosx2 );
         }
      }
      return 0;
   }

   /*
    * Circular Moffat PSF, variable beta exponent.
    */
   static int FitCircularMoffat( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A/(1 + ((x - x0)^2 + (y - y0)^2)/sigma^2)^beta
       */
      if ( B < 0 || A < 0 || beta < 0 || beta > 10 || Abs( beta - F->m_beta )/F->m_beta > 0.05 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      F->m_beta = beta;

      double sx2 = sx*sx;
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dy = y - h2y0;
         double dy2 = dy*dy;
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
         {
            double dx = x - w2x0;
            *fvec++ = *s++ - B - A/Pow( 1 + (dx*dx + dy2)/sx2, beta );
         }
      }
      return 0;
   }

#undef beta
#define beta   F->P[5]

   /*
    * Circular Moffat PSF, prescribed beta exponent.
    */
   static int FitCircularMoffatWithFixedBeta( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A/(1 + ((x - x0)^2 + (y - y0)^2)/sigma^2)^beta
       */
      if ( B < 0 || A < 0 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      double sx2 = sx*sx;
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dy = y - h2y0;
         double dy2 = dy*dy;
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
         {
            double dx = x - w2x0;
            *fvec++ = *s++ - B - A/Pow( 1 + (dx*dx + dy2)/sx2, beta );
         }
      }
      return 0;
   }

   /*
    * Circular variable shape PSF.
    */
   static int FitCircularVShape( void* p, int m, int n, const double* __restrict__ a, double* __restrict__ fvec, int iflag )
   {
      /*
       * f(x,y) = B + A*Exp( -((x - x0)^k + (y - y0)^k)/(k*sigma^k) )
       */
      if ( B < 0 || A < 0 )
      {
         for ( int i = 0; i < m; ++i )
            *fvec++ = std::numeric_limits<double>::max();
         return 0;
      }

      double ksxk = beta*Pow( Abs( sx ), beta );
      int h = F->S.Rows();
      int w = F->S.Cols();
      double w2x0 = (w >> 1) + x0;
      double h2y0 = (h >> 1) + y0;
      const double* __restrict__ s = F->S.Begin();
      for ( int y = 0; y < h; ++y )
      {
         double dyk = Pow( Abs( y - h2y0 ), beta );
         PCL_IVDEP
         for ( int x = 0; x < w; ++x )
            *fvec++ = *s++ - B - A*Exp( -(Pow( Abs( x - w2x0 ), beta ) + dyk)/ksxk );
      }
      return 0;
   }

#undef F
#undef B
#undef A
#undef x0
#undef y0
#undef sx
#undef beta
}; // PSFFitEngine

// ----------------------------------------------------------------------------

void PSFData::ToImage( Image& image ) const
{
   // ### TODO
}

// ----------------------------------------------------------------------------

class PCL_VSFitThread : public pcl::Thread
{
public:

   Array<PSFFit> fits;

   PCL_VSFitThread( const ImageVariant& image, const DPoint& pos, const DRect& rect, bool circular,
                    double betaFirst, double betaLast, double betaStep )
      : m_image( image )
      , m_pos( pos )
      , m_rect( rect )
      , m_circular( circular )
      , m_betaFirst( betaFirst )
      , m_betaLast( betaLast )
      , m_betaStep( betaStep )
   {
   }

   PCL_HOT_FUNCTION void Run() override
   {
      for ( double beta = m_betaFirst; beta <= m_betaLast; beta += m_betaStep )
      {
         PSFFit F( m_image, m_pos, m_rect, PSFunction::VariableShape, m_circular, beta, beta );
         if ( F )
            fits << F;
      }
   }

private:

   const ImageVariant& m_image;
         DPoint        m_pos;
         DRect         m_rect;
         bool          m_circular;
         double        m_betaFirst, m_betaLast, m_betaStep;
};

typedef int (*cminpack_callback)( void*, int, int, const double*, double*, int );

PSFFit::PSFFit( const ImageVariant& image, const DPoint& pos, const DRect& rect,
                psf_function function, bool circular, double betaMin, double betaMax )
{
   if ( !image )
      return;

   if ( function == PSFunction::VariableShape )
   {
      betaMin = Range( betaMin, 0.5, 6.0 );
      betaMax = Range( betaMax, betaMin, 6.0 );

      if ( betaMin < betaMax )
      {
         /*
          * Fit for beta discretely in [betaMin, betaMin+delta, ..., betaMax]
          */
         Array<size_type> L = Thread::OptimalThreadLoads( 10, 1/*overheadLimit*/ );
         ReferenceArray<PCL_VSFitThread> threads;
         double betaStep = (betaMax - betaMin)/10;
         for ( int i = 0, n = 0; i < int( L.Length() ); n += int( L[i++] ) )
         {
            double betaFirst = betaMin + n*betaStep;
            double betaLast = betaMin + (n + int( L[i] ) - 1)*betaStep;
            threads << new PCL_VSFitThread( image, pos, rect, circular, betaFirst, betaLast, betaStep );
         }
         if ( threads.Length() > 1 )
         {
            for ( int i = 0; i < int( threads.Length() ); ++i )
               threads[i].Start( ThreadPriority::DefaultMax, i );
            for ( auto& thread : threads )
               thread.Wait();
         }
         else
            threads[0].Run();

         Array<PSFFit> fits;
         for ( auto& thread : threads )
            fits << thread.fits;
         threads.Destroy();

         /*
          * Find an optimal beta parameter value by interpolation using a
          * golden section search scheme. We search for the value of beta that
          * minimizes mean absolute deviation in the difference between the
          * computed PSF and the sampled image data.
          */
         if ( !fits.IsEmpty() )
         {
            int n = int( fits.Length() );
            if ( n == 1 ) // need n >= 2 for interpolation
            {
               psf = fits[0].psf;
               return;
            }

            Vector B( n ), M( n );
            double mmin = fits[0].psf.mad;
            int imin = 0;
            for ( int i = 0; i < n; ++i )
            {
               B[i] = fits[i].psf.beta;
               M[i] = fits[i].psf.mad;
               if ( M[i] < mmin )
               {
                  mmin = M[i];
                  imin = i;
               }
            }
            CubicSplineInterpolation S;
            S.Initialize( B, M );

            // The golden ratios.
            const double R = 1.61803398875;
            const double C = 1 - R;

            // Form an initial triplet (ax,bx,cx) that brackets the minimum.
            double ax = Max( betaMin, B[imin] - betaStep );
            double cx = Min( betaMax, B[imin] + betaStep );
            double bx = (ax + cx)/2;

            // [x0,x3] is the total search interval.
            double x0 = ax;
            double x3 = cx;

            // [x1,x2] is the inner search interval.
            // Start by sectioning the largest segment: (ax,bx) or (bx,cx).
            double x1, x2;
            if ( Abs( bx - ax ) < Abs( cx - bx ) )
            {
               x1 = bx;
               x2 = bx + C*(cx - bx);
            }
            else
            {
               x1 = bx - C*(bx - ax);
               x2 = bx;
            }

            // Start with MAD estimates at the inner interval boundaries.
            double f1 = S( x1 );
            double f2 = S( x2 );

            // Perform a golden section search for the beta parameter value
            // that minimizes mean absolute deviation.
            while ( Abs( x3 - x0 ) > 0.005 )
            {
               if ( f2 < f1 )
               {
                  x0 = x1;
                  x1 = x2;
                  x2 = R*x2 + C*x3;
                  f1 = f2;
                  f2 = S( x2 );
               }
               else
               {
                  x3 = x2;
                  x2 = x1;
                  x1 = R*x1 + C*x0;
                  f2 = f1;
                  f1 = S( x1 );
               }

               // Interpolation can try to go beyond the supported range in
               // search for a (theoretical) global minimum - that would keep
               // us iterating forever.
               if ( x0 < betaMin || x3 > betaMax )
                  break;
            }

            /*
             * Fit this PSF using the estimated optimal beta parameter value.
             */
            double beta = Range( (f1 < f2) ? x1 : x2, betaMin, betaMax );
            PSFFit F( image, pos, rect, PSFunction::VariableShape, circular, beta, beta );
            if ( F )
               psf = F.psf;
         }

         return;
      }
   }

   /*
    * Form the source sample matrix.
    */
   Rect r( TruncInt( rect.x0 ),   TruncInt( rect.y0 ),
           TruncInt( rect.x1 )+1, TruncInt( rect.y1 )+1 );
   image.Clip( r );
   S = Matrix::FromImage( image, r );
   int h = S.Rows();
   int w = S.Cols();

   /*
    * Center of the sampling region.
    */
   DPoint r0 = DRect( r ).Center();

   /*
    * Setup initial working parameters.
    */
   P = Vector( 8 );
   P[0] = (S.RowVector( 0 ).Median() +
           S.RowVector( h-1 ).Median() +
           S.ColVector( 0 ).Median() +
           S.ColVector( w-1 ).Median())/4;         // B
   P[1] = *MaxItem( S.Begin(), S.End() ) - P[0];   // A
   P[2] = pos.x - r0.x;                            // x0
   P[3] = pos.y - r0.y;                            // y0

   if ( circular )
      P[4] = 0.15*rect.Width();                    // sx
   else
   {
      P[4] = P[5] = 0.15*rect.Width();             // sx, sy
      P[6] = 0;                                    // theta
   }

   /*
    * Setup CMINPACK working parameters and data.
    */

   // Number of data points
   int m = int( S.NumberOfElements() );

   // Number of fitted parameters
   int n = (function == PSFunction::Moffat) ? 6 : 5;
   if ( !circular )
      n += 2; // sy, theta

   // Fitting function
   cminpack_callback fitFunc;
   if ( circular )
      switch ( function )
      {
      default:
      case PSFunction::Gaussian:
         fitFunc = PSFFitEngine::FitCircularGaussian;
         break;
      case PSFunction::Moffat:
         fitFunc = PSFFitEngine::FitCircularMoffat;
         break;
      case PSFunction::MoffatA:
      case PSFunction::Moffat8:
      case PSFunction::Moffat6:
      case PSFunction::Moffat4:
      case PSFunction::Moffat25:
      case PSFunction::Moffat15:
      case PSFunction::Lorentzian:
         fitFunc = PSFFitEngine::FitCircularMoffatWithFixedBeta;
         break;
      case PSFunction::VariableShape:
         fitFunc = PSFFitEngine::FitCircularVShape;
         break;
      }
   else
      switch ( function )
      {
      default:
      case PSFunction::Gaussian:
         fitFunc = PSFFitEngine::FitGaussian;
         break;
      case PSFunction::Moffat:
         fitFunc = PSFFitEngine::FitMoffat;
         break;
      case PSFunction::MoffatA:
      case PSFunction::Moffat8:
      case PSFunction::Moffat6:
      case PSFunction::Moffat4:
      case PSFunction::Moffat25:
      case PSFunction::Moffat15:
      case PSFunction::Lorentzian:
         fitFunc = PSFFitEngine::FitMoffatWithFixedBeta;
         break;
      case PSFunction::VariableShape:
         fitFunc = PSFFitEngine::FitVShape;
         break;
      }

   int ibeta = circular ? 5 : 7;
   switch ( function )
   {
   default:
   case PSFunction::Gaussian:
      break;
   case PSFunction::Moffat:
      m_beta = P[ibeta] = 3;
      break;
   case PSFunction::MoffatA:
      P[ibeta] = 10;
      break;
   case PSFunction::Moffat8:
      P[ibeta] = 8;
      break;
   case PSFunction::Moffat6:
      P[ibeta] = 6;
      break;
   case PSFunction::Moffat4:
      P[ibeta] = 4;
      break;
   case PSFunction::Moffat25:
      P[ibeta] = 2.5;
      break;
   case PSFunction::Moffat15:
      P[ibeta] = 1.5;
      break;
   case PSFunction::Lorentzian:
      P[ibeta] = 1;
      break;
   case PSFunction::VariableShape:
      P[ibeta] = betaMin;
      break;
   }

   /*
    * CMINPACK - Levenberg-Marquadt / finite differences.
    */
   {
      Vector fvec( m );
      IVector iwa( n );
      Vector wa( m*n + 5*n + m );
      int info = lmdif1( fitFunc,
                         this,
                         m,
                         n,
                         P.Begin(),
                         fvec.Begin(),
                         1.0e-08/*tol*/,
                         iwa.Begin(),
                         wa.Begin(),
                         wa.Length() );
      /*
       * Translate CMINPACK information codes into PSF fit status codes.
       */
      switch ( info )
      {
      case 0:  // improper input parameters
         psf.status = PSFFitStatus::BadParameters;
         break;
      case 1:  // The relative error in the sum of squares is at most tol
      case 2:  // The relative error between x and the solution is at most tol
      case 3:  // Conditions 1 and 2 both hold
         psf.status = PSFFitStatus::FittedOk;
         break;
      case 4:  // Function vector (fvec) is orthogonal to the columns of the Jacobian to machine precision
         psf.status = PSFFitStatus::NoSolution;
         break;
      case 5:  // Number of calls to fcn with iflag = 1 has reached 100*(n+1)
         psf.status = PSFFitStatus::NoConvergence;
         break;
      case 6:  // tol is too small. No further reduction in the sum of squares is possible
      case 7:  // tol is too small. No further improvement in the approximate solution x is possible
         psf.status = PSFFitStatus::InaccurateSolution;
         break;
      default:
         psf.status = PSFFitStatus::UnknownError;
         break;
      }
   }

   /*
    * First sanity check.
    */
   if ( psf )
      if (  !IsFinite( P[0] )                                  // B
         || !IsFinite( P[1] ) || 1 + P[1] == 1 || P[1] < 0     // A
         || !IsFinite( P[2] )                                  // x0
         || !IsFinite( P[3] )                                  // y0
         || !IsFinite( P[4] ) || 1 + P[4] == 1                 // sx
         || !circular && (!IsFinite( P[5] ) || 1 + P[5] == 1)  // sy
         || !circular && !IsFinite( P[6] )                     // theta
         || function != PSFunction::Gaussian && (!IsFinite( P[ibeta] ) || 1 + P[ibeta] == 1 || P[ibeta] < 0) )
      {
         psf = PSFData();
         psf.status = PSFFitStatus::Invalid;
      }

   /*
    * For Moffat functions with a variable beta parameter and variable shape
    * functions, a bad fit can go wildly unstable on this parameter, so we have
    * to impose a reasonable maximum value.
    */
   if ( psf )
      if ( function == PSFunction::Moffat || function == PSFunction::VariableShape )
         if ( P[ibeta] > 9.99 )
            psf.status = PSFFitStatus::NoConvergence;

   psf.function = function;
   psf.circular = circular;

   /*
    * Ensure valid PSF sizes. The L-M algorithm may choose negative and/or
    * unordered sigma parameters in a valid fit, which we must fix for
    * coherence with our advertised interface. We have to protect us also
    * against wildly large fitted function dimensions.
    */
   if ( psf )
   {
      // Ensure 0 < sx.
      P[4] = Abs( P[4] );
      if ( !circular )
      {
         // For elliptical functions, ensure 0 < sy < sx.
         P[5] = Abs( P[5] );
         if ( P[4] < P[5] )
            Swap( P[4], P[5] );
      }

      // Check if the fitted function wants to be larger than the sampling
      // region. Most likely this denotes a bad fit.
      if ( psf.FWHM( function, P[4], P[ibeta] ) > rect.Width() )
         psf.status = PSFFitStatus::NoConvergence;
   }

   if ( psf )
   {
      // Estimated local background. Must be >= 0.
      psf.B = Max( 0.0, P[0] );
      // Estimated function maximum.
      psf.A = P[1];
      // Centroid coordinates.
      psf.c0.x = P[2] + r0.x;
      psf.c0.y = P[3] + r0.y;

      if ( circular )
      {
         /*
          * Circular PSF (prescribed)
          */
         psf.sx = psf.sy = P[4];
         psf.theta = 0;
         Vector r = GoodnessOfFit( function, true/*circular*/ );
         psf.flux = r[2];
         psf.meanSignal = r[3];
         psf.mad = r[0];
      }
      else
      {
         psf.sx = P[4];
         psf.sy = P[5];

         if ( Abs( psf.sx - psf.sy ) < 0.01 )
         {
            /*
             * Circular PSF (incidental, to centipixel accuracy)
             */
            psf.theta = 0;
            Vector r = GoodnessOfFit( function, false/*circular*/ );
            psf.flux = r[2];
            psf.meanSignal = r[3];
            psf.mad = r[0];
         }
         else
         {
            /*
             * Elliptical PSF
             *
             * After L-M minimization the rotation angle cannot be determined
             * without uncertainty from the fitted parameters. We check the
             * four possibilities and select the angle that causes the minimum
             * absolute difference with the sampled matrix.
             */

            // Constrain theta to the first quadrant.
            psf.theta = P[6];
            psf.theta = ArcTan( Sin( psf.theta ), Cos( psf.theta ) );
            if ( psf.theta < 0 )
               psf.theta += Pi();
            if ( psf.theta > Pi()/2 )
               psf.theta -= Pi()/2;

            // There are four choices that we must check.
            Vector a( 4 );
            a[0] =          psf.theta;
            a[1] = Pi()/2 - psf.theta;
            a[2] = Pi()/2 + psf.theta;
            a[3] = Pi()   - psf.theta;

            // Generate the four models and compute absolute differences.
            P[6] = a[0];
            Vector r0 = GoodnessOfFit( function, false/*circular*/ );
            int imin = 0;
            for ( int i = 1; i < 4; ++i )
            {
               P[6] = a[i];
               Vector ri = GoodnessOfFit( function, false/*circular*/ );
               if ( ri[1] < r0[1] )
               {
                  imin = i;
                  r0 = ri;
               }
            }

            // Select the orientation angle that minimizes absolute deviation.
            psf.theta = Deg( a[imin] );
            psf.flux = r0[2];
            psf.meanSignal = r0[3];
            psf.mad = r0[0];
         }
      }

      // Moffat/VariableShape beta parameter.
      psf.beta = (function == PSFunction::Gaussian) ? 2.0 : P[ibeta];

      /*
       * Normalize mean absolute deviation with respect to the estimated mean
       * signal value.
       */
      if ( 1 + psf.meanSignal != 1 )
         psf.mad /= psf.meanSignal;
      else
         psf.status = PSFFitStatus::Invalid;
   }
}

// ----------------------------------------------------------------------------

/*
 * Robust estimates of mean absolute difference and total flux, measured from
 * sampled pixel data and the fitted PSF model.
 */
Vector PSFFit::GoodnessOfFit( psf_function function, bool circular ) const
{
#define B      P[0]
#define A      P[1]
#define x0     P[2]
#define y0     P[3]
#define sx     P[4]

   int w = S.Cols();
   int h = S.Rows();
   double w2x0 = (w >> 1) + x0;
   double h2y0 = (h >> 1) + y0;
   const double* s = S.Begin();

   Vector adev( w*h );
   double flux = 0;
   double zsum = 0;

   if ( circular )
   {
#define beta   P[5]
      switch ( function )
      {
      default:
      case PSFunction::Gaussian:
         {
            double twosx2 = 2*sx*sx;
            for ( int y = 0, i = 0; y < h; ++y )
            {
               double dy  = y - h2y0;
               double dy2 = dy*dy;
               for ( int x = 0; x < w; ++x, ++i, ++s )
               {
                  double dx = x - w2x0;
                  double z = Exp( -(dx*dx + dy2)/twosx2 );
                  adev[i] = Abs( *s - B - A*z );
                  if ( *s > B )
                  {
                     flux += (*s - B)*z;
                     zsum += z;
                  }
               }
            }
         }
         break;

      case PSFunction::Moffat:
      case PSFunction::MoffatA:
      case PSFunction::Moffat8:
      case PSFunction::Moffat6:
      case PSFunction::Moffat4:
      case PSFunction::Moffat25:
      case PSFunction::Moffat15:
      case PSFunction::Lorentzian:
         {
            double sx2 = sx*sx;
            for ( int y = 0, i = 0; y < h; ++y )
            {
               double dy  = y - h2y0;
               double dy2 = dy*dy;
               for ( int x = 0; x < w; ++x, ++i, ++s )
               {
                  double dx = x - w2x0;
                  double z = 1/Pow( 1 + (dx*dx + dy2)/sx2, beta );
                  adev[i] = Abs( *s - B - A*z );
                  if ( *s > B )
                  {
                     flux += (*s - B)*z;
                     zsum += z;
                  }
               }
            }
         }
         break;

      case PSFunction::VariableShape:
         {
            double ksxk = beta*Pow( Abs( sx ), beta );
            for ( int y = 0, i = 0; y < h; ++y )
            {
               double dyk = Pow( Abs( y - h2y0 ), beta );
               for ( int x = 0; x < w; ++x, ++i, ++s )
               {
                  double z = Exp( -(Pow( Abs( x - w2x0 ), beta ) + dyk)/ksxk );
                  adev[i] = Abs( *s - B - A*z );
                  if ( *s > B )
                  {
                     flux += (*s - B)*z;
                     zsum += z;
                  }
               }
            }
         }
         break;
      }
#undef beta
   }
   else
   {
#define sy     P[5]
#define theta  P[6]
#define beta   P[7]
      switch ( function )
      {
      default:
      case PSFunction::Gaussian:
         {
            double st, ct;
            SinCos( theta, st, ct );
            double sct    = st*ct;
            double st2    = st*st;
            double ct2    = ct*ct;
            double twosx2 = 2*sx*sx;
            double twosy2 = 2*sy*sy;
            double p1     = ct2/twosx2 + st2/twosy2;
            double p2     = sct/twosy2 - sct/twosx2;
            double p3     = st2/twosx2 + ct2/twosy2;
            for ( int y = 0, i = 0; y < h; ++y )
            {
               double dy      = y - h2y0;
               double twop2dy = 2*p2*dy;
               double p3dy2   = p3*dy*dy;
               for ( int x = 0; x < w; ++x, ++i, ++s )
               {
                  double dx = x - w2x0;
                  double z = Exp( -(p1*dx*dx + twop2dy*dx + p3dy2) );
                  adev[i] = Abs( *s - B - A*z );
                  if ( *s > B )
                  {
                     flux += (*s - B)*z;
                     zsum += z;
                  }
               }
            }
         }
         break;

      case PSFunction::Moffat:
      case PSFunction::MoffatA:
      case PSFunction::Moffat8:
      case PSFunction::Moffat6:
      case PSFunction::Moffat4:
      case PSFunction::Moffat25:
      case PSFunction::Moffat15:
      case PSFunction::Lorentzian:
         {
            double st, ct;
            SinCos( theta, st, ct );
            double sct = st*ct;
            double st2 = st*st;
            double ct2 = ct*ct;
            double sx2 = sx*sx;
            double sy2 = sy*sy;
            double p1  = ct2/sx2 + st2/sy2;
            double p2  = sct/sy2 - sct/sx2;
            double p3  = st2/sx2 + ct2/sy2;
            for ( int y = 0, i = 0; y < h; ++y )
            {
               double dy      = y - h2y0;
               double twop2dy = 2*p2*dy;
               double p3dy2   = p3*dy*dy;
               for ( int x = 0; x < w; ++x, ++i, ++s )
               {
                  double dx = x - w2x0;
                  double z = 1/Pow( 1 + p1*dx*dx + twop2dy*dx + p3dy2, beta );
                  adev[i] = Abs( *s - B - A*z );
                  if ( *s > B )
                  {
                     flux += (*s - B)*z;
                     zsum += z;
                  }
               }
            }
         }
         break;

      case PSFunction::VariableShape:
         {
            double st, ct;
            SinCos( theta, st, ct );
            double ksxk = beta*Pow( Abs( sx ), beta );
            double ksyk = beta*Pow( Abs( sy ), beta );
            for ( int y = 0, i = 0; y < h; ++y )
            {
               double dy   = y - h2y0;
               double dyst = dy*st;
               double dyct = dy*ct;
               for ( int x = 0; x < w; ++x, ++i, ++s )
               {
                  double dx = x - w2x0;
                  double dy = dx*st + dyct;
                  dx = dx*ct - dyst;
                  double z = Exp( -(Pow( Abs( dx ), beta )/ksxk + Pow( Abs( dy ), beta )/ksyk) );
                  adev[i] = Abs( *s - B - A*z );
                  if ( *s > B )
                  {
                     flux += (*s - B)*z;
                     zsum += z;
                  }
               }
            }
         }
         break;
      }
#undef sy
#undef theta
#undef beta
   }

#undef B
#undef A
#undef x0
#undef y0
#undef sx

   Vector R( 4 );

   /*
    * The fourth component of the returned vector is the estimated mean signal
    * value.
    */
   R[3] = (1 + zsum != 1) ? flux/zsum : 0.0;

   /*
    * The third component of the returned vector is the total flux above the
    * local background level, measured from source pixel data.
    */
   R[2] = flux;

   /*
    * The second component of the returned vector is the average absolute
    * deviation for internal use, e.g. to solve ambiguity in the quadrant of
    * the fitted rotation angle.
    */
   R[1] = adev.Mean();

   /*
    * The first returned component is a robust estimate of fitting quality.
    * Here we need an estimator of location with a good balance between
    * robustness and efficiency/sufficiency for the vector of absolute
    * differences. We use a mean with median replacement for a 10% of the
    * sample tails.
    */
   int n = adev.Length();
   int i0 = TruncInt( 0.1*n );
   int i1 = n - i0;
   adev.Sort();
   double m = adev[n >> 1];
   for ( int i = 0; i < i0; ++i )
      adev[i] = m;
   for ( int i = i1; i < n; ++i )
      adev[i] = m;
   R[0] = adev.Mean();

   return R;
}

// ----------------------------------------------------------------------------

String PSFData::FunctionName() const
{
   switch ( function )
   {
   case PSFunction::Gaussian:      return "Gaussian";
   case PSFunction::Moffat:        return "Moffat";
   case PSFunction::MoffatA:       return "Moffat10";
   case PSFunction::Moffat8:       return "Moffat8";
   case PSFunction::Moffat6:       return "Moffat6";
   case PSFunction::Moffat4:       return "Moffat4";
   case PSFunction::Moffat25:      return "Moffat25";
   case PSFunction::Moffat15:      return "Moffat15";
   case PSFunction::Lorentzian:    return "Lorentzian";
   case PSFunction::VariableShape: return "VarShape";
   default:                        return "Unknown"; // ?!
   }
}

String PSFData::StatusText() const
{
   switch ( status )
   {
   case PSFFitStatus::NotFitted:          return "Not fitted";
   case PSFFitStatus::FittedOk:           return "Fitted Ok";
   case PSFFitStatus::BadParameters:      return "Bad parameters";
   case PSFFitStatus::NoSolution:         return "No solution";
   case PSFFitStatus::NoConvergence:      return "No convergence";
   case PSFFitStatus::InaccurateSolution: return "Inaccurate solution";
   default: // ?!
   case PSFFitStatus::UnknownError:       return "Unknown error";
   }
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/PSFFit.cpp - Released 2020-12-17T15:46:35Z
