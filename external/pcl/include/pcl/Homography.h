//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/Homography.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_Homography_h
#define __PCL_Homography_h

/// \file pcl/Homography.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/Algebra.h>
#include <pcl/Array.h>
#include <pcl/Matrix.h>
#include <pcl/Point.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class Homography
 * \brief Homography geometric transformation.
 *
 * A two-dimensional projective transformation, or \e homography, is a
 * line-preserving geometric transformation between two sets of points in the
 * plane. More formally, if P represents the set of points in the plane, a
 * homography is an invertible mapping H from P^2 to itself such that three
 * points p1, p2, p3 are collinear if and only if H(p1), H(p2), H(p3) do.
 *
 * Homographies have important practical applications in the field of computer
 * vision. On the PixInsight platform, this class is an essential component of
 * image registration and astrometry processes.
 */
template <class P = DPoint>
class PCL_CLASS Homography
{
public:

   /*!
    * Represents a reference point in two dimensions.
    */
   typedef P               point;

   /*!
    * Represents a list of two-dimensional reference points involved in a
    * homography transformation.
    */
   typedef Array<point>    point_list;

   /*!
    * Default constructor. Constructs a no-op transformation with a unit matrix
    * transformation matrix.
    */
   Homography()
      : m_H( Matrix::UnitMatrix( 3 ) )
   {
   }

   /*!
    * Constructor from a given homography matrix.
    */
   Homography( const Matrix& H )
      : m_H( H )
   {
   }

   /*!
    * Constructor from two 2D point lists.
    *
    * Computes a homography transformation to generate a list \a P2 of
    * transformed points from a list \a P1 of original points. In other words,
    * the computed homography H works as follows:
    *
    * P2 = H( P1 )
    *
    * The transformation matrix is calculated by the Direct Linear
    * Transformation (DLT) method. Both point lists must contain at least four
    * points.
    *
    * If one of the specified point lists contains less than four points, or if
    * no homography can be estimated from the specified point lists (which
    * leads to a singular transformation matrix), this constructor throws an
    * appropriate Error exception.
    *
    * \b References
    *
    * R. Hartley, <em>In defense of the eight-point algorithm.</em> IEEE
    * Transactions on Pattern Analysis and Machine Intelligence, vol. 19, pp.
    * 580–593, June 1997.
    */
   Homography( const point_list& P1, const point_list& P2 )
      : m_H( DLT( P1, P2 ) )
   {
   }

   /*!
    * Copy constructor.
    */
   Homography( const Homography& ) = default;

   /*!
    * Copy assignment operator.
    */
   Homography& operator =( const Homography& ) = default;

   /*!
    * Move constructor.
    */
   Homography( Homography&& ) = default;

   /*!
    * Move assignment operator.
    */
   Homography& operator =( Homography&& ) = default;

   /*!
    * Coordinate transformation operator. Applies the homography matrix to the
    * specified \a x and \a y coordinates. Returns the transformed point as a
    * two-dimensional point with real coordinates.
    */
   template <typename T>
   DPoint operator ()( T x, T y ) const
   {
      double w = m_H[2][0]*x + m_H[2][1]*y + m_H[2][2];
      PCL_CHECK( 1 + w != 1 )
      return DPoint( (m_H[0][0]*x + m_H[0][1]*y + m_H[0][2])/w,
                     (m_H[1][0]*x + m_H[1][1]*y + m_H[1][2])/w );
   }

   /*!
    * Point transformation operator. Applies the homography matrix to the
    * coordinates of the specified point \a p. Returns the transformed point as
    * a two-dimensional point with real coordinates.
    */
   template <typename T>
   DPoint operator ()( const GenericPoint<T>& p ) const
   {
      return operator ()( p.x, p.y );
   }

   /*!
    * Returns the inverse of this homography transformation.
    *
    * If this transformation has been computed from two point lists \a P1 and
    * \a P2:
    *
    * P2 = H( P1 )
    *
    * then this function returns a transformation H1 such that:
    *
    * P1 = H1( P2 )
    */
   Homography Inverse() const
   {
      return Homography( m_H.Inverse() );
   }

   /*!
    * Returns the homography transformation matrix.
    */
   operator const Matrix&() const
   {
      return m_H;
   }

   /*!
    * Returns true iff this transformation has been initialized and is valid.
    */
   bool IsValid() const
   {
      return !m_H.IsEmpty();
   }

   /*!
    * Returns true iff this is an affine homography transformation.
    *
    * An affine homography is a special type of a general homography where the
    * last row of the 3x3 transformation matrix is equal to (0, 0, 1). This
    * function verifies that this property holds for the current transformation
    * matrix (if it is valid) up to the machine epsilon for the \c double
    * numeric type.
    */
   bool IsAffine() const
   {
      return     IsValid()
          &&     Abs( m_H[2][0] ) <= std::numeric_limits<double>::epsilon()
          &&     Abs( m_H[2][1] ) <= std::numeric_limits<double>::epsilon()
          && 1 - Abs( m_H[2][2] ) <= std::numeric_limits<double>::epsilon();
   }

