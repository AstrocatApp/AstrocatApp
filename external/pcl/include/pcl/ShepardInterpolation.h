//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/ShepardInterpolation.h - Released 2020-12-17T15:46:28Z
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

#ifndef __PCL_ShepardInterpolation_h
#define __PCL_ShepardInterpolation_h

/// \file pcl/ShepardInterpolation.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/QuadTree.h>
#include <pcl/Vector.h>

#ifndef __PCL_BUILDING_PIXINSIGHT_APPLICATION
#  include <pcl/Console.h>
#endif

namespace pcl
{

// ----------------------------------------------------------------------------

/*
 * Default normalized search radius for local Shepard interpolation. This is an
 * initial search radius relative to the unit circle for the adaptive quadtree
 * search algorithm.
 */
#define __PCL_SHEPARD_DEFAULT_SEARCH_RADIUS  0.10

/*
 * Default power parameter for local Shepard interpolation. Larger values tend
 * to yield more accurate interpolation devices. Small powers lead to more
 * approximating (smoothing) devices. The chosen default value is intermediate.
 */
#define __PCL_SHEPARD_DEFAULT_POWER          4

/*
 * Default regularization (smoothing) factor for local Shepard interpolation,
 * in the range [0,1). This is a clipping fraction for Winsorization of nearby
 * function values in the point interpolation routine.
 */
#define __PCL_SHEPARD_DEFAULT_REGULARIZATION 0

// ----------------------------------------------------------------------------

/*!
 * \class ShepardInterpolation
 * \brief Two-dimensional surface interpolation with the local Shepard method.
 *
 * %ShepardInterpolation implements the Shepard method of function
 * interpolation/approximation for arbitrarily distributed input nodes in two
 * dimensions.
 *
 * This class implements local Shepard interpolation with Franke-Little
 * weights, quadtree structures for fast rectangular search of input nodes,
 * optional regularization, and an adaptive local interpolation search routine.
 *
 * \b References
 *
 * Shepard, Donald (1968). <em>A two-dimensional interpolation function for
 * irregularly-spaced data</em>. Proceedings of the 1968 ACM National
 * Conference, pp. 517-524.
 *
 * Franke, Richard (1982). <em>Scattered data interpolation: tests of some
 * methods</em>. Mathematics of Computation 38 (1982), pp. 181-200.
 *
 * Hanan Samet, <em>Foundations of Multidimensional and Metric Data
 * Structures,</em> Morgan Kaufmann, 2006, Section 1.4.
 *
 * Mark de Berg et al, <em>Computational Geometry: Algorithms and Applications
 * Third Edition,</em> Springer, 2010, Chapter 14.
 *
 * \sa SurfaceSpline, SurfacePolynomial, QuadTree
 */
template <typename T>
class PCL_CLASS ShepardInterpolation
{
public:

   /*!
    * Represents a vector of coordinates or function values.
    */
   typedef GenericVector<T>                  vector_type;

   /*!
    * The numeric type used to represent coordinates and function values.
    */
   typedef typename vector_type::scalar      scalar;

   /*!
    * The class used to implement fast coordinate search operations.
    */
   typedef QuadTree<vector_type>             search_tree;

   /*!
    * The class used to specify interpolation regions.
    */
   typedef typename search_tree::rectangle   search_rect;

   /*!
    * The maximum number of interpolation points in a leaf quadtree node.
    */
   constexpr static int BucketCapacity = 16;

   /*!
    * Default constructor. Constructs an empty %ShepardInterpolation object.
    */
   ShepardInterpolation() = default;

   /*!
    * Copy constructor. Copy construction is disabled because this class uses
    * internal data structures that cannot be copy-constructed. However,
    * %ShepardInterpolation implements move construction and move assignment.
    */
   ShepardInterpolation( const ShepardInterpolation& ) = delete;

   /*!
    * Move constructor.
    */
   ShepardInterpolation( ShepardInterpolation&& ) = default;

   /*!
    * Virtual destructor.
    */
   virtual ~ShepardInterpolation()
   {
   }

   /*!
    * Copy assignment operator. Copy assignment is disabled because this class
    * uses internal data structures that cannot be copy-assigned. However,
    * %ShepardInterpolation implements move assignment and move construction.
    */
   ShepardInterpolation& operator =( const ShepardInterpolation& ) = delete;

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   ShepardInterpolation& operator =( ShepardInterpolation&& ) = default;

