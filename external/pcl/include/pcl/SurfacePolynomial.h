//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/SurfacePolynomial.h - Released 2020-12-17T15:46:28Z
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

#ifndef __PCL_SurfacePolynomial_h
#define __PCL_SurfacePolynomial_h

/// \file pcl/SurfacePolynomial.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/Matrix.h>
#include <pcl/Point.h>
#include <pcl/Vector.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class SurfacePolynomial
 * \brief Two-dimensional interpolating/approximating surface polynomial.
 *
 * %SurfacePolynomial implements interpolating surface polynomials for
 * arbitrarily distributed input nodes in two dimensions.
 *
 * Compared to surface splines (also known as <em>thin plates</em>), surface
 * polynomials are much more rigid, that is, much less adaptable to local
 * function variations. This can be a desirable property in some scenarios. An
 * important drawback of polynomials is that they are prone to oscillations,
 * especially for relatively high interpolation orders greater than three. An
 * important advantage is that they are fast even for very large data sets.
 *
 * \sa SurfaceSpline
 */
template <typename T>
class PCL_CLASS SurfacePolynomial
{
public:

   /*!
    * Represents a vector of coordinates, function values or polynomial
    * coefficients.
    */
   typedef GenericVector<T>               vector_type;

   /*!
    * The numeric type used to represent coordinates, function values and
    * polynomial coefficients.
    */
   typedef typename vector_type::scalar   scalar;

   /*!
    * Default constructor. Constructs an empty, two-dimensional interpolating
    * surface polynomial of third degree.
    */
   SurfacePolynomial() = default;

   /*!
    * Copy constructor.
    */
   SurfacePolynomial( const SurfacePolynomial& ) = default;

   /*!
    * Move constructor.
    */
   SurfacePolynomial( SurfacePolynomial&& ) = default;

   /*!
    * Virtual destructor.
    */
   virtual ~SurfacePolynomial()
   {
   }

   /*!
    * Copy assignment operator. Returns a reference to this object.
    */
   SurfacePolynomial& operator =( const SurfacePolynomial& ) = default;

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   SurfacePolynomial& operator =( SurfacePolynomial&& ) = default;

   /*!
    * Returns true iff this surface polynomial is valid. A valid surface
    * polynomial has been initialized with a sufficient number of nodes.
    */
   bool IsValid() const
   {
      return !m_polynomial.IsEmpty();
   }

   /*!
    * Returns the degree of this surface polynomial.
    */
   int Degree() const
   {
      return m_degree;
   }

   /*!
    * Sets the degree of this surface polynomial.
    *
    * \param degree  Polynomial degree. Must be &ge; 1.
    *
    * Calling this member function implicitly resets this %SurfacePolynomial
    * object and destroys all internal working structures.
    *
    * High-degree polynomials, especially for degrees higher than three, tend
    * to oscillate and may generate artifacts in the interpolated surface.
    *
    * The default degree is 3. Recommended values are 2, 3 and 4.
    */
   void SetDegree( int degree )
   {
      PCL_PRECONDITION( degree >= 1 )
      Clear();
      m_degree = pcl::Max( 1, degree );
   }

