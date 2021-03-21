//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/SurfaceSimplifier.h - Released 2020-12-17T15:46:28Z
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

#ifndef __PCL_SurfaceSimplifier_h
#define __PCL_SurfaceSimplifier_h

/// \file pcl/SurfaceSimplifier.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/Matrix.h>
#include <pcl/QuadTree.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*
 * Default tolerance for surface simplification.
 *
 * The tolerance parameter determines the maximum absolute deviation of
 * function values from a plane allowed to simplify the subset of points within
 * a subregion of the input point space. This parameter is specified in
 * bivariate function values, i.e. in Z-axis units.
 */
#define __PCL_SURFACE_SIMPLIFIER_DEFAULT_TOLERANCE          0.01

/*
 * Default state of the surface simplification outlier rejection feature.
 *
 * When enabled, a prescribed fraction of outlier points will be rejected on
 * each subregion for estimation of local curvature.
 */
#define __PCL_SURFACE_SIMPLIFIER_DEFAULT_REJECTION_ENABLED  true

/*
 * Default rejection fraction for surface simplification.
 *
 * This parameter specifies the maximum fraction of outliers allowed for
 * simplification of a subset of points in a region of the input point space.
 * Point rejection makes the surface simplification algorithm robust to outlier
 * function values.
 */
#define __PCL_SURFACE_SIMPLIFIER_DEFAULT_REJECT_FRACTION    0.2F

/*
 * Whether to include subregion centroids in simplified point sets.
 *
 * When enabled, the centroid of each simplified subregion will also be
 * included in the corresponding list of simplified points. This can improve
 * the shape preservation behavior of the surface simplification algorithm, at
 * the cost of a reduced amount of additional points.
 */
#define __PCL_SURFACE_SIMPLIFIER_DEFAULT_INCLUDE_CENTROIDS  false

// ----------------------------------------------------------------------------

/*!
 * \class SurfaceSimplifier
 * \brief Shape-preserving simplification of 2-D surfaces
 *
 * Given a finite set of three dimensional points representing sampled values
 * of a real bivariate function of the form
 *
 * z = f(x,y),
 *
 * the shape-preserving surface simplification algorithm attempts to generate a
 * reduced set of points with equivalent geometric properties to within a
 * prescribed maximum error parameter.
 *
 * The implemented algorithm divides the input point space recursively on the
 * XY plane into rectangular regions using custom quadtree structures. For each
 * region, the algorithm finds the orientation of the dominant plane through
 * principal component analysis. The deviation of function values from the
 * dominant plane is evaluated for the points in the region, and if the region
 * is considered flat to within a tolerance parameter, its points are replaced
 * with a simplified (reduced) set of points that tends to preserve the local
 * shape of the original function over the region. If the region is tagged as
 * curve, it is further divided using a new quadtree recursion, until no
 * additional simplification can be achieved.
 *
 * Surface simplification is an important auxiliary tool to improve the
 * practical application of surface interpolation and approximation devices.
 * These algorithms allow us to work with large-scale data sets by selecting a
 * subset of essential data points, usually much smaller than the original set,
 * adapted to solve a particular problem. Surface simplification is
 * particularly useful for the application of computationally expensive
 * approximation algorithms, such as surface splines or thin plates. A good
 * example is computation of high accuracy astrometric solutions, where surface
 * simplification allows us to use large sets of thousands of stars to generate
 * thin plate models of local distortions. Since generation of thin plates has
 * roughly O(N^3) time complexity, the efficient reduction of input point sets
 * is crucial for this application.
 */
class PCL_CLASS SurfaceSimplifier
{
private:

   /*!
    * \struct SurfaceSimplifier::point
    * \brief Working 3-D point structure for quadtree storage used by the
    * recursive surface simplification algorithm.
    * \internal
    */
   struct point
   {
      typedef double component;

      component x, y, z;

      point() = default;
      point( const point& ) = default;

      template <typename T>
      point( T a_x, T a_y, T a_z )
         : x( double( a_x ) )
         , y( double( a_y ) )
         , z( double( a_z ) )
      {
      }