   /*!
    * Returns true iff this object is valid. A valid %ShepardInterpolation
    * object has been initialized with a sufficient number of input nodes.
    */
   bool IsValid() const
   {
      return !m_T.IsEmpty();
   }

   /*!
    * Sets the <em>power parameter</em> of the local Shepard interpolation.
    *
    * \param m    Power parameter. Must be > 0.
    *
    * The power parameter is a positive real > 0 that defines the behavior of
    * the interpolation/approximation function. For large values of \a m, the
    * interpolating surface tends to be uniform within boundaries defined
    * around input nodes, and hence is more local. For values of \a m &le; 2,
    * the surface is more global, that is, interpolated values are more
    * influenced by nodes far away from the interpolation coordinates. The
    * default power parameter value is 4.
    *
    * If an invalid value \a m &le; 0 is specified, the default \a m = 4 power
    * parameter value will be set.
    *
    * Calling this member function does not reset this %ShepardInterpolation
    * object, since no internal structures built upon initialization depend on
    * the power parameter. This facilitates the use of this class to compare
    * the results of different power parameter values applied to the same data.
    */
   void SetPower( int m )
   {
      PCL_PRECONDITION( m > 0 )
      m_mu = (m > 0) ? m : __PCL_SHEPARD_DEFAULT_POWER;
   }

   /*!
    * Returns the current power parameter of this local Shepard interpolation.
    *
    * See SetPower() for more information.
    */
   int Power() const
   {
      return m_mu;
   }

   /*!
    * Sets the normalized search radius of the local Shepard interpolation.
    *
    * \param R    Search radius in the range (0,1].
    *
    * The search radius defines a distance from the interpolation point where
    * existing input nodes will be used to compute an interpolated function
    * value. Larger values of \a R will construct more global interpolation
    * surfaces, while smaller values will tend to yield more local
    * interpolations. Smaller search radii will also lead to faster
    * interpolation devices, since the computational complexity is reduced as
    * the number of input nodes used for each interpolation point decreases.
    *
    * The search radius parameter is normalized to the (0,1] range in this
    * implementation, where 1 represents the largest distance between two
    * distinct input nodes, or equivalently, the size of the interpolation
    * region. The default search radius is 0.1.
    *
    * If an invalid value \a R &le; 0 is specified, the default \a R = 0.1
    * search radius parameter value will be set.
    *
    * Calling this member function does not reset this %ShepardInterpolation
    * object, since no internal structures built upon initialization depend on
    * the search radius. This facilitates the use of this class to compare the
    * results of different search radius values applied to the same data.
    */
   void SetRadius( double R )
   {
      PCL_PRECONDITION( R > 0 )
      m_R = (R < 0 || 1 + R == 1) ? __PCL_SHEPARD_DEFAULT_SEARCH_RADIUS : R;
   }

   /*!
    * Returns the current normalized search radius of this local Shepard
    * interpolation. See SetRadius() for more information.
    */
   double Radius() const
   {
      return m_R;
   }

   /*!
    * Sets the <em>smoothing factor</em> of the local Shepard interpolation.
    *
    * \param r    Smoothing factor in the range [0,1).
    *
    * For \a r > 0, a regularized local interpolation will be applied. The \a r
    * argument represents a fraction of the count of nearby function samples
    * that will be Winsorized, that is, replaced with their r-th nearest value
    * at the top and the tail of the interpolation sample.
    *
    * For \a r = 0, a normal (unsmoothed) local Shepard interpolation scheme is
    * used. This is the default state for newly created instances of
    * %ShepardInterpolation.
    *
    * If an invalid value \a r < 0 or \a r &ge; 1 is specified, the default
    * \a r = 0 smoothing factor will be set.
    */
   void SetSmoothing( float r )
   {
      PCL_PRECONDITION( r >= 0 && r < 1 )
      m_r = (r < 0 || r >= 1) ? __PCL_SHEPARD_DEFAULT_REGULARIZATION : r;
   }

   /*!
    * Returns the <em>smoothing factor</em> of this local Shepard
    * interpolation. See SetSmoothing() for more information.
    */
   float Smoothing() const
   {
      return m_r;
   }