   /*!
    * Generation of a two-dimensional surface polynomial.
    *
    * \param x       X node coordinates.
    *
    * \param y       Y node coordinates.
    *
    * \param z       Node values.
    *
    * \param n       Number of nodes. Must be &ge; 3
    *                (3 nodes * 2 coordinates = six degrees of freedom).
    *
    * The input nodes can be arbitrarily distributed, and they don't need to
    * follow any specific order. However, all nodes must be distinct with
    * respect to the machine epsilon for the floating point type T.
    */
   void Initialize( const T* x, const T* y, const T* z, int n )
   {
      PCL_PRECONDITION( x != nullptr && y != nullptr && z != nullptr )
      PCL_PRECONDITION( n > 2 )

      if ( n < 3 )
         throw Error( "At least three input nodes are required in SurfacePolynomial::Initialize()" );

      Clear();

      // Find mean coordinate values
      m_x0 = m_y0 = 0;
      for ( int i = 0; i < n; ++i )
      {
         m_x0 += x[i];
         m_y0 += y[i];
      }
      m_x0 /= n;
      m_y0 /= n;

      // Find radius of unit circle
      m_r0 = 0;
      for ( int i = 0; i < n; ++i )
      {
         double dx = m_x0 - x[i];
         double dy = m_y0 - y[i];
         double r = Sqrt( dx*dx + dy*dy );
         if ( r > m_r0 )
            m_r0 = r;
      }
      if ( 1 + m_r0 == 1 )
         throw Error( "SurfacePolynomial::Initialize(): Empty or insignificant interpolation space" );

      m_r0 = 1/m_r0;

      const int size = (m_degree + 1)*(m_degree + 2) >> 1;

      DMatrix M( 0.0, size, size );
      DVector R( 0.0, size );
      {
         // Transform coordinates to unit circle
         DVector X( n ), Y( n );
         for ( int i = 0; i < n; ++i )
         {
            X[i] = m_r0*(x[i] - m_x0);
            Y[i] = m_r0*(y[i] - m_y0);
         }

         GenericVector<DVector> Z( n );
         for ( int k = 0; k < n; ++k )
         {
            Z[k] = DVector( size );
            for ( int i = 0, l = 0; i <= m_degree; ++i )
               for ( int j = 0; j <= m_degree-i; ++j, ++l )
                  Z[k][l] = PowI( X[k], i ) * PowI( Y[k], j );
         }

         int n2 = n*n;
         for ( int i = 0; i < size; ++i )
         {
            for ( int j = 0; j < size; ++j )
            {
               for ( int k = 0; k < n; ++k )
                  M[i][j] += Z[k][i] * Z[k][j];
               M[i][j] /= n2;
            }

            for ( int k = 0; k < n; ++k )
               R[i] += z[k] * Z[k][i];
            R[i] /= n2;
         }
      }

      for ( int i = 0; i < size; ++i )
      {
         double pMi = M[i][i];
         if ( M[i][i] != 0 )
         {
            for ( int j = i; j < size; ++j )
               M[i][j] /= pMi;
            R[i] /= pMi;
         }

         for ( int k = 1; i+k < size; ++k )
         {
            double pMk = M[i+k][i];
            if ( M[i+k][i] != 0 )
            {
               for ( int j = i; j < size; ++j )
               {
                  M[i+k][j] /= pMk;
                  M[i+k][j] -= M[i][j];
               }
               R[i+k] /= pMk;
               R[i+k] -= R[i];
            }
         }
      }

      m_polynomial = vector_type( size );
      for ( int i = size; --i >= 0; )
      {
         m_polynomial[i] = scalar( R[i] );
         for ( int j = i; --j >= 0; )
            R[j] -= M[j][i]*R[i];
      }
   }

   /*!
    * Two-dimensional surface polynomial interpolation. Returns an interpolated
    * value at the specified \a x and \a y coordinates.
    */
   T operator ()( double x, double y ) const
   {
      PCL_PRECONDITION( !m_polynomial.IsEmpty() )

      double dx = m_r0*(x - m_x0);
      double dy = m_r0*(y - m_y0);
      double z = 0;
      double px = 1;
      for ( int i = 0, l = 0; ; px *= dx )
      {
         double py = 1;
         for ( int j = 0; j <= m_degree-i; py *= dy, ++j, ++l )
            z += m_polynomial[l]*px*py;
         if ( ++i > m_degree )
            break;
      }
      return T( z );
   }

   /*!
    * Resets this surface polynomial interpolation, deallocating all internal
    * working structures.
    */
   void Clear()
   {
      m_polynomial.Clear();
   }

protected:

   double      m_r0 = 1;     // scaling factor for normalization of node coordinates
   double      m_x0 = 0;     // zero offset for normalization of X node coordinates
   double      m_y0 = 0;     // zero offset for normalization of Y node coordinates
   int         m_degree = 3; // polynomial degree > 0
   vector_type m_polynomial; // coefficients of the 2-D surface polynomial
};