      template <typename T>
      point( T k )
      {
         x = y = z = double( k );
      }

      component operator[]( int i ) const
      {
         return i ? y : x;
      }

      point& operator +=( const point& p )
      {
         x += p.x; y += p.y; z += p.z;
         return *this;
      }

      template <typename T>
      point& operator /=( T k )
      {
         x /= k; y /= k; z /= k;
         return *this;
      }

      double SquaredDistanceTo( const point& p ) const
      {
         double dx = p.x - x, dy = p.y - y;
         return dx*dx + dy*dy;
      }
   };

   typedef QuadTree<point>             tree;

   typedef typename tree::rectangle    rectangle;

   typedef typename tree::point_list   point_list;

public:

   /*!
    * Constructs a new %SurfaceSimplifier object with default parameters:
    *
    * \li Tolerance = 0.01 in function value units (Z-axis values).
    * \li Outlier rejection enabled
    * \li Outlier rejection fraction = 0.2
    * \li Inclusion of centroid points disabled
    */
   SurfaceSimplifier() = default;

   /*!
    * Constructs a new %SurfaceSimplifier instance with the specified
    * \a tolerance.
    *
    *
    */
   SurfaceSimplifier( double tolerance )
      : m_tolerance( Abs( tolerance ) )
   {
      PCL_PRECONDITION( tolerance >= 0 )
   }

   /*!
    * Copy constructor.
    */
   SurfaceSimplifier( const SurfaceSimplifier& ) = default;

   /*!
    * Returns the current tolerance of this surface simplifier.
    *
    * See the SetTolerance() member function for a description of the
    * tolerance parameter.
    */
   double Tolerance() const
   {
      return m_tolerance;
   }

   /*!
    * Sets the \a tolerance of this surface simplifier.
    *
    * The tolerance parameter determines the maximum absolute deviation of
    * function values from a plane allowed to simplify the subset of points
    * within a subregion of the input point space.
    *
    * The value of this parameter is specified in bivariate function value
    * units, i.e. in Z-axis units. Higher tolerances tend to allow for more
    * simplification, and hence for shorter simplified point lists. However, an
    * excessive tolerance value may degrade the accuracy of the simplified
    * surface in terms of preservation of the original function's structure and
    * shape. This parameter must be tailored carefully to the requirements of
    * the function being simplified and the tasks where the simplified version
    * is going to be applied.
    */
   void SetTolerance( double tolerance )
   {
      PCL_PRECONDITION( tolerance > 0 )
      m_tolerance = Abs( tolerance );
   }

   /*!
    * Returns true iff outlier rejection is enabled for this object.
    *
    * See the EnableRejection() and RejectFraction() member functions for a
    * description of outlier rejection in the surface simplification algorithm.
    */
   bool IsRejectionEnabled() const
   {
      return m_enableRejection;
   }

   /*!
    * Enables outlier rejection for this surface simplifier.
    *
    * When enabled, a prescribed fraction of outlier points (see the
    * RejectFraction() member function) will be rejected on each subregion for
    * estimation of local curvature. An adequate amount of rejection is
    * important to achieve a robust result, especially for simplification of
    * noisy data where outliers may generate false curvatures that prevent
    * efficient simplification.
    */
   void EnableRejection( bool enabled = true )
   {
      m_enableRejection = enabled;
   }

   /*!
    * Disables outlier rejection for this surface simplifier. See
    * EnableRejection() for more information.
    */
   void DisableRejection( bool disable = true )
   {
      EnableRejection( !disable );
   }

   /*!
    * Returns the fraction of outlier points rejected for estimation of local
    * curvature.
    *
    * See the EnableRejection() and RejectFraction() member functions for a
    * description of outlier rejection in the surface simplification algorithm.
    */
   float RejectFraction() const
   {
      return m_rejectFraction;
   }