   /*!
    * Generation of a two-dimensional surface approximation.
    *
    * \param x       X node coordinates.
    *
    * \param y       Y node coordinates.
    *
    * \param z       Node values.
    *
    * \param n       Number of nodes. There must be at least three distinct
    *                input nodes.
    *
    * The input nodes can be arbitrarily distributed and don't need to follow
    * any specific order. However, all node points should be distinct with
    * respect to the machine epsilon for the floating point type T.
    *
    * This initialization function includes a sanitization routine. If there
    * are duplicate points in the specified set of input nodes, only the first
    * occurrence of each duplicate will be kept to build the interpolation
    * surface, and the rest of duplicate points will be ignored. Two points are
    * considered equal if their coordinates don't differ more than the machine
    * epsilon for the floating point type T.
    */
   void Initialize( const T* x, const T* y, const T* z, int n )
   {
      DoInitialize( nullptr/*rect*/, x, y, z, n );
   }

   /*!
    * Generation of a two-dimensional surface approximation with a prescribed
    * rectangular interpolation region.
    *
    * \param rect    The rectangular region for interpolation.
    *
    * \param x       X node coordinates.
    *
    * \param y       Y node coordinates.
    *
    * \param z       Node values.
    *
    * \param n       Number of nodes. There must be at least three distinct
    *                input nodes within the specified interpolation region.
    *
    * The input nodes can be arbitrarily distributed and don't need to follow
    * any specific order. However, all node points should be distinct with
    * respect to the machine epsilon for the floating point type T.
    *
    * This initialization function includes a sanitization routine. If there
    * are duplicate points in the specified set of input nodes, only the first
    * occurrence of each duplicate will be kept to build the interpolation
    * surface, and the rest of duplicate points will be ignored. Two points are
    * considered equal if their coordinates don't differ more than the machine
    * epsilon for the floating point type T.
    *
    * This function will only take into account input nodes located within the
    * specified region \a rect; all points outside this region will be ignored.
    * A prescribed interpolation region is useful to ensure that the
    * approximation surface can be evaluated on the entire region, for example
    * to represent images or other data sets, not necessarily bounded by the
    * extreme coordinates in the set of input nodes. Specifying a region also
    * allows to use a reduced subset of the available data, to accelerate
    * calculations.
    */
   void Initialize( const search_rect& rect, const T* x, const T* y, const T* z, int n )
   {
      DoInitialize( &rect, x, y, z, n );
   }

   /*!
    * Two-dimensional surface interpolation/approximation with the local
    * Shepard method. Returns an approximated function value at the specified
    * \a x and \a y coordinates.
    *
    * The interpolation function uses an adaptive point search routine. The
    * current search radius is used as an initial parameter. If less than
    * three input nodes are found within the search radius distance from the
    * desired interpolation point, the radius is increased and a new search is
    * performed. This is repeated until at least three nodes are found around
    * the specified interpolation point.
    *
    * In degenerate cases where no valid solution can be found, zero is
    * returned conventionally. These cases are rare and may only happen if the
    * input nodes are very close together with respect to the machine epsilon
    * for the numeric type T.
    */
   T operator ()( double x, double y ) const
   {
      PCL_PRECONDITION( !m_T.IsEmpty() )

      const double dx = m_r0*(x - m_x0);
      const double dy = m_r0*(y - m_y0);

      for ( double R = m_R; ; R += m_R )
      {
         int m = 0;
         double R2 = R*R;
         Array<DPoint> V;
         m_T.Search( DRect( dx-R, dy-R, dx+R, dy+R ),
                     [&]( const vector_type& v, void* )
                     {
                        double x = dx - v[0];
                        double y = dy - v[1];
                        double r2 = x*x + y*y;
                        if ( r2 < R2 )
                        {
                           ++m;
                           double w = PowI( 1 - Sqrt( r2 )/R, m_mu );
                           /*
                            * N.B. The equivalent code below is about 400 times
                            * slower than the above call to PowI() for m_mu=16.
                            * Measured on a TR 2990WX.
                            *
                           double w = 1 - Sqrt( r2 )/R;
                           for ( int i = 1; i < m_mu; ++i )
                              w *= w;
                            */
                           V << DPoint( w, w*v[2] );
                        }
                     },
                     nullptr );
         if ( m >= 3 )
         {
            if ( m_r > 0 )
            {
               /*
                * Regularization by Winsorization of the weighted sample.
                */
               int r = Min( TruncInt( m_r * m ), (m >> 1) - (m & 1)^1 );
               if ( r > 0 )
               {
                  V.Sort( []( const DPoint& v1, const DPoint& v2 )
                     {
                        return v1.y < v2.y;
                     } );
                  for ( int i = 0; i < r; ++i )
                  {
                     V[i].y = V[r].y;
                     V[m-i-1].y = V[m-r-1].y;
                  }
               }
            }
            DPoint Wz( 0 );
            for ( const DPoint& v : V )
               Wz += v;
            if ( 1 + Wz.x != 1 )
               return T( Wz.y/Wz.x );
            if ( R >= 1 )
               break; // degenerate!
         }
      }

      return 0; // empty!?
   }

