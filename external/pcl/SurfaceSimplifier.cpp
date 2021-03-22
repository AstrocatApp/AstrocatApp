//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/SurfaceSimplifier.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/SurfaceSimplifier.h>

namespace pcl
{

// ----------------------------------------------------------------------------

#define _n_ 3

/*
 * Reduces a real symmetric matrix to a symmetric tridiagonal matrix using and
 * accumulating orthogonal similarity transformations.
 *
 * Based on EISPACK/C++ and JAMA libraries.
 */
static void tred2( Matrix& E, Vector& d, Vector& e )
{
//    int _n_ = e.Length();
   for ( int j = 0; j < _n_; ++j )
      d[j] = E[_n_-1][j];

   for ( int i = _n_-1; i > 0; --i )
   {
      /*
       * Scale row.
       */
      double scale = 0;
      double h = 0;
      for ( int k = 0; k < i; ++k )
         scale += Abs( d[k] );
      if ( scale == 0 )
      {
         e[i] = d[i-1];
         for ( int j = 0; j < i; ++j )
         {
            d[j] = E[i-1][j];
            E[i][j] = E[j][i] = 0;
         }
      }
      else
      {
         for ( int k = 0; k < i; ++k )
         {
            d[k] /= scale;
            h += d[k]*d[k];
         }
         double f = d[i-1];
         double g = Sqrt( h );
         if ( f > 0 )
            g = -g;
         e[i] = scale*g;
         h -= f*g;
         d[i-1] = f - g;

         /*
          * Form A*U.
          */
         for ( int j = 0; j < i; ++j )
            e[j] = 0;

         // Apply similarity transformation to remaining columns.
         for ( int j = 0; j < i; ++j )
         {
            f = d[j];
            E[j][i] = f;
            g = e[j] + E[j][j]*f;
            for ( int k = j+1; k <= i-1; ++k )
            {
               g += E[k][j]*d[k];
               e[k] += E[k][j]*f;
            }
            e[j] = g;
         }

         /*
          * Form P.
          */
         f = 0;
         for ( int j = 0; j < i; ++j )
         {
            e[j] /= h;
            f += e[j]*d[j];
         }

         /*
          * Form Q.
          */
         double hh = f/(h + h);
         for ( int j = 0; j < i; ++j )
            e[j] -= hh*d[j];

         /*
          * Form reduced A.
          */
         for ( int j = 0; j < i; ++j )
         {
            f = d[j];
            g = e[j];
            for ( int k = j; k <= i-1; ++k )
               E[k][j] -= f*e[k] + g*d[k];
            d[j] = E[i-1][j];
            E[i][j] = 0;
         }
      }

      d[i] = h;
   }

   /*
    * Accumulation of transformation matrices.
    */
   for ( int i = 0; i < _n_-1; ++i )
   {
      E[_n_-1][i] = E[i][i];
      E[i][i] = 1;
      double h = d[i+1];
      if ( h != 0 )
      {
         for (int k = 0; k <= i; ++k )
            d[k] = E[k][i+1]/h;
         for ( int j = 0; j <= i; ++j )
         {
            double g = 0;
            for ( int k = 0; k <= i; ++k )
               g += E[k][i+1]*E[k][j];
            for ( int k = 0; k <= i; ++k )
               E[k][j] -= g*d[k];
         }
      }
      for ( int k = 0; k <= i; ++k )
         E[k][i+1] = 0;
   }
   for ( int j = 0; j < _n_; ++j )
   {
      d[j] = E[_n_-1][j];
      E[_n_-1][j] = 0;
   }
   E[_n_-1][_n_-1] = 1;
   e[0] = 0;
}

// ----------------------------------------------------------------------------

/*
 * Find the eigenvalues and eigenvectors of a symmetric tridiagonal matrix by
 * the QL method.
 *
 * Based on EISPACK/C++ and JAMA libraries.
 */
static int tql2( Matrix& E, Vector& d, Vector& e )
{
//    int _n_ = e.Length();
   for ( int i = 1; i < _n_; ++i )
      e[i-1] = e[i];
   e[_n_-1] = 0;

   double f = 0;
   double tst1 = 0;
   double eps = pow( 2.0, -52.0 );
   for ( int l = 0; l < _n_; ++l )
   {
      /*
       * Look for a small sub-diagonal element.
       */
      tst1 = Max( tst1, Abs( d[l] ) + Abs( e[l] ) );
      int m = l;
      for ( ; m < _n_ && Abs( e[m] ) > eps*tst1; ++m ) {}
      // If m == l, d[l] is an eigenvalue; otherwise, iterate.
      if ( m != l )
         for ( int it = 0; ; ++it )
         {
            /*
             * Check for iteration limit.
             */
            if ( it > 30 )
               return l + 1;

            /*
             * Form shift.
             */
            double g = d[l];
            double p = (d[l+1] - g)/(2*e[l]);
            double r = Sqrt( p*p + 1 );
            if ( p < 0 )
               r = -r;
            d[l] = e[l]/(p + r);
            d[l+1] = e[l]*(p + r);
            double dl1 = d[l+1];
            double h = g - d[l];
            for ( int i = l+2; i < _n_; ++i )
               d[i] -= h;
            f += h;

            /*
             * QL transformation.
             */
            p = d[m];
            double c = 1;
            double c2 = c;
            double c3 = c;
            double el1 = e[l+1];
            double s = 0;
            double s2 = 0;
            for ( int i = m-1; i >= l; --i )
            {
               c3 = c2;
               c2 = c;
               s2 = s;
               g = c*e[i];
               h = c*p;
               r = Sqrt( p*p + e[i]*e[i] );
               e[i+1] = s*r;
               s = e[i]/r;
               c = p/r;
               p = c*d[i] - s*g;
               d[i+1] = h + s*(c*g + s*d[i]);

               /*
                * Form vector.
                */
               for ( int k = 0; k < _n_; ++k )
               {
                  h = E[k][i+1];
                  E[k][i+1] = s*E[k][i] + c*h;
                  E[k][i] = c*E[k][i] - s*h;
               }
            }
            p = -s*s2*c3*el1*e[l]/dl1;
            e[l] = s*p;
            d[l] = c*p;

            if ( Abs( e[l] ) <= eps*tst1 )
               break;
         }

      d[l] += f;
      e[l] = 0;
   }

   /*
    * Order eigenvalues and eigenvectors.
    */
   for ( int i = 0; i < _n_-1; ++i )
   {
      int k = i;
      double p = d[i];
      for ( int j = i+1; j < _n_; ++j )
         if ( d[j] < p )
         {
            k = j;
            p = d[j];
         }
      if ( k != i )
      {
         d[k] = d[i];
         d[i] = p;
         for ( int j = 0; j < _n_; ++j )
            Swap( E[j][i], E[j][k] );
      }
   }

   return 0;
}

// ----------------------------------------------------------------------------

void SurfaceSimplifier::ComputeEigenvectors( Matrix& E )
{
   PCL_CHECK( E.Rows() == 3 && E.Cols() == 3 )
   // Eigenvalues, which we don't need in this implementation.
   Vector d( _n_/*E.Rows()*/ );
   // Working space.
   Vector e( _n_/*E.Rows()*/ );
   tred2( E, d, e );
   tql2( E, d, e );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

/*
 * The Graham Scan algorithm to find the convex hull of a finite set of points
 * on the plane. The algorithm has O(NlogN) time complexity.
 *
 * Based on GeeksforGeeks's implementation:
 * https://www.geeksforgeeks.org/convex-hull-set-2-graham-scan/
 */
template <class P>
struct GrahamScan
{
   typedef P         point;

   typedef Array<P>  point_list;

   struct PointStack : public point_list
   {
      void Push( const point& p )
      {
         this->Append( p );
      }

      void Pop()
      {
         this->Shrink();
      }

      const point& Top() const
      {
         return *(this->End() - 1);
      }

      const point& NextToTop() const
      {
         return *(this->End() - 2);
      }
   };

   enum
   {
      COLLINEAR,
      CLOCKWISE,
      COUNTER_CLOCKWISE
   };

   /*
    * Relative orientation of three contiguous points:
    * COLLINEAR         : if p, q, r are collinear to within machine epsilon.
    * CLOCKWISE         : if q->r makes a clockwise turn w.r.t. p->q
    * COUNTER_CLOCKWISE : if q->r makes a counter-clockwise turn w.r.t. p->q
    */
   static int Orientation( const point& p, const point& q, const point& r )
   {
      double val = (q.y - p.y)*(r.x - q.x) - (q.x - p.x)*(r.y - q.y);
      if ( val < 0 )
         return (val < -std::numeric_limits<double>::epsilon()) ? COUNTER_CLOCKWISE : COLLINEAR;
      return (val > std::numeric_limits<double>::epsilon()) ? CLOCKWISE : COLLINEAR;
   }

   /*
    * Returns the convex hull of the specified point list, or the end points if
    * all points are collinear to within the machine epsilon for double.
    */
   point_list operator()( point_list& points ) const
   {
      int n = int( points.Length() );
      if ( n < 3 )
         return points;

      /*
       * Find the bottom-most point.
       */
      int imin = 0;
      for ( int i = 1, ymin = points[0].y; i < n; ++i )
      {
         int y = points[i].y;
         // Pick the bottom-most or chose the leftmost point in case of tie.
         if ( y < ymin || y == ymin && points[i].x < points[imin].x )
            ymin = y, imin = i;
      }

      /*
       * Place the bottom-most point at first position.
       */
      Swap( points[0], points[imin] );

      /*
       * Sort n-1 points with respect to the first point. A point p1 precedes
       * p2 in the sorted list if p2 has larger polar angle, in the
       * counter-clockwise direction, than p1 with respect to p0.
       */
      point p0 = points[0];
      QuickSort( points.At( 1 ), points.End(),
         [&p0]( const point& p1, const point& p2 )
         {
            int o = Orientation( p0, p1, p2 );
            if ( o == COLLINEAR )
               return p0.SquaredDistanceTo( p2 ) >= p0.SquaredDistanceTo( p1 );
            return o == COUNTER_CLOCKWISE;
         } );

      /*
       * Remove any contiguous sequences of collinear points.
       */
      int m = 1;
      for ( int i = 1; i < n; ++i, ++m )
      {
         for ( ; i < n-1; ++i )
            if ( Orientation( p0, points[i], points[i+1] ) != COLLINEAR )
               break;
         points[m] = points[i];
      }

      /*
       * If all points are collinear (to within machine epsilon), return just
       * the end points of the whole segment. This may be questionable
       * regarding shape preservation, but we are dealing with marginal or
       * degenerate cases here.
       */
      if ( m < 3 )
         return point_list() << p0 << points[1];

      /*
       * Create an empty stack and push the first three points to it.
       */
      PointStack S;
      S.Push( points[0] );
      S.Push( points[1] );
      S.Push( points[2] );

      /*
       * Process the remaining m-3 points.
       */
      for ( int i = 3; i < m; ++i )
      {
         /*
          * Keep removing top while the angle formed by points next-to-top,
          * top, and points[i] makes a non-left turn.
          */
         while ( Orientation( S.NextToTop(), S.Top(), points[i] ) != COUNTER_CLOCKWISE )
            S.Pop();
         S.Push( points[i] );
      }

      return std::move( S );
   }
};

// ----------------------------------------------------------------------------

SurfaceSimplifier::point_list SurfaceSimplifier::ConvexHull( point_list& P )
{
   return GrahamScan<point>()( P );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/SurfaceSimplifier.cpp - Released 2020-12-17T15:46:35Z