   /*!
    * Sets the fraction of outlier points rejected by this surface simplifier.
    *
    * This parameter defines a fraction of outlier points that will be rejected
    * on each subregion of the point space being simplified, for estimation of
    * local curvature.
    *
    * Rejecting an adequate fraction of points makes the surface simplification
    * algorithm more immune to noise in the input data, including erroneous
    * points that may deviate considerably from the true surface represented by
    * the sampled function. The result is a more robust and accurate simplified
    * surface. However, too high of a rejection fraction may remove significant
    * data and lead to an inaccurate result.
    *
    * The specified \a rejectFraction value must be in the (0,1) range. The
    * default value upon construction is 0.2, which is quite appropriate in
    * most cases.
    */
   void SetRejectFraction( float rejectFraction )
   {
      PCL_PRECONDITION( rejectFraction > 0 && rejectFraction < 1 )
      m_rejectFraction = Range( rejectFraction, 0.0F, 1.0F );
   }

   /*!
    * Returns true iff inclusion of centroid points is enabled for this object.
    *
    * See the EnableCentroidInclusion() member function for a description of
    * the centroid inclusion feature of the surface simplification algorithm.
    */
   bool IsCentroidInclusionEnabled() const
   {
      return m_includeCentroids;
   }

   /*!
    * Enables inclusion of centroid points for this surface simplifier.
    *
    * When a subregion of the input space is simplified, the surface
    * simplification algorithm replaces the subset of points in the subregion
    * with a simplified, reduced set. If this option is enabled, the average
    * point of the subset, also known as \e centroid, is also included in the
    * simplified point list. This usually improves the shape preservation
    * behavior of the algorithm, at the cost of a small amount of additional
    * points in the simplified point list.
    */
   void EnableCentroidInclusion( bool enable = true )
   {
      m_includeCentroids = enable;
   }

   /*!
    * Disables inclusion of centroid points for this surface simplifier. See
    * EnableCentroidInclusion() for more information.
    */
   void DisableCentroidInclusion( bool disable = true )
   {
      EnableCentroidInclusion( !disable );
   }

   /*!
    * Attempts to simplify a set of points given by its separate coordinates
    * and function values.
    *
    * \param[out] xs    The X coordinates of the simplified point set.
    *
    * \param[out] ys    The Y coordinates of the simplified point set.
    *
    * \param[out] zs    The function values for the simplified point set.
    *
    * \param x          The X coordinates of the input point set.
    *
    * \param y          The Y coordinates of the input point set.
    *
    * \param z          The function values for the input point set.
    *
    * The template argument C must be a direct container of numeric scalars
    * with random access semantics, such as Array or GenericVector.
    *
    * If the specified surface can be simplified with the current working
    * parameters defined for this object, the output containers will have less
    * elements (usually \e much less) than the input containers. Otherwise an
    * exact copy of the input containers will be obtained in \a xs, \a ys and
    * \a zs. This will happen also if the input containers have less than four
    * coordinates, since a triangular facet cannot be simplified.
    */
   template <class C>
   void Simplify( C& xs, C& ys, C& zs, const C& x, const C& y, const C& z ) const
   {
      int n = Min( Min( x.Length(), y.Length() ), z.Length() );
      if ( n < 4 )
      {
         xs = x; ys = y; zs = z;
         return;
      }

      point_list P;
      for ( int i = 0; i < n; ++i )
         P << point( x[i], y[i], z[i] );

      tree R( P, n );
      P = Simplify( R );
      if ( int( P.Length() ) >= n )
      {
         xs = x; ys = y; zs = z;
         return;
      }

      R.Build( P, n );
      P = Simplify( R );

      n = int( P.Length() );
      xs = C( n );
      ys = C( n );
      zs = C( n );
      for ( int i = 0; i < n; ++i )
      {
         xs[i] = typename C::item_type( P[i].x );
         ys[i] = typename C::item_type( P[i].y );
         zs[i] = typename C::item_type( P[i].z );
      }
   }

private:

   double m_tolerance = __PCL_SURFACE_SIMPLIFIER_DEFAULT_TOLERANCE;
   float  m_rejectFraction = __PCL_SURFACE_SIMPLIFIER_DEFAULT_REJECT_FRACTION;
   bool   m_enableRejection = __PCL_SURFACE_SIMPLIFIER_DEFAULT_REJECTION_ENABLED;
   bool   m_includeCentroids = __PCL_SURFACE_SIMPLIFIER_DEFAULT_INCLUDE_CENTROIDS;

   /*!
    * Recursive part of the shape-preserving surface simplification algorithm.
    * Returns the simplified point list for the set of points stored in the
    * local quadtree \a R.
    * \internal
    */
   point_list Simplify( tree& R ) const
   {
      point_list Q;
      R.Traverse(
         [&Q,this]( const rectangle& rect, point_list& points, void*& )
         {
            int n = int( points.Length() );
            if ( n < 4 ) // cannot simplify triangles
            {
               Q << points;
               return;
            }

            /*
             * Compute local centroid coordinates.
             */
            point p0( 0 );
            for ( const point& p : points )
               p0 += p;
            p0 /= n;

            /*
             * Form the covariance matrix.
             */
            double xx = 0, xy = 0, xz = 0, yy = 0, yz = 0, zz = 0;
            for ( const point& p : points )
            {
               double dx = p.x - p0.x;
               double dy = p.y - p0.y;
               double dz = p.z - p0.z;
               xx += dx*dx;
               xy += dx*dy;
               xz += dx*dz;
               yy += dy*dy;
               yz += dy*dz;
               zz += dz*dz;
            }
            int n1 = n - 1;
            xx /= n1;
            xy /= n1;
            xz /= n1;
            yy /= n1;
            yz /= n1;
            zz /= n1;
            Matrix M( xx, xy, xz,
                      xy, yy, yz,
                      xz, yz, zz );

            /*
             * Test all local function values against the plane fitted at the
             * centroid, with optional outlier rejection. The plane normal
             * vector is the least eigenvector, from which we can form the
             * plane equation: ax + by + cz = 0.
             */
            ComputeEigenvectors( M );
            double a = M[0][0];
            double b = M[1][0];
            double c = M[2][0];
            int mr = m_enableRejection ? Max( 1, TruncInt( m_rejectFraction*n ) ) : 1; // max. number of outliers
            int nr = 0;
            point_list P = points;
            for ( point& p : P )
            {
               double z = p0.z - (a*(p.x - p0.x) + b*(p.y - p0.y))/c;
               if ( Abs( p.z - z ) > m_tolerance )
               {
                  if ( ++nr == mr )
                  {
                     /*
                      * If the current region deviates from the fitted plane
                      * more than allowed by the tolerance parameter after
                      * outlier rejection, try to simplify it further with a
                      * deeper quadtree subdivision.
                      */
                     tree R( points, TruncInt( 0.75*n ) );
                     Q << Simplify( R );
                     return;
                  }

                  // Winsorize outlier function values.
                  p.z = (p.z > z) ? z + m_tolerance : z - m_tolerance;
               }
            }

            /*
             * If the current region is flat to within the tolerance parameter,
             * take the convex hull as its simplified point set. This is what
             * makes our simplification algorithm shape-preserving.
             */
            if ( m_includeCentroids )
            {
               // If one or more points have been rejected, calculate a new
               // centroid function value from Winsorized z coordinates.
               if ( nr > 0 )
               {
                  p0.z = 0;
                  for ( const point& p : P )
                     p0.z += p.z;
                  p0.z /= n;
               }
               Q << p0;
            }
            Q << ConvexHull( P );
         }
      );

      return Q;
   }

   /*!
    * \internal
    * Compute the eigenvectors of a 3x3 symmetric matrix.
    */
   static void ComputeEigenvectors( Matrix& );

   /*!
    * \internal
    * Compute the convex hull of a point set.
    */
   static point_list ConvexHull( point_list& );
};

// ----------------------------------------------------------------------------

}  // pcl

#endif   // __PCL_SurfaceSimplifier_h

// ----------------------------------------------------------------------------
// EOF pcl/SurfaceSimplifier.h - Released 2020-12-17T15:46:28Z