   /*!
    * Returns an interpolated/approximated function value at the specified
    * \a p.x and \a p.y point coordinates. See operator()( double, double ) for
    * more information.
    */
   template <typename Tp>
   T operator ()( const GenericPoint<Tp>& p ) const
   {
      return operator ()( double( p.x ), double( p.y ) );
   }

   /*!
    * Resets this %ShepardInterpolation object, deallocating all internal
    * working structures.
    */
   void Clear()
   {
      m_T.Clear();
   }

protected:

   double      m_r0 = 1;   // scaling factor for normalization of node coordinates
   double      m_x0 = 0;   // zero offset for normalization of X node coordinates
   double      m_y0 = 0;   // zero offset for normalization of Y node coordinates
   int         m_mu = __PCL_SHEPARD_DEFAULT_POWER;          // power parameter (leveling factor)
   double      m_R  = __PCL_SHEPARD_DEFAULT_SEARCH_RADIUS;  // initial search radius
   float       m_r  = __PCL_SHEPARD_DEFAULT_REGULARIZATION; // regularization (clipping fraction)
   search_tree m_T;        // tree points store input coordinates and function values

   /*!
    * Performs input data normalization and sanitization. Builds the point
    * search quadtree with normalized node coordinates.
    * \internal
    */
   void DoInitialize( const search_rect* rect, const T* x, const T* y, const T* z, int n )
   {
      PCL_PRECONDITION( x != nullptr && y != nullptr && z != nullptr )
      PCL_PRECONDITION( n > 2 )

      if ( n < 3 )
         throw Error( "ShepardInterpolation::Initialize(): At least three input nodes must be specified." );

      Clear();

      try
      {
         if ( rect == nullptr )
         {
            /*
             * Find mean coordinate values.
             */
            m_x0 = m_y0 = 0;
            for ( int i = 0; i < n; ++i )
            {
               m_x0 += x[i];
               m_y0 += y[i];
            }
            m_x0 /= n;
            m_y0 /= n;

            /*
             * Find radius of unit circle.
             */
            m_r0 = 0;
            for ( int i = 0; i < n; ++i )
            {
               double dx = x[i] - m_x0;
               double dy = y[i] - m_y0;
               double r = Sqrt( dx*dx + dy*dy );
               if ( r > m_r0 )
                  m_r0 = r;
            }
         }
         else
         {
            m_x0 = rect->CenterX();
            m_y0 = rect->CenterY();
            m_r0 = rect->Diagonal()/2;
         }

         if ( 1 + m_r0 == 1 )
            throw Error( "ShepardInterpolation::Initialize(): Empty or insignificant interpolation space." );
         m_r0 = 1/m_r0;

         /*
          * Build working vector. Transform coordinates to the unit circle.
          */
         Array<vector_type> V;
         for ( int i = 0; i < n; ++i )
            V << vector_type( m_r0*(x[i] - m_x0), m_r0*(y[i] - m_y0), z[i] );

         /*
          * Find and remove duplicate input nodes. Two nodes are equal if their
          * coordinates don't differ more than the machine epsilon for the
          * floating point type T.
          */
         V.Sort(  []( const vector_type& p, const vector_type& q )
                  {
                     return (p[0] != q[0]) ? p[0] < q[0] : p[1] < q[1];
                  } );
         Array<int> remove;
         for ( int i = 0, j = 1; j < n; ++i, ++j )
            if ( Abs( V[i][0] - V[j][0] ) <= std::numeric_limits<T>::epsilon() )
               if ( Abs( V[i][1] - V[j][1] ) <= std::numeric_limits<T>::epsilon() )
                  remove << i;
         if ( !remove.IsEmpty() )
         {
            Array<vector_type> U;
            int i = 0;
            for ( int j : remove )
            {
               for ( ; i < j; ++i )
                  U << V[i];
               ++i;
            }
            for ( ; i < n; ++i )
               U << V[i];
            if ( U.Length() < 3 )
               throw Error( "ShepardInterpolation::Initialize(): Less than three input nodes left after sanitization." );
            V = U;
         }

         /*
          * Build the point search tree.
          */
         if ( rect == nullptr )
            m_T.Build( V, BucketCapacity );
         else
         {
            m_T.Build( *rect, V, BucketCapacity );
            if ( m_T.Length() < 3 )
               throw Error( "ShepardInterpolation::Initialize(): Less than three input nodes in the specified search region." );
         }
      }
      catch ( ... )
      {
         Clear();
         throw;
      }
   }
};

// ----------------------------------------------------------------------------

/*!
 * \class PointShepardInterpolation
 * \brief Vector Shepard interpolation/approximation in two dimensions
 *
 * The template parameter P represents an interpolation point in two
 * dimensions. The type P must implement P::x and P::y data members accessible
 * from the current %PointShepardInterpolation template specialization. These
 * members must provide the values of the horizontal and vertical coordinates,
 * respectively, of an interpolation point. In addition, the scalar types of
 * the P::x and P::y point members must support conversion to double semantics.
 */
template <class P = DPoint>
class PCL_CLASS PointShepardInterpolation
{
public:

   /*!
    * Represents an interpolation point in two dimensions.
    */
   typedef P                           point;

   /*!
    * Represents a sequence of interpolation points.
    */
   typedef Array<point>                point_list;

   /*!
    * Represents a coordinate interpolating/approximating surface.
    */
   typedef ShepardInterpolation<double> surface;

   /*!
    * Default constructor. Yields an empty instance that cannot be used without
    * initialization.
    */
   PointShepardInterpolation() = default;

   /*!
    * Copy constructor.
    */
   PointShepardInterpolation( const PointShepardInterpolation& ) = default;

   /*!
    * Move constructor.
    */
   PointShepardInterpolation( PointShepardInterpolation&& ) = default;

   /*!
    * Constructs a %PointShepardInterpolation object initialized for the
    * specified input data and interpolation parameters.
    *
    * See the corresponding Initialize() member function for a detailed
    * description of parameters.
    */
   PointShepardInterpolation( const point_list& P1, const point_list& P2,
                              int power = __PCL_SHEPARD_DEFAULT_POWER,
                              double radius = __PCL_SHEPARD_DEFAULT_SEARCH_RADIUS )
   {
      Initialize( P1, P2, power, radius );
   }

   /*!
    * Constructs a %PointShepardInterpolation object initialized with
    * prescribed point surface interpolations.
    *
    * See the corresponding Initialize() member function for a more detailed
    * description of parameters and their required conditions.
    */
   PointShepardInterpolation( const surface& Sx, const surface& Sy )
   {
      Initialize( Sx, Sy );
   }

   /*!
    * Copy assignment operator. Copy assignment has been disabled for this
    * class because the ShepardInterpolation class does not implement copy
    * assignment.
    */
   PointShepardInterpolation& operator =( const PointShepardInterpolation& ) = delete;

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   PointShepardInterpolation& operator =( PointShepardInterpolation&& ) = default;

