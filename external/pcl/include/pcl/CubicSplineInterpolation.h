//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/CubicSplineInterpolation.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_CubicSplineInterpolation_h
#define __PCL_CubicSplineInterpolation_h

/// \file pcl/CubicSplineInterpolation.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/UnidimensionalInterpolation.h>

namespace pcl
{

// ----------------------------------------------------------------------------

#define m_x this->m_x
#define m_y this->m_y

/*!
 * \class CubicSplineInterpolation
 * \brief Generic interpolating cubic spline
 *
 * Interpolation with piecewise cubic polynomials. Spline interpolation is
 * usually preferred to interpolation with high-degree polynomials, which are
 * subject to oscillations caused by the Runge's phenomenon.
 *
 * \sa AkimaInterpolation, LinearInterpolation
 */
template <typename T = double>
class PCL_CLASS CubicSplineInterpolation : public UnidimensionalInterpolation<T>
{
public:

   typedef typename UnidimensionalInterpolation<T>::vector_type vector_type;

   /*!
    * Constructs an empty %CubicSplineInterpolation instance, which cannot be
    * used for interpolation prior to initialization.
    */
   CubicSplineInterpolation() = default;

   /*!
    * Copy constructor.
    */
   CubicSplineInterpolation( const CubicSplineInterpolation& ) = default;

   /*!
    * Move constructor.
    */
   CubicSplineInterpolation( CubicSplineInterpolation&& ) = default;

   /*!
    * Virtual destructor.
    */
   virtual ~CubicSplineInterpolation()
   {
   }

   /*!
    * Gets the boundary conditions of this interpolating cubic spline.
    *
    * \param[out] y1    First derivative of the interpolating cubic spline at
    *                   the first data point x[0].
    *
    * \param[out] yn    First derivative of the interpolating cubic spline at
    *                   the last data point x[n-1].
    */
   void GetBoundaryConditions( double& y1, double& yn ) const
   {
      y1 = m_dy1;
      yn = m_dyn;
   }

   /*!
    * Sets the boundary conditions of this interpolating cubic spline.
    *
    * \param y1   First derivative of the interpolating cubic spline at the
    *             first data point x[0].
    *
    * \param yn   First derivative of the interpolating cubic spline at the
    *             last data point x[n-1].
    */
   void SetBoundaryConditions( double y1, double yn )
   {
      Clear();
      m_dy1 = y1;
      m_dyn = yn;
   }

   /*!
    * Generation of an interpolating cubic spline.
    *
    * \param x    %Vector of x-values:\n
    *             \n
    *    \li If \a x is not empty: Must be a vector of monotonically
    *    increasing, distinct values: x[0] < x[1] < ... < x[n-1].\n
    *    \li If \a x is empty: This function will generate a natural cubic
    *    spline with implicit x[i] = i for i = {0,1,...,n-1}.
    *
    * \param y    %Vector of function values for i = {0,1,...,n-1}.
    *
    * When \a x is an empty vector, a <em>natural spline</em> is always
    * generated: boundary conditions are ignored and taken as zero at both ends
    * of the data sequence.
    *
    * The length of the \a y vector (and also the length of a nonempty \a x
    * vector) must be \e n >= 2.
    */
   void Initialize( const vector_type& x, const vector_type& y ) override
   {
      if ( y.Length() < 2 )
         throw Error( "CubicSplineInterpolation::Initialize(): Less than two data points specified." );

      try
      {
         Clear();
         UnidimensionalInterpolation<T>::Initialize( x, y );

         int n = this->Length();
         m_dy2 = DVector( n );
         m_current = -1; // prepare for 1st interpolation
         DVector w( n ); // working vector

         if ( m_x )
         {
            /*
             * Cubic splines with explicit x[i] for i = {0,...,n-1}.
             */
            if ( m_dy1 == 0 && m_dyn == 0 )
            {
               /*
                * Natural cubic spline.
                */
               m_dy2[0] = m_dy2[n-1] = w[0] = 0;

               for ( int i = 1; i < n-1; ++i )
               {
                  double s = (double( m_x[i] ) - double( m_x[i-1] )) / (double( m_x[i+1] ) - double( m_x[i-1] ));
                  double p = s*m_dy2[i-1] + 2;
                  m_dy2[i] = (s - 1)/p;
                  w[i] = (double( m_y[i+1] ) - double( m_y[i  ] )) / (double( m_x[i+1] ) - double( m_x[i  ] ))
                       - (double( m_y[i  ] ) - double( m_y[i-1] )) / (double( m_x[i  ] ) - double( m_x[i-1] ));
                  w[i] = (6*w[i]/(double( m_x[i+1] ) - double( m_x[i-1] )) - s*w[i-1])/p;
               }

               for ( int i = n-2; i > 0; --i ) // N.B. w[0] is not defined
                  m_dy2[i] = m_dy2[i]*m_dy2[i+1] + w[i];
            }
            else
            {
               /*
                * Cubic spline with prescribed end point derivatives.
                */
               w[0] = 3/(double( m_x[1] ) - double( m_x[0] ))
                    * ((double( m_y[1] ) - double( m_y[0] ))/(double( m_x[1] ) - double( m_x[0] )) - m_dy1);

               m_dy2[0] = -0.5;

               for ( int i = 1; i < n-1; ++i )
               {
                  double s = (double( m_x[i] ) - double( m_x[i-1] )) / (double( m_x[i+1] ) - double( m_x[i-1] ));
                  double p = s*m_dy2[i-1] + 2;
                  m_dy2[i] = (s - 1)/p;
                  w[i] = (double( m_y[i+1] ) - double( m_y[i  ] )) / (double( m_x[i+1] ) - double( m_x[i  ] ))
                       - (double( m_y[i  ] ) - double( m_y[i-1] )) / (double( m_x[i  ] ) - double( m_x[i-1] ));
                  w[i] = (6*w[i]/(double( m_x[i+1] ) - double( m_x[i-1] )) - s*w[i-1])/p;
               }

               m_dy2[n-1] = (3/(double( m_x[n-1] ) - double( m_x[n-2] ))
                             * (m_dyn - (double( m_y[n-1] ) - double( m_y[n-2] ))/(double( m_x[n-1] ) - double( m_x[n-2] ))) - w[n-2]/2)
                          / (1 + m_dy2[n-2]/2);

               for ( int i = n-2; i >= 0; --i )
                  m_dy2[i] = m_dy2[i]*m_dy2[i+1] + w[i];
            }
         }
         else
         {
            /*
             * Natural cubic spline with
             * implicit x[i] = i for i = {0,1,...,n-1}.
             */
            m_dy2[0] = m_dy2[n-1] = w[0] = 0;

            for ( int i = 1; i < n-1; ++i )
            {
               double p = m_dy2[i-1]/2 + 2;
               m_dy2[i] = -0.5/p;
               w[i] = double( m_y[i+1] ) - 2*double( m_y[i] ) + double( m_y[i-1] );
               w[i] = (3*w[i] - w[i-1]/2)/p;
            }

            for ( int i = n-2; i > 0; --i ) // N.B. w[0] is not defined
               m_dy2[i] = m_dy2[i]*m_dy2[i+1] + w[i];
         }
      }
      catch ( ... )
      {
         Clear();
         throw;
      }
   }