   /*!
    * Ensures that the transformation uniquely references its internal matrix
    * data.
    */
   void EnsureUnique()
   {
      m_H.EnsureUnique();
   }

private:

   Matrix m_H;

   /*
    * Normalization of reference points.
    */
   struct NormalizedPoints
   {
      Array<DPoint> N; // the normalized points
      Matrix        T; // 3x3 normalization matrix

      NormalizedPoints( const point_list& points )
      {
         /*
          * Calculate the centroid of the input set of points.
          */
         DPoint centroid( 0 );
         for ( const auto& p : points )
            centroid += p;
         centroid /= points.Length();

         /*
          * Calculate mean centroid distance and move origin to the centroid.
          */
         double d0 = 0;
         for ( const auto& p : points )
         {
            double x = p.x - centroid.x;
            double y = p.y - centroid.y;
            N << DPoint( x, y );
            d0 += Sqrt( x*x + y*y );
         }
         d0 /= points.Length();

         /*
          * Scale point coordinates.
          */
         double scale = Const<double>::sqrt2()/d0;
         for ( auto& p : N )
            p *= scale;

         /*
          * Form the normalization matrix.
          */
         T = Matrix( scale, 0.0,   -scale*centroid.x,
                     0.0,   scale, -scale*centroid.y,
                     0.0,   0.0,   1.0 );
      }
   };

   /*
    * Implementation of the Direct Linear Transformation (DLT) method to
    * compute a normalized homography matrix.
    */
   static Matrix DLT( const point_list& P1, const point_list& P2 )
   {
      int n = Min( P1.Length(), P2.Length() );
      if ( n < 4 )
         throw Error( "Homography::DLT(): Less than four points specified." );

      /*
       * Normalize all points.
       */
      NormalizedPoints p1( P1 );
      NormalizedPoints p2( P2 );

      /*
       * Setup cross product matrix A.
       */
      Matrix A( 2*n, 9 );
      for ( int i = 0; i < n; ++i )
      {
         double  x1 = p1.N[i].x;
         double  y1 = p1.N[i].y;
         double  x2 = p2.N[i].x;
         double  y2 = p2.N[i].y;
         double* A0 = A[2*i];
         double* A1 = A[2*i + 1];
         //double* A2 = A[3*i + 2]; <-- linearly dependent eqn.

         A0[0] = A0[1] = A0[2] = 0;
         A0[3] = -x1;
         A0[4] = -y1;
         A0[5] = -1;
         A0[6] =  y2*x1;
         A0[7] =  y2*y1;
         A0[8] =  y2;

         A1[0] =  x1;
         A1[1] =  y1;
         A1[2] =  1;
         A1[3] = A1[4] = A1[5] = 0;
         A1[6] = -x2*x1;
         A1[7] = -x2*y1;
         A1[8] = -x2;

         /*                         <-- linearly dependent eqn.
         A2[0] = -y2*x1;
         A2[1] = -y2*y1;
         A2[2] = -y2;
         A2[3] =  x2*x1;
         A2[4] =  x2*y1;
         A2[5] =  x2;
         A2[6] = A2[7] = A2[8] = 0;
         */
      }

      /*
       * SVD of cross product matrix.
       */
      InPlaceSVD svd( A );

      /*
       * For sanity, set to zero all insignificant singular values.
       */
      for ( int i = 0; i < svd.W.Length(); ++i )
         if ( Abs( svd.W[i] ) <= std::numeric_limits<double>::epsilon() )
            svd.W[i] = 0;

      /*
       * Locate the smallest nonzero singular value.
       */
      int i = svd.IndexOfSmallestSingularValue();

      /*
       * The components of the homography matrix are those of the smallest
       * eigenvector, i.e. the column vector of V corresponding to the smallest
       * singular value.
       */
      Matrix H( svd.V[0][i], svd.V[1][i], svd.V[2][i],
                svd.V[3][i], svd.V[4][i], svd.V[5][i],
                svd.V[6][i], svd.V[7][i], svd.V[8][i] );

      if ( Abs( H[2][2] ) <= std::numeric_limits<double>::epsilon() )
         throw Error( "Homography::DLT(): Singular matrix." );

      /*
       * Denormalize matrix components.
       */
      H = p2.T.Inverse() * H * p1.T;

      /*
       * Normalize matrix so that H[2][2] = 1.
       */
      return H *= 1/H[2][2];
   }
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_Homography_h

// ----------------------------------------------------------------------------
// EOF pcl/Homography.h - Released 2020-12-17T15:46:29Z