// ----------------------------------------------------------------------------

/*!
 * \class PointSurfacePolynomial
 * \brief Vector polynomial interpolation/approximation in two dimensions
 *
 * The template parameter P represents an interpolation point in two
 * dimensions. The type P must implement P::x and P::y data members accessible
 * from the current %PointSurfacePolynomial template specialization. These
 * members must provide the values of the horizontal and vertical coordinates,
 * respectively, of an interpolation point. In addition, the scalar types of
 * the P::x and P::y point members must support conversion to double semantics.
 */
template <class P = DPoint>
class PCL_CLASS PointSurfacePolynomial
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
   typedef SurfacePolynomial<double>   surface;

   /*!
    * Default constructor. Yields an empty instance that cannot be used without
    * initialization.
    */
   PointSurfacePolynomial() = default;

   /*!
    * Copy constructor.
    */
   PointSurfacePolynomial( const PointSurfacePolynomial& ) = default;

   /*!
    * Move constructor.
    */
   PointSurfacePolynomial( PointSurfacePolynomial&& ) = default;

   /*!
    * Constructs a %PointSurfacePolynomial object initialized for the
    * specified input data and interpolation parameters.
    *
    * See the corresponding Initialize() member function for a detailed
    * description of parameters.
    */
   PointSurfacePolynomial( const point_list& P1, const point_list& P2, int degree = 3 )
   {
      Initialize( P1, P2, degree );
   }

   /*!
    * Constructs a %PointSurfacePolynomial object initialized with prescribed
    * point surface interpolations.
    *
    * See the corresponding Initialize() member function for a more detailed
    * description of parameters and their required conditions.
    */
   PointSurfacePolynomial( const surface& Sx, const surface& Sy )
   {
      Initialize( Sx, Sy );
   }

   /*!
    * Copy assignment operator. Returns a reference to this object.
    */
   PointSurfacePolynomial& operator =( const PointSurfacePolynomial& ) = default;

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   PointSurfacePolynomial& operator =( PointSurfacePolynomial&& ) = default;

   /*!
    * Initializes this %PointSurfacePolynomial object for the specified
    * input data and interpolation parameters.
    *
    * \param P1         A sequence of distinct interpolation node points.
    *
    * \param P2         A sequence of interpolation values. For each point in
    *                   \a P1, the coordinates of its counterpart point in
    *                   \a P2 will be used as the interpolation node values in
    *                   the X and Y directions.
    *
    * \param degree     Polynomial degree. Must be &ge; 1. The default value is
    *                   3. See SurfacePolynomial::SetDegree() for a complete
    *                   description of this parameter.
    *
    * The input nodes can be arbitrarily distributed and don't need to follow
    * any specific order. However, all node points should be distinct with
    * respect to the machine epsilon for the floating point type used to
    * represent coordinates.
    *
    * See the SurfacePolynomial::Initialize() member function for a complete
    * description of this initialization process.
    */
   void Initialize( const point_list& P1, const point_list& P2, int degree = 3 )
   {
      PCL_PRECONDITION( P1.Length() >= 3 )
      PCL_PRECONDITION( P1.Length() <= P2.Length() )
      PCL_PRECONDITION( degree > 0 )

      m_Sx.Clear();
      m_Sy.Clear();

      m_Sx.SetDegree( degree );
      m_Sy.SetDegree( degree );

      if ( P1.Length() < 3 || P2.Length() < 3 )
         throw Error( "PointSurfacePolynomial::Initialize(): At least three input nodes must be specified." );

      if ( P2.Length() < P1.Length() )
         throw Error( "PointSurfacePolynomial::Initialize(): Incompatible point array lengths." );

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
};

// ----------------------------------------------------------------------------

}  // pcl

#endif   // __PCL_SurfacePolynomial_h

// ----------------------------------------------------------------------------
// EOF pcl/SurfacePolynomial.h - Released 2020-12-17T15:46:28Z