   /*!
    * Initializes this %PointShepardInterpolation object for the specified
    * input data and interpolation parameters.
    *
    * \param P1         A sequence of distinct interpolation node points.
    *
    * \param P2         A sequence of interpolation values. For each point in
    *                   \a P1, the coordinates of its counterpart point in
    *                   \a P2 will be used as the interpolation node values in
    *                   the X and Y directions.
    *
    * \param power      Power parameter. Must be > 0. The default value is 4.
    *                   See ShepardInterpolation::SetPower() for a complete
    *                   description of this parameter.
    *
    * \param radius     Normalized search radius. Must be > 0. The default
    *                   value is 0.1. See ShepardInterpolation::SetRadius() for
    *                   a complete description of this parameter.
    *
    * \param smoothing  Smoothing factor. Must be in the range [0,1). The
    *                   default value is zero. See
    *                   ShepardInterpolation::SetSmoothing() for a complete
    *                   description of this parameter.
    *
    * The input nodes can be arbitrarily distributed and don't need to follow
    * any specific order. However, all node points should be distinct with
    * respect to the machine epsilon for the floating point type used to
    * represent coordinates.
    *
    * See the ShepardInterpolation::Initialize() member function for a complete
    * description of this initialization process.
    */
   void Initialize( const point_list& P1, const point_list& P2,
                    int power = __PCL_SHEPARD_DEFAULT_POWER,
                    double radius = __PCL_SHEPARD_DEFAULT_SEARCH_RADIUS,
                    float smoothing = __PCL_SHEPARD_DEFAULT_REGULARIZATION )
   {
      PCL_PRECONDITION( P1.Length() >= 3 )
      PCL_PRECONDITION( P1.Length() <= P2.Length() )
      PCL_PRECONDITION( power > 0 )
      PCL_PRECONDITION( radius > 0 )
      PCL_PRECONDITION( smoothing >= 0 && smoothing < 1 )

      m_Sx.Clear();
      m_Sy.Clear();

      m_Sx.SetPower( power );
      m_Sy.SetPower( power );

      m_Sx.SetRadius( radius );
      m_Sy.SetRadius( radius );

      m_Sx.SetSmoothing( smoothing );
      m_Sy.SetSmoothing( smoothing );

      if ( P1.Length() < 3 || P2.Length() < 3 )
         throw Error( "PointShepardInterpolation::Initialize(): At least three input nodes must be specified." );

      if ( P2.Length() < P1.Length() )
         throw Error( "PointShepardInterpolation::Initialize(): Incompatible point array lengths." );

      DVector X( int( P1.Length() ) ),
              Y( int( P1.Length() ) ),
              Zx( int( P1.Length() ) ),
              Zy( int( P1.Length() ) );
      for ( int i = 0; i < X.Length(); ++i )
      {
          X[i] = P1[i].x;
          Y[i] = P1[i].y;
         Zx[i] = P2[i].x;
         Zy[i] = P2[i].y;
      }
      m_Sx.Initialize( X.Begin(), Y.Begin(), Zx.Begin(), X.Length() );
      m_Sy.Initialize( X.Begin(), Y.Begin(), Zy.Begin(), X.Length() );
   }

   /*!
    * Deallocates internal structures, yielding an empty object that cannot be
    * used before a new call to Initialize().
    */
   void Clear()
   {
      m_Sx.Clear();
      m_Sy.Clear();
   }

   /*!
    * Returns true iff this is a valid, initialized object ready for
    * interpolation.
    */
   bool IsValid() const
   {
      return m_Sx.IsValid() && m_Sy.IsValid();
   }

   /*!
    * Returns a reference to the internal object used for interpolation in the
    * X plane direction.
    */
   const surface& SurfaceX() const
   {
      return m_Sx;
   }

   /*!
    * Returns a reference to the internal object used for interpolation in the
    * Y plane direction.
    */
   const surface& SurfaceY() const
   {
      return m_Sy;
   }

   /*!
    * Returns an interpolated point at the specified coordinates.
    */
   template <typename T>
   DPoint operator ()( T x, T y ) const
   {
      return DPoint( m_Sx( x, y ), m_Sy( x, y ) );
   }

   /*!
    * Returns an interpolated point at the given \a p.x and \a p.y coordinates.
    */
   template <typename T>
   DPoint operator ()( const GenericPoint<T>& p ) const
   {
      return operator ()( p.x, p.y );
   }

private:

   /*
    * The surface interpolations in the X and Y plane directions.
    */
   surface m_Sx, m_Sy;

   friend class DrizzleData;
   friend class DrizzleDataDecoder;
};

// ----------------------------------------------------------------------------

}  // pcl

#endif   // __PCL_ShepardInterpolation_h

// ----------------------------------------------------------------------------
// EOF pcl/ShepardInterpolation.h - Released 2020-12-17T15:46:28Z