   /*!
    * Cubic spline interpolation. Returns an interpolated value at the
    * specified point \a x.
    */
   double operator()( double x ) const override
   {
      PCL_PRECONDITION( IsValid() )

      int n = this->Length();

      if ( m_x )
      {
         /*
          * Cubic spline with explicit x[i] for i = {0,...,n-1}.
          */

         /*
          * Bracket the evaluation point x by binary search of the closest
          * pair of data points, if needed. m_current < 0 signals first-time
          * evaluation since initialization.
          */
         int j0 = m_current, j1;
         if ( j0 < 0 || x < m_x[j0] || m_x[j0+1] < x )
            for ( j0 = 0, j1 = n-1; j1-j0 > 1; )
            {
               int m = (j0 + j1) >> 1;
               if ( x < m_x[m] )
                  j1 = m;
               else
                  j0 = m;
            }
         else
            j1 = j0 + 1;
         m_current = j0;

         /*
          * Distance h between the closest neighbors. Will be zero (or
          * insignificant) if two or more x values are equal with respect to
          * the machine epsilon for type T.
          */
         double h = double( m_x[j1] ) - double( m_x[j0] );
         if ( 1 + h == 1 )
            return 0.5*(double( m_y[j0] ) + double( m_y[j1] ));

         double a = (double( m_x[j1] ) - x)/h;
         double b = (x - double( m_x[j0] ))/h;
         return a*double( m_y[j0] )
              + b*double( m_y[j1] )
              + ((a*a*a - a)*m_dy2[j0] + (b*b*b - b)*m_dy2[j1])*h*h/6;
      }
      else
      {
         /*
          * Natural cubic spline with implicit x[i] = i for i = {0,1,...,n-1}.
          */
         int j0 = pcl::Range( pcl::TruncInt( x ), 0, n-1 );
         int j1 = pcl::Min( n-1, j0+1 );
         double a = j1 - x;
         double b = x - j0;
         return a*double( m_y[j0] )
              + b*double( m_y[j1] )
              + ((a*a*a - a)*m_dy2[j0] + (b*b*b - b)*m_dy2[j1])/6;
      }
   }

   /*!
    * Resets this cubic spline interpolation, deallocating all internal
    * working structures.
    */
   void Clear() override
   {
      UnidimensionalInterpolation<T>::Clear();
      m_dy2.Clear();
   }

   /*!
    * Returns true iff this interpolation is valid, i.e. if it has been
    * correctly initialized and is ready to interpolate function values.
    */
   bool IsValid() const override
   {
      return m_dy2;
   }

private:

           double  m_dy1 = 0;      // 1st derivative of spline at the first data point
           double  m_dyn = 0;      // 1st derivative of spline at the last data point
           DVector m_dy2;          // second derivatives of the interpolating function at x[i]
   mutable int     m_current = -1; // index of the current interpolation segment
};

#undef m_x
#undef m_y

// ----------------------------------------------------------------------------

} // pcl

#endif  // __PCL_CubicSplineInterpolation_h

// ----------------------------------------------------------------------------
// EOF pcl/CubicSplineInterpolation.h - Released 2020-12-17T15:46:29Z
