//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/Vector.h - Released 2020-12-17T15:46:28Z
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

#ifndef __PCL_Vector_h
#define __PCL_Vector_h

/// \file pcl/Vector.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/Container.h>
#include <pcl/Exception.h>
#include <pcl/ReferenceCounter.h>
#include <pcl/Search.h>
#include <pcl/Sort.h>
#include <pcl/Utility.h>

#ifndef __PCL_NO_VECTOR_STATISTICS
#  include <pcl/Selection.h>
#endif

#ifndef __PCL_NO_VECTOR_INSTANTIATE
#  include <pcl/Complex.h>
#endif

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class GenericVector
 * \brief Generic vector of arbitrary length.
 *
 * %GenericVector is a lightweight template class implementing a vector of
 * arbitrary length. This class provides the following main features:
 *
 * \li Implicit data sharing with reference counting and copy-on-write
 * functionality. %GenericVector instances can safely be passed as function
 * return values and by-value function arguments.
 *
 * \li Thread-safe. %GenericVector instances can safely be accessed from
 * multiple threads. The reference counter implements atomic reference and
 * dereference operations.
 *
 * \li Efficient vector storage and access to vector elements. Vector
 * elements are allocated as a single, contiguous memory block.
 *
 * \li Support for a large set of vector operations, including scalar-to-vector
 * and vector-to-vector arithmetic operations, dot and cross products.
 *
 * \li Calculation of a variety of descriptive statistics of vector components.
 *
 * \sa GenericMatrix, \ref vector_operators, \ref vector_types
 */
template <typename T>
class PCL_CLASS GenericVector : public DirectContainer<T>
{
public:

   /*!
    * Represents a scalar.
    */
   typedef T                        scalar;

   /*!
    * Represents a vector component.
    */
   typedef T                        component;

   /*!
    * Represents a mutable vector iterator.
    */
   typedef T*                       iterator;

   /*!
    * Represents an immutable vector iterator.
    */
   typedef const T*                 const_iterator;

   /*!
    * Constructs an empty vector. An empty vector has no components and its
    * length is zero.
    */
   GenericVector()
   {
      m_data = new Data;
   }

   /*!
    * Constructs an uninitialized vector of the specified length.
    *
    * \param len     Number of vector components (>= 0).
    *
    * This constructor does not initialize vector components. The newly created
    * vector will contain unpredictable values.
    */
   GenericVector( int len )
   {
      m_data = new Data( len );
   }

   /*!
    * Constructs a vector and fills it with a constant component value.
    *
    * \param x       Initial value for all vector components.
    * \param len     Number of vector components (>= 0).
    */
   GenericVector( const component& x, int len )
   {
      m_data = new Data( len );
            iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
      PCL_IVDEP
      for ( ; i < j; ++i )
         *i = x;
   }

   /*!
    * Constructs a vector and initializes it with component values taken from a
    * static array.
    *
    * \param a       Address of the first item of a static array for
    *                initialization of vector components. The array must provide
    *                at least \a len consecutive items.
    *
    * \param len     Number of vector components (>= 0).
    */
   template <typename T1>
   GenericVector( const T1* a, int len )
   {
      m_data = new Data( len );
      if ( a != nullptr )
      {
               iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
         const T1*      __restrict__ k = a;
         PCL_IVDEP
         for ( ; i < j; ++i, ++k )
            *i = component( *k );
      }
   }

   /*!
    * Constructs a vector and initializes it with component values taken from
    * the specified initializer list \a c.
    *
    * This constructor is equivalent to:
    *
    * \code GenericVector( c.begin(), int( c.size() ) ) \endcode
    */
   template <typename T1>
   GenericVector( std::initializer_list<T1> c )
      : GenericVector( c.begin(), int( c.size() ) )
   {
   }

   /*!
    * Constructs a three-component vector initialized with the specified \a x,
    * \a y and \a z component values.
    */
   template <typename T1>
   GenericVector( const T1& x, const T1& y, const T1& z )
   {
      m_data = new Data( 3 );
      iterator v = m_data->Begin();
      *v++ = component( x );
      *v++ = component( y );
      *v   = component( z );
   }

   /*!
    * Constructs a four-component vector initialized with the specified \a x,
    * \a y, \a z and \a t component values.
    */
   template <typename T1>
   GenericVector( const T1& x, const T1& y, const T1& z, const T1& t )
   {
      m_data = new Data( 4 );
      iterator v = m_data->Begin();
      *v++ = component( x );
      *v++ = component( y );
      *v++ = component( z );
      *v   = component( t );
   }

   /*!
    * Copy constructor. This object references the same data that is being
    * referenced by the specified vector \a x.
    */
   GenericVector( const GenericVector& x )
      : m_data( x.m_data )
   {
      m_data->Attach();
   }

   /*!
    * Move constructor.
    */
   GenericVector( GenericVector&& x )
      : m_data( x.m_data )
   {
      x.m_data = nullptr;
   }

   /*!
    * Destroys a %GenericVector object. This destructor dereferences the vector
    * data. If the vector data becomes unreferenced, it is destroyed and
    * deallocated immediately.
    */
   virtual ~GenericVector()
   {
      if ( m_data != nullptr )
      {
         DetachFromData();
         m_data = nullptr;
      }
   }

   /*!
    * Deallocates vector data, yielding an empty vector.
    */
   void Clear()
   {
      if ( !IsEmpty() )
         if ( m_data->IsUnique() )
            m_data->Deallocate();
         else
         {
            Data* newData = new Data( 0 );
            DetachFromData();
            m_data = newData;
         }
   }

   /*!
    * Copy assignment operator. Returns a reference to this object.
    *
    * See the Assign() member function for more information.
    */
   GenericVector& operator =( const GenericVector& x )
   {
      Assign( x );
      return *this;
   }

   /*!
    * Assigns a vector \a x to this object.
    *
    * If this instance and the specified source instance \a x reference
    * different vector data, the data previously referenced by this object is
    * dereferenced. If the previous data becomes unreferenced, it is destroyed
    * and deallocated. Then the data being referenced by \a x is also
    * referenced by this object.
    *
    * If this instance and the specified source instance \a x already reference
    * the same vector data, then this function does nothing.
    */
   void Assign( const GenericVector& x )
   {
      x.m_data->Attach();
      DetachFromData();
      m_data = x.m_data;
   }

   /*!
    * Move assignment operator. Returns a reference to this object.
    *
    * See the Transfer() member function for more information.
    */
   GenericVector& operator =( GenericVector&& x )
   {
      Transfer( x );
      return *this;
   }

   /*!
    * Transfers data from another vector \a x to this object.
    *
    * Decrements the reference counter of the current vector data. If the data
    * becomes unreferenced, it is destroyed and deallocated. The vector data
    * referenced by the source object \a x is then transferred to this object,
    * and the source object \a x is left as an invalid empty vector.
    *
    * \warning The source vector \a x will be an invalid object after calling
    * this function, and hence should be destroyed immediately. Any attempt to
    * access an invalid object will most likely lead to a crash.
    */
   void Transfer( GenericVector& x )
   {
      DetachFromData();
      m_data = x.m_data;
      x.m_data = nullptr;
   }

   /*!
    * Transfers data from another vector \a x to this object.
    *
    * Decrements the reference counter of the current vector data. If the data
    * becomes unreferenced, it is destroyed and deallocated. The vector data
    * referenced by the source object \a x is then transferred to this object,
    * and the source object \a x is left as an invalid empty vector.
    *
    * \warning The source vector \a x will be an invalid object after calling
    * this function, and hence should be destroyed immediately. Any attempt to
    * access an invalid object will most likely lead to a crash.
    */
   void Transfer( GenericVector&& x )
   {
      DetachFromData();
      m_data = x.m_data;
      x.m_data = nullptr;
   }

   /*!
    * Exchanges two vectors \a x1 and \a x2.
    *
    * This function is efficient because it simply swaps the internal vector
    * data pointers owned by the objects.
    */
   friend void Swap( GenericVector& x1, GenericVector& x2 ) noexcept
   {
      pcl::Swap( x1.m_data, x2.m_data );
   }

   /*!
    * Assigns a constant scalar \a x to all components of this vector. Returns
    * a reference to this object.
    *
    * Before assigning a constant value to all vector components, this function
    * ensures that this instance uniquely references its vector data,
    * generating a new vector data block if necessary.
    */
   GenericVector& operator =( const scalar& x )
   {
      if ( !IsUnique() )
      {
         Data* newData = new Data( m_data->Length() );
         DetachFromData();
         m_data = newData;
      }
            iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
      PCL_IVDEP
      for ( ; i < j; ++i )
         *i = x;
      return *this;
   }

   /*!
    * Vector addition/assignment. Adds each component of the specified vector
    * \a x to the corresponding component of this vector. Returns a reference
    * to this object.
    *
    * Before operating vector components, this function ensures that this
    * instance uniquely references its vector data, generating a duplicate if
    * necessary.
    *
    * If the specified vectors are incompatible for component-wise operations
    * (because the length of \a x is less than the length of this object), this
    * function throws an appropriate Error exception.
    */
   GenericVector& operator +=( const GenericVector& x )
   {
      if ( x.Length() < Length() )
         throw Error( "Invalid vector addition." );
      EnsureUnique();
            iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
      const_iterator __restrict__ k = x.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         *i += *k;
      return *this;
   }

   /*!
    * Vector subtraction/assignment. Subtracts each component of the specified
    * vector \a x from the corresponding component of this vector. Returns a
    * reference to this object.
    *
    * Before operating vector components, this function ensures that this
    * instance uniquely references its vector data, generating a duplicate if
    * necessary.
    *
    * If the specified vectors are incompatible for component-wise operations
    * (because the length of \a x is less than the length of this object), this
    * function throws an appropriate Error exception.
    */
   GenericVector& operator -=( const GenericVector& x )
   {
      if ( x.Length() < Length() )
         throw Error( "Invalid vector subtraction." );
      EnsureUnique();
            iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
      const_iterator __restrict__ k = x.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         *i -= *k;
      return *this;
   }

   /*!
    * Component-wise vector multiplication/assignment. Multiplies each
    * component of this vector by the corresponding component of the specified
    * vector \a x. Returns a reference to this object.
    *
    * Before operating vector components, this function ensures that this
    * instance uniquely references its vector data, generating a duplicate if
    * necessary.
    *
    * If the specified vectors are incompatible for component-wise operations
    * (because the length of \a x is less than the length of this object), this
    * function throws an appropriate Error exception.
    */
   GenericVector& operator *=( const GenericVector& x )
   {
      if ( x.Length() < Length() )
         throw Error( "Invalid vector multiplication." );
      EnsureUnique();
            iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
      const_iterator __restrict__ k = x.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         *i *= *k;
      return *this;
   }

   /*!
    * Component-wise vector division/assignment. Divides each component of this
    * vector by the corresponding component of the specified vector \a x.
    * Returns a reference to this object.
    *
    * Before operating vector components, this function ensures that this
    * instance uniquely references its vector data, generating a duplicate if
    * necessary.
    *
    * If the specified vectors are incompatible for component-wise operations
    * (because the length of \a x is less than the length of this object), this
    * function throws an appropriate Error exception.
    *
    * \note Make sure that no component of the specified divisor vector \a x is
    * zero or insignificant, or calling this member function will lead to
    * division(s) by zero.
    */
   GenericVector& operator /=( const GenericVector& x )
   {
      if ( x.Length() < Length() )
         throw Error( "Invalid vector division." );
      EnsureUnique();
            iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
      const_iterator __restrict__ k = x.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         *i /= *k;
      return *this;
   }

#define IMPLEMENT_SCALAR_ASSIGN_OP( op )                                      \
      if ( IsUnique() )                                                       \
      {                                                                       \
               iterator __restrict__ i = m_data->Begin();                     \
         const_iterator __restrict__ j = m_data->End();                       \
         PCL_IVDEP                                                            \
         for ( ; i < j; ++i )                                                 \
            *i op##= x;                                                       \
      }                                                                       \
      else                                                                    \
      {                                                                       \
         Data* newData = new Data( m_data->Length() );                        \
         const_iterator __restrict__ i = m_data->Begin();                     \
         const_iterator __restrict__ j = m_data->End();                       \
               iterator __restrict__ k = newData->Begin();                    \
         PCL_IVDEP                                                            \
         for ( ; i < j; ++i, ++k )                                            \
            *k = *i op x;                                                     \
         DetachFromData();                                                    \
         m_data = newData;                                                    \
      }                                                                       \
      return *this;

   /*!
    * Adds a constant scalar \a x to all components of this vector. Returns a
    * reference to this object.
    *
    * Before adding a constant value to all vector components, this function
    * ensures that this instance uniquely references its vector data,
    * generating a duplicate if necessary.
    */
   GenericVector& operator +=( const scalar& x )
   {
      IMPLEMENT_SCALAR_ASSIGN_OP( + )
   }

   /*!
    * Subtracts a constant scalar \a x from all components of this vector.
    * Returns a reference to this object.
    *
    * Before subtracting a constant value from all vector components, this
    * function ensures that this instance uniquely references its vector data,
    * generating a duplicate if necessary.
    */
   GenericVector& operator -=( const scalar& x )
   {
      IMPLEMENT_SCALAR_ASSIGN_OP( - )
   }

   /*!
    * Multiplies all components of this vector by a constant scalar \a x.
    * Returns a reference to this object.
    *
    * Before multiplying all vector components by a constant value, this
    * function ensures that this instance uniquely references its vector data,
    * generating a duplicate if necessary.
    */
   GenericVector& operator *=( const scalar& x )
   {
      IMPLEMENT_SCALAR_ASSIGN_OP( * )
   }

   /*!
    * Divides all components of this vector by a constant scalar \a x. Returns
    * a reference to this object.
    *
    * Before dividing all vector components by a constant value, this function
    * ensures that this instance uniquely references its vector data,
    * generating a duplicate if necessary.
    */
   GenericVector& operator /=( const scalar& x )
   {
      IMPLEMENT_SCALAR_ASSIGN_OP( / )
   }

   /*!
    * Raises all components of this vector to a constant scalar \a x. Returns
    * a reference to this object.
    *
    * Before raising all vector components to a constant value, this function
    * ensures that this instance uniquely references its vector data,
    * generating a duplicate if necessary.
    */
   GenericVector& operator ^=( const scalar& x )
   {
      if ( IsUnique() )
      {
               iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
         PCL_IVDEP
         for ( ; i < j; ++i )
            *i = pcl::Pow( *i, x );
      }
      else
      {
         Data* newData = new Data( m_data->Length() );
         const_iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
               iterator __restrict__ k = newData->Begin();
         PCL_IVDEP
         for ( ; i < j; ++i, ++k )
            *k = pcl::Pow( *i, x );
         DetachFromData();
         m_data = newData;
      }
      return *this;
   }

#undef IMPLEMENT_SCALAR_ASSIGN_OP

   /*!
    * Returns the dot product of this vector and a vector \a v.
    *
    * \note For performance reasons, this function does not check whether the
    * specified vector has at least the same number of components as this
    * vector. If that condition does not hold, this function will invoke
    * undefined behavior.
    */
   double Dot( const GenericVector& v ) const noexcept
   {
      PCL_PRECONDITION( v.Length() >= Length() )
      double r = 0;
      const_iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
      const_iterator __restrict__ k = v.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         r += double( *i ) * double( *k );
      return r;
   }

   /*!
    * Returns the cross product of this vector and a vector \a v2.
    *
    * \note The cross product is only defined for vectors of three components.
    * For performance reasons, this function does not check vector lengths. For
    * vectors of length < 3, this function will invoke undefined behavior.
    */
   GenericVector Cross( const GenericVector& v2 ) const
   {
      PCL_PRECONDITION( Length() == 3 && v2.Length() == 3 )
      component x1 = *At( 0 );
      component y1 = *At( 1 );
      component z1 = *At( 2 );
      component x2 = v2[0];
      component y2 = v2[1];
      component z2 = v2[2];
      return GenericVector( y1*z2 - z1*y2,
                            z1*x2 - x1*z2,
                            x1*y2 - y1*x2 );
   }

   /*!
    * Returns the square of this vector. The result is a new vector of the same
    * length where each component is the square of its counterpart in this
    * vector.
    */
   GenericVector Sqr() const
   {
      GenericVector R( m_data->Length() );
      const_iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
            iterator __restrict__ k = R.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         *k = *i * *i;
      return R;
   }

   /*!
    * Replaces all components of this vector with their squares.
    *
    * Before performing its task, this function ensures that this instance
    * uniquely references its vector data, generating a duplicate if necessary.
    */
   void SetSqr()
   {
      if ( IsUnique() )
      {
               iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
         PCL_IVDEP
         for ( ; i < j; ++i )
            *i *= *i;
      }
      else
      {
         Data* newData = new Data( m_data->Length() );
         const_iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
               iterator __restrict__ k = newData->Begin();
         PCL_IVDEP
         for ( ; i < j; ++i, ++k )
            *k = *i * *i;
         DetachFromData();
         m_data = newData;
      }
   }

   /*!
    * Returns the square root of this vector. The result is a new vector of the
    * same length where each component is the square root of its counterpart in
    * this vector.
    */
   GenericVector Sqrt() const
   {
      GenericVector R( m_data->Length() );
      const_iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
            iterator __restrict__ k = R.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         *k = pcl::Sqrt( *i );
      return R;
   }

   /*!
    * Replaces all components of this vector with their square roots.
    *
    * Before performing its task, this function ensures that this instance
    * uniquely references its vector data, generating a duplicate if necessary.
    */
   void SetSqrt()
   {
      if ( IsUnique() )
      {
               iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
         for ( ; i < j; ++i )
            *i = pcl::Sqrt( *i );
      }
      else
      {
         Data* newData = new Data( m_data->Length() );
         const_iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
               iterator __restrict__ k = newData->Begin();
         PCL_IVDEP
         for ( ; i < j; ++i, ++k )
            *k = pcl::Sqrt( *i );
         DetachFromData();
         m_data = newData;
      }
   }

   /*!
    * Returns the absolute value of this vector. The result is a new vector of
    * the same length where each component is the absolute value of its
    * counterpart in this vector.
    */
   GenericVector Abs() const
   {
      GenericVector R( m_data->Length() );
      const_iterator __restrict__ i = m_data->Begin();
      const_iterator __restrict__ j = m_data->End();
            iterator __restrict__ k = R.Begin();
      PCL_IVDEP
      for ( ; i < j; ++i, ++k )
         *k = pcl::Abs( *i );
      return R;
   }

   /*!
    * Replaces all components of this vector with their absolute values.
    *
    * Before performing its task, this function ensures that this instance
    * uniquely references its vector data, generating a duplicate if necessary.
    */
   void SetAbs()
   {
      if ( IsUnique() )
      {
               iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
         PCL_IVDEP
         for ( ; i < j; ++i )
            *i = pcl::Abs( *i );
      }
      else
      {
         Data* newData = new Data( m_data->Length() );
         const_iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
               iterator __restrict__ k = newData->Begin();
         PCL_IVDEP
         for ( ; i < j; ++i, ++k )
            *k = pcl::Abs( *i );
         DetachFromData();
         m_data = newData;
      }
   }

   /*!
    * Returns the norm of this vector. For any real p > 0, the norm N of a
    * vector v is given by:
    *
    * N = sum( abs( x )^p )^(1/p)
    *
    * for all vector components x of v.
    *
    * \sa L1Norm(), L2Norm()
    */
   double Norm( double p ) const noexcept
   {
      return pcl::Norm( m_data->Begin(), m_data->End(), p );
   }

   /*!
    * Returns the L1 norm (or Manhattan norm) of this vector. The L1 norm is
    * the sum of the absolute values of all vector components.
    */
   double L1Norm() const noexcept
   {
      return pcl::L1Norm( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the L2 norm (or Euclidean norm) of this vector. The L2 norm is
    * the square root of the sum of squared vector components.
    */
   double L2Norm() const noexcept
   {
      return pcl::L2Norm( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the L2 norm (or Euclidean norm) of this vector. This function is
    * a synonym for L2Norm().
    */
   double Norm() const noexcept
   {
      return L2Norm();
   }

   /*!
    * Returns a unit vector with the same direction as this vector. A unit
    * vector has a norm (or magnitude) of 1.
    */
   GenericVector Unit() const
   {
      GenericVector R( *this );
      double N = L2Norm();
      if ( 1 + N > 1 )
         R /= N;
      return R;
   }

   /*!
    * Transforms this vector to a unit vector with the same direction. A unit
    * vector has a norm (or magnitude) of 1.
    */
   void SetUnit()
   {
      double N = L2Norm();
      if ( 1 + N > 1 )
         (void)operator /=( N );
   }

   /*!
    * Sorts the components of this vector in ascending order.
    */
   void Sort()
   {
      EnsureUnique();
      pcl::Sort( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns a sorted copy of this vector.
    */
   GenericVector Sorted() const
   {
      GenericVector R( *this );
      R.Sort();
      return R;
   }

   /*!
    * Sorts the components of this vector in reverse (descending) order.
    */
   void ReverseSort()
   {
      EnsureUnique();
      pcl::Sort( m_data->Begin(), m_data->End(),
                 []( const scalar& a, const scalar& b ){ return b < a; } );
   }

   /*!
    * Returns a reverse sorted copy of this vector.
    */
   GenericVector ReverseSorted() const
   {
      GenericVector R( *this );
      R.ReverseSort();
      return R;
   }

   /*!
    * Sorts the components of this vector in ascending order. Ordering of
    * vector components is defined such that for any pair a, b of vector
    * components, the specified binary predicate p( a, b ) is true if a
    * precedes b.
    */
   template <class BP>
   void Sort( BP p )
   {
      EnsureUnique();
      pcl::Sort( m_data->Begin(), m_data->End(), p );
   }

   /*!
    * Returns a sorted copy of this vector, where ordering of vector components
    * is defined by the specified binary predicate \a p. See Sort( BP p ).
    */
   template <class BP>
   GenericVector Sorted( BP p ) const
   {
      GenericVector R( *this );
      R.Sort( p );
      return R;
   }

   /*!
    * Returns the index of the first vector element with the specified value
    * \a x, or -1 if this vector does not contain such value.
    */
   int Find( const component& x ) const noexcept
   {
      const_iterator p = pcl::LinearSearch( m_data->Begin(), m_data->End(), x );
      return (p != m_data->End()) ? int( p - m_data->Begin() ) : -1;
   }

   /*!
    * Returns the index of the first vector element with the specified value
    * \a x, or -1 if this vector does not contain such value.
    * This function is an alias to Find().
    */
   int FindFirst( const component& x ) const noexcept
   {
      return Find( x );
   }

   /*!
    * Returns the index of the last vector element with the specified value
    * \a x, or -1 if this vector does not contain such value.
    */
   int FindLast( const component& x ) const noexcept
   {
      const_iterator p = pcl::LinearSearchLast( m_data->Begin(), m_data->End(), x );
      return (p != m_data->End()) ? int( p - m_data->Begin() ) : -1;
   }

   /*!
    * Returns true iff this vector contains the specified value \a x.
    */
   bool Contains( const component& x ) const noexcept
   {
      return pcl::LinearSearch( m_data->Begin(), m_data->End(), x ) != m_data->End();
   }

#ifndef __PCL_NO_VECTOR_STATISTICS

   /*!
    * Returns the index of the smallest vector component.
    *
    * For empty vectors, this function returns zero. For vectors where the
    * smallest component occurs more than once, this function returns the index
    * of the first occurrence.
    */
   int IndexOfSmallestComponent() const noexcept
   {
      return int( pcl::MinItem( m_data->Begin(), m_data->End() ) - m_data->Begin() );
   }

   /*!
    * Returns the index of the largest vector component.
    *
    * For empty vectors, this function returns zero. For vectors where the
    * largest component occurs more than once, this function returns the index
    * of the first occurrence.
    */
   int IndexOfLargestComponent() const noexcept
   {
      return int( pcl::MaxItem( m_data->Begin(), m_data->End() ) - m_data->Begin() );
   }

   /*!
    * Returns the index of the last occurrence of the smallest vector
    * component.
    *
    * For empty vectors, this function returns zero. For vectors where the
    * smallest component occurs more than once, this function returns the index
    * of the last occurrence.
    */
   int IndexOfLastSmallestComponent() const noexcept
   {
      iterator i = m_data->Begin();
      if ( m_data->Length() > 0 )
         for ( iterator j = m_data->Begin(); ++j < m_data->End(); )
            if ( *j <= *i )
               i = j;
      return i - m_data->Begin();
   }

   /*!
    * Returns the index of the last occurrence of the largest vector
    * component.
    *
    * For empty vectors, this function returns zero. For vectors where the
    * largest component occurs more than once, this function returns the index
    * of the last occurrence.
    */
   int IndexOfLastLargestComponent() const noexcept
   {
      iterator i = m_data->Begin();
      if ( m_data->Length() > 0 )
         for ( iterator j = m_data->Begin(); ++j < m_data->End(); )
            if ( *i <= *j )
               i = j;
      return i - m_data->Begin();
   }

   /*!
    * Returns the index of the smallest nonzero vector component.
    *
    * For empty vectors, this function returns zero. For vectors where the
    * smallest nonzero component occurs more than once, this function returns
    * the index of the first occurrence.
    */
   int IndexOfSmallestNonzeroComponent() const noexcept
   {
      iterator i = m_data->Begin();
      if ( m_data->Length() > 0 )
         for ( iterator j = m_data->Begin(); ++j < m_data->End(); )
            if ( *j != component( 0 ) )
               if ( *j < *i )
                  i = j;
      return i - m_data->Begin();
   }

   /*!
    * Returns the index of the last occurrence of the smallest nonzero vector
    * component.
    *
    * For empty vectors, this function returns zero. For vectors where the
    * smallest nonzero component occurs more than once, this function returns
    * the index of the last occurrence.
    */
   int IndexOfLastSmallestNonzeroComponent() const noexcept
   {
      iterator i = m_data->Begin();
      if ( m_data->Length() > 0 )
         for ( iterator j = m_data->Begin(); ++j < m_data->End(); )
            if ( *j != component( 0 ) )
               if ( *j <= *i )
                  i = j;
      return i - m_data->Begin();
   }

   /*!
    * Returns the value of the smallest vector component.
    * For empty vectors, this function returns zero conventionally.
    */
   component MinComponent() const noexcept
   {
      if ( m_data->Length() > 0 )
         return *pcl::MinItem( m_data->Begin(), m_data->End() );
      return component( 0 );
   }

   /*!
    * Returns the value of the largest vector component.
    * For empty vectors, this function returns zero conventionally.
    */
   component MaxComponent() const noexcept
   {
      if ( m_data->Length() > 0 )
         return *pcl::MaxItem( m_data->Begin(), m_data->End() );
      return component( 0 );
   }

   /*!
    * Returns the \a kth order statistic of the sample in this vector.
    *
    * Before selecting the specified vector component, this function ensures
    * that this instance uniquely references its vector data, generating a
    * duplicate if necessary.
    *
    * The specified index \a k must be valid, that is, it must be in the range
    * [0,Length()-1]. If a value of \a k outside the valid range is specified,
    * or equivalently, if this function is called for an empty vector, this
    * member function will invoke undefined behavior.
    *
    * \note This function uses a \e destructive selection algorithm: it alters
    * the order in the sequence of vector components. For a nondestructive
    * version, see the const version of this member function.
    */
   component OrderStatistic( int k )
   {
      PCL_PRECONDITION( !IsEmpty() )
      PCL_PRECONDITION( k >= 0 && k < m_data->Length() )
      EnsureUnique();
      return *pcl::Select( m_data->Begin(), m_data->End(), k );
   }

   /*!
    * Returns the \a kth order statistic of the sample in this vector, without
    * modifying this instance.
    *
    * The specified index \a k must be valid, that is, it must be in the range
    * [0,Length()-1]. If a value of \a k outside the valid range is specified,
    * or equivalently, if this function is called for an empty vector, this
    * member function will invoke undefined behavior.
    *
    * This is a \e nondestructive selection routine that doesn't modify the
    * order of existing vector components. To achieve that goal, this routine
    * simply generates a temporary working vector as a copy of this object,
    * then calls its OrderStatistic() member function to obtain the function's
    * return value.
    */
   component OrderStatistic( int k ) const
   {
      return GenericVector( *this ).OrderStatistic( k );
   }

   /*!
    * Returns the sum of vector components.
    * For empty vectors, this function returns zero.
    */
   double Sum() const noexcept
   {
      return pcl::Sum( m_data->Begin(), m_data->End() );
   }

   /*!
    * Computes the sum of vector components using a numerically stable
    * summation algorithm to minimize roundoff error.
    *
    * For empty vectors, this function returns zero.
    */
   double StableSum() const noexcept
   {
      return pcl::StableSum( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the sum of the absolute values of all vector components.
    * For empty vectors, this function returns zero.
    */
   double Modulus() const noexcept
   {
      return pcl::Modulus( m_data->Begin(), m_data->End() );
   }

   /*!
    * Computes the sum of the absolute values of all vector components using a
    * numerically stable summation algorithm to minimize roundoff error.
    *
    * For empty vectors, this function returns zero.
    */
   double StableModulus() const noexcept
   {
      return pcl::StableModulus( m_data->Begin(), m_data->End() );
   }

   /*!
    * Computes the sum of the squares of all vector components.
    * For empty vectors, this function returns zero.
    */
   double SumOfSquares() const noexcept
   {
      return pcl::SumOfSquares( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the sum of the squares of all vector components using a
    * numerically stable summation algorithm to minimize roundoff error.
    *
    * For empty vectors, this function returns zero.
    */
   double StableSumOfSquares() const noexcept
   {
      return pcl::StableSumOfSquares( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the mean of the values in this vector.
    * For empty vectors, this function returns zero.
    */
   double Mean() const
   {
      return pcl::Mean( m_data->Begin(), m_data->End() );
   }

   /*!
    * Computes the mean of the values in this vector using a numerically stable
    * summation algorithm to minimize roundoff error.
    *
    * For empty vectors, this function returns zero.
    */
   double StableMean() const noexcept
   {
      return pcl::StableMean( m_data->Begin(), m_data->End() );
   }

   /*!
    * Computes the two-sided, asymmetric trimmed mean of the values in this
    * vector. See pcl::TrimmedMean() for a complete description of the
    * implemented algorithm with information on function parameters.
    *
    * For empty vectors, this function returns zero.
    */
   double TrimmedMean( distance_type l = 1, distance_type h = 1 ) const noexcept
   {
      return pcl::TrimmedMean( m_data->Begin(), m_data->End(), l, h );
   }

   /*!
    * Computes the two-sided, asymmetric trimmed mean of the squared values in
    * this vector. See pcl::TrimmedMeanOfSquares() for a complete description
    * of the implemented algorithm with information on function parameters.
    *
    * For empty vectors, this function returns zero.
    */
   double TrimmedMeanOfSquares( distance_type l = 1, distance_type h = 1 ) const noexcept
   {
      return pcl::TrimmedMeanOfSquares( m_data->Begin(), m_data->End(), l, h );
   }

   /*!
    * Returns the variance from the mean for the values in this vector.
    *
    * For vectors with less than two components, this function returns zero.
    */
   double Variance() const noexcept
   {
      return pcl::Variance( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the standard deviation from the mean for the values in this
    * vector.
    *
    * For vectors with less than two components, this function returns zero.
    */
   double StdDev() const noexcept
   {
      return pcl::StdDev( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the median of the values in this vector.
    *
    * For vectors of length < 2, this function returns zero conventionally.
    */
   double Median() const
   {
      return pcl::Median( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the average absolute deviation with respect to the specified
    * \a center value.
    *
    * When the median of the vector elements is used as the center value, this
    * function returns the average absolute deviation from the median, which is
    * a well-known estimator of dispersion.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \note To make the average absolute deviation about the median consistent
    * with the standard deviation of a normal distribution, it must be
    * multiplied by the constant 1.2533.
    */
   double AvgDev( double center ) const noexcept
   {
      return pcl::AvgDev( m_data->Begin(), m_data->End(), center );
   }

   /*!
    * Computes the average absolute deviation with respect to the specified
    * \a center value, using a numerically stable summation algorithm to
    * minimize roundoff error.
    *
    * When the median of the vector elements is used as the center value, this
    * function returns the average absolute deviation from the median, which is
    * a well-known estimator of dispersion.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \note To make the average absolute deviation about the median consistent
    * with the standard deviation of a normal distribution, it must be
    * multiplied by the constant 1.2533.
    */
   double StableAvgDev( double center ) const noexcept
   {
      return pcl::StableAvgDev( m_data->Begin(), m_data->End(), center );
   }

   /*!
    * Returns the average absolute deviation from the median.
    *
    * The mean absolute deviation from the median is a well-known estimator of
    * dispersion.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \note To make the average absolute deviation about the median consistent
    * with the standard deviation of a normal distribution, it must be
    * multiplied by the constant 1.2533.
    */
   double AvgDev() const
   {
      return pcl::AvgDev( m_data->Begin(), m_data->End() );
   }

   /*!
    * Computes the average absolute deviation from the median using a
    * numerically stable summation algorithm to minimize roundoff error.
    *
    * The mean absolute deviation from the median is a well-known estimator of
    * dispersion.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \note To make the average absolute deviation about the median consistent
    * with the standard deviation of a normal distribution, it must be
    * multiplied by the constant 1.2533.
    */
   double StableAvgDev() const
   {
      return pcl::StableAvgDev( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the two-sided average absolute deviation with respect to the
    * specified \a center value.
    *
    * See AvgDev( double ) for more information.
    */
   TwoSidedEstimate TwoSidedAvgDev( double center ) const noexcept
   {
      return pcl::TwoSidedAvgDev( m_data->Begin(), m_data->End(), center );
   }

   /*!
    * Returns the two-sided average absolute deviation from the median.
    *
    * See AvgDev() for more information.
    */
   TwoSidedEstimate TwoSidedAvgDev() const
   {
      return pcl::TwoSidedAvgDev( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the median absolute deviation (MAD) with respect to the specified
    * \a center value.
    *
    * The MAD is a well-known robust estimator of scale.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \note To make the MAD estimator consistent with the standard deviation of
    * a normal distribution, it must be multiplied by the constant 1.4826.
    */
   double MAD( double center ) const
   {
      return pcl::MAD( m_data->Begin(), m_data->End(), center );
   }

   /*!
    * Returns the median absolute deviation from the median (MAD).
    *
    * The MAD is a well-known robust estimator of scale.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \note To make the MAD estimator consistent with the standard deviation of
    * a normal distribution, it must be multiplied by the constant 1.4826.
    */
   double MAD() const
   {
      return pcl::MAD( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the two-sided median absolute deviation (MAD) with respect to the
    * specified \a center value.
    *
    * See MAD( double ) for more information.
    */
   TwoSidedEstimate TwoSidedMAD( double center ) const
   {
      return pcl::TwoSidedMAD( m_data->Begin(), m_data->End(), center );
   }

   /*!
    * Returns the two-sided median absolute deviation from the median (MAD).
    *
    * See MAD() for more information.
    */
   TwoSidedEstimate TwoSidedMAD() const
   {
      return pcl::TwoSidedMAD( m_data->Begin(), m_data->End() );
   }

   /*!
    * Returns the biweight midvariance (BWMV).
    *
    * \param center  Reference center value. Normally, the median of the vector
    *                components should be used.
    *
    * \param sigma   A reference estimate of dispersion. Normally, the median
    *                absolute deviation from the median (MAD) of the vector
    *                components should be used.
    *
    * \param k       Rejection limit in sigma units. The default value is k=9.
    *
    * \param reducedLength    If true, reduce the sample size to exclude
    *                rejected vector components. This tends to approximate the
    *                true dispersion of the data more accurately for relatively
    *                small samples, or samples with large amounts of outliers.
    *                Note that this departs from the standard definition of
    *                biweight midvariance, where the total number of data items
    *                is used to scale the variance estimate. If false, use the
    *                full vector length, including rejected components, which
    *                gives a standard biweight midvariance estimate.
    *
    * The square root of the biweight midvariance is a robust estimator of
    * scale. It is an efficient estimator with respect to many statistical
    * distributions (about 87% Gaussian efficiency), and appears to have a
    * breakdown point close to 0.5 (the same as MAD).
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \b References
    *
    * Rand R. Wilcox (2017), <em>Introduction to Robust Estimation and
    * Hypothesis Testing, 4th Edition</em>, Elsevier Inc., Section 3.12.1.
    */
   double BiweightMidvariance( double center, double sigma, int k = 9, bool reducedLength = false ) const noexcept
   {
      return pcl::BiweightMidvariance( m_data->Begin(), m_data->End(), center, sigma, k, reducedLength );
   }

   /*!
    * Returns the biweight midvariance (BWMV) with respect to the median and
    * the median absolute deviation from the median (MAD).
    *
    * \param k       Rejection limit in sigma units. The default value is k=9.
    *
    * \param reducedLength    If true, reduce the sample size to exclude
    *                rejected vector components. This tends to approximate the
    *                true dispersion of the data more accurately for relatively
    *                small samples, or samples with large amounts of outliers.
    *                Note that this departs from the standard definition of
    *                biweight midvariance, where the total number of data items
    *                is used to scale the variance estimate. If false, use the
    *                full vector length, including rejected components, which
    *                gives a standard biweight midvariance estimate.
    *
    * The square root of the biweight midvariance is a robust estimator of
    * scale. It is an efficient estimator with respect to many statistical
    * distributions (about 87% Gaussian efficiency), and appears to have a
    * breakdown point close to 0.5 (the same as MAD).
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \b References
    *
    * Rand R. Wilcox (2017), <em>Introduction to Robust Estimation and
    * Hypothesis Testing, 4th Edition</em>, Elsevier Inc., Section 3.12.1.
    */
   double BiweightMidvariance( int k = 9, bool reducedLength = false ) const
   {
      double center = Median();
      return BiweightMidvariance( center, MAD( center ), k, reducedLength );
   }

   /*!
    * Returns the two-sided biweight midvariance (BWMV).
    *
    * \param center  Reference center value. Normally, the median of the vector
    *                components should be used.
    *
    * \param sigma   A two-sided reference estimate of dispersion. Normally,
    *                the two-sided median absolute deviation from the median
    *                (MAD) of the vector components should be used. See the
    *                TwoSidedMAD() member function.
    *
    * See BiweightMidvariance( double, double, int, bool ) for more information
    * on the rest of parameters and references.
    */
   TwoSidedEstimate TwoSidedBiweightMidvariance( double center, const TwoSidedEstimate& sigma,
                                                 int k = 9, bool reducedLength = false ) const noexcept
   {
      return pcl::TwoSidedBiweightMidvariance( m_data->Begin(), m_data->End(), center, sigma, k, reducedLength );
   }

   /*!
    * Returns the two-sided biweight midvariance (BWMV) with respect to the
    * median and the two-sided median absolute deviation from the median (MAD).
    *
    * See BiweightMidvariance( int, bool ) for more information and references.
    */
   TwoSidedEstimate TwoSidedBiweightMidvariance( int k = 9, bool reducedLength = false ) const
   {
      double center = Median();
      return TwoSidedBiweightMidvariance( center, TwoSidedMAD( center ), k, reducedLength );
   }

   /*!
    * Returns a percentage bend midvariance (PBMV).
    *
    * \param center  Reference center value. Normally, the median of the vector
    *                components should be used.
    *
    * \param beta    Rejection parameter in the [0,0.5] range. Higher values
    *                improve robustness to outliers (i.e., increase the
    *                breakdown point of the estimator) at the expense of a
    *                lower efficiency. The default value is beta=0.2.
    *
    * The square root of the percentage bend midvariance is a robust estimator
    * of scale. With the default beta=0.2, its Gaussian efficiency is 67%. With
    * beta=0.1, its efficiency is 85% but its breakdown is only 0.1.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \b References
    *
    * Rand R. Wilcox (2017), <em>Introduction to Robust Estimation and
    * Hypothesis Testing, 4th Edition</em>, Elsevier Inc., Section 3.12.3.
    */
   double BendMidvariance( double center, double beta = 0.2 ) const
   {
      return pcl::BendMidvariance( m_data->Begin(), m_data->End(), center, beta );
   }

   /*!
    * Returns a percentage bend midvariance (PBMV) with respect to the median.
    *
    * \param beta    Rejection parameter in the [0,0.5] range. Higher values
    *                improve robustness to outliers (i.e., increase the
    *                breakdown point of the estimator) at the expense of a
    *                lower efficiency. The default value is beta=0.2.
    *
    * The square root of the percentage bend midvariance is a robust estimator
    * of scale. With the default beta=0.2, its Gaussian efficiency is 67%. With
    * beta=0.1, its efficiency is 85% but its breakdown is only 0.1.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * \b References
    *
    * Rand R. Wilcox (2017), <em>Introduction to Robust Estimation and
    * Hypothesis Testing, 4th Edition</em>, Elsevier Inc., Section 3.12.3.
    */
   double BendMidvariance( double beta = 0.2 ) const
   {
      return pcl::BendMidvariance( m_data->Begin(), m_data->End(), Median(), beta );
   }

   /*!
    * Returns the Sn scale estimator of Rousseeuw and Croux:
    *
    * <pre>
    * Sn = c * low_median( high_median( |x_i - x_j| ) )
    * </pre>
    *
    * where low_median() is the order statistic of rank (n + 1)/2, and
    * high_median() is the order statistic of rank n/2 + 1.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * The constant c = 1.1926 must be used to make the Sn estimator converge to
    * the standard deviation of a pure normal distribution. However, this
    * implementation does not apply it (it uses c=1 implicitly), for
    * consistency with other implementations of scale estimators.
    *
    * \b References
    *
    * P.J. Rousseeuw and C. Croux (1993), <em>Alternatives to the Median
    * Absolute Deviation,</em> Journal of the American Statistical Association,
    * Vol. 88, pp. 1273-1283.
    */
   double Sn() const
   {
      GenericVector v( *this );
      return pcl::Sn( v.Begin(), v.End() );
   }

   /*!
    * Returns the Qn scale estimator of Rousseeuw and Croux:
    *
    * <pre>
    * Qn = c * first_quartile( |x_i - x_j| : i < j )
    * </pre>
    *
    * where first_quartile() is the order statistic of rank (n + 1)/4.
    *
    * For vectors with less than two components, this function returns zero.
    *
    * The constant c = 2.2219 must be used to make the Qn estimator converge to
    * the standard deviation of a pure normal distribution. However, this
    * implementation does not apply it (it uses c=1 implicitly), for consistency
    * with other implementations of scale estimators.
    *
    * \b References
    *
    * P.J. Rousseeuw and C. Croux (1993), <em>Alternatives to the Median
    * Absolute Deviation,</em> Journal of the American Statistical Association,
    * Vol. 88, pp. 1273-1283.
    */
   double Qn() const
   {
      GenericVector v( *this );
      return pcl::Qn( v.Begin(), v.End() );
   }

#endif   // !__PCL_NO_VECTOR_STATISTICS

   /*!
    * Returns a 64-bit non-cryptographic hash value computed for this vector.
    *
    * This function calls pcl::Hash64() for the internal vector data.
    *
    * The \a seed parameter can be used to generate repeatable hash values. It
    * can also be set to a random value in compromised environments.
    */
   uint64 Hash64( uint64 seed = 0 ) const noexcept
   {
      return pcl::Hash64( m_data->Begin(), m_data->Size(), seed );
   }

   /*!
    * Returns a 32-bit non-cryptographic hash value computed for this vector.
    *
    * This function calls pcl::Hash32() for the internal vector data.
    *
    * The \a seed parameter can be used to generate repeatable hash values. It
    * can also be set to a random value in compromised environments.
    */
   uint32 Hash32( uint32 seed = 0 ) const noexcept
   {
      return pcl::Hash32( m_data->Begin(), m_data->Size(), seed );
   }

   /*!
    * Returns a non-cryptographic hash value computed for this vector. This
    * function is a synonym for Hash64().
    */
   uint64 Hash( uint64 seed = 0 ) const noexcept
   {
      return Hash64( seed );
   }

   /*!
    * Returns true iff this instance uniquely references its vector data.
    */
   bool IsUnique() const noexcept
   {
      return m_data->IsUnique();
   }

   /*!
    * Returns true iff this instance references (shares) the same vector data as
    * another instance \a x.
    */
   bool IsAliasOf( const GenericVector& x ) const noexcept
   {
      return m_data == x.m_data;
   }

   /*!
    * Ensures that this instance uniquely references its vector data.
    *
    * If necessary, this member function generates a duplicate of the vector
    * data, references it, and then decrements the reference counter of the
    * original vector data.
    */
   void EnsureUnique()
   {
      if ( !IsUnique() )
      {
         Data* newData = new Data( m_data->Length() );
         const_iterator __restrict__ i = m_data->Begin();
         const_iterator __restrict__ j = m_data->End();
               iterator __restrict__ k = newData->Begin();
         PCL_IVDEP
         for ( ; i < j; ++i, ++k )
            *k = *i;
         DetachFromData();
         m_data = newData;
      }
   }

   /*!
    * Returns the number of components in this vector. If this object is an
    * empty vector, this member function returns zero.
    */
   int Length() const noexcept
   {
      return m_data->Length();
   }

   /*!
    * Returns the total number of bytes required to store the data contained in
    * this vector.
    */
   size_type Size() const noexcept
   {
      return m_data->Size();
   }

   /*!
    * Returns true only if this vector is valid. A vector is valid if it
    * references an internal vector structure, even if it is an empty vector.
    *
    * In general, all %GenericVector objects are valid with only two
    * exceptions:
    *
    * \li Objects that have been move-copied or move-assigned to other vectors.
    * \li Objects that have been invalidated explicitly by calling Transfer().
    *
    * An invalid vector object cannot be used and should be destroyed
    * immediately. Invalid vectors are always destroyed automatically during
    * move construction and move assignment operations.
    */
   bool IsValid() const noexcept
   {
      return m_data != nullptr;
   }

   /*!
    * Returns true iff this is an empty vector. An empty vector has no
    * components, and hence its length is zero.
    */
   bool IsEmpty() const noexcept
   {
      return Length() == 0;
   }

   /*!
    * Returns true iff this vector is not empty. This operator is equivalent to:
    *
    * \code !IsEmpty(); \endcode
    */
   operator bool() const noexcept
   {
      return !IsEmpty();
   }

   /*!
    * Equality operator. Returns true if this vector is equal to another vector
    * \a x. Two vectors are equal if both have the same length and identical
    * component values.
    */
   bool operator ==( const GenericVector& x ) const noexcept
   {
      return IsAliasOf( x ) || SameLength( x ) && pcl::Equal( Begin(), x.Begin(), x.End() );
   }

   /*!
    * Less than relational operator. Returns true if this vector is less than
    * another vector \a x.
    *
    * In this operator, vector comparisons are performed component-wise,
    * irrespective of vector lengths, until either two vector components differ
    * or until the end of one of the vectors is reached. In the latter case,
    * the shortest vector is the lesser one.
    */
   bool operator <( const GenericVector& x ) const noexcept
   {
      return !IsAliasOf( x ) && pcl::Compare( Begin(), End(), x.Begin(), x.End() ) < 0;
   }

   /*!
    * Returns true iff this vector has the same length as other vector \a x.
    */
   bool SameLength( const GenericVector& x ) const noexcept
   {
      return Length() == x.Length();
   }

   /*!
    * Returns a mutable vector iterator pointing to the \a i-th component of
    * this vector.
    *
    * Before returning, this function ensures that this instance uniquely
    * references its vector data.
    */
   iterator At( int i )
   {
      EnsureUnique();
      return m_data->At( i );
   }

   /*!
    * Returns an immutable vector iterator pointing to the \a i-th component of
    * this vector.
    */
   const_iterator At( int i ) const noexcept
   {
      return m_data->At( i );
   }

   /*!
    * Returns a mutable vector iterator pointing to the first vector component,
    * i.e. to the component at index 0 of this vector.
    *
    * All vector components are guaranteed to be stored at consecutive
    * locations addressable from the iterator returned by this function.
    *
    * Before returning, this function ensures that this instance uniquely
    * references its vector data.
    */
   iterator Begin()
   {
      EnsureUnique();
      return m_data->Begin();
   }

   /*!
    * Returns an immutable vector iterator pointing to the first vector
    * component, i.e. to the component at index 0 of this vector.
    *
    * All vector components are guaranteed to be stored at consecutive
    * locations addressable from the iterator returned by this function.
    */
   const_iterator Begin() const noexcept
   {
      return m_data->Begin();
   }

   /*!
    * A synonym for Begin() const.
    */
   const_iterator ConstBegin() const noexcept
   {
      return Begin();
   }

   /*!
    * Returns a mutable vector iterator pointing to the first vector component,
    * i.e. to the component at index 0 of this vector.
    *
    * This member function is a convenience alias to Begin().
    */
   iterator operator *()
   {
      return Begin();
   }

   /*!
    * Returns an immutable vector iterator pointing to the first vector
    * component, i.e. to the component at index 0 of this vector.
    *
    * This member function is a convenience alias to Begin() const.
    */
   const_iterator operator *() const noexcept
   {
      return Begin();
   }

   /*!
    * Returns a mutable iterator past the last vector component, i.e. a
    * pointer to a (nonexistent) component located at index=Length().
    *
    * All vector components are guaranteed to be stored in reverse order at
    * consecutive locations addressable from the iterator returned by this
    * function.
    *
    * Before returning, this function ensures that this instance uniquely
    * references its vector data.
    */
   iterator End()
   {
      EnsureUnique();
      return m_data->End();
   }

   /*!
    * Returns an immutable iterator past the last vector component, i.e. a
    * pointer to a (nonexistent) component located at index=Length().
    *
    * All vector components are guaranteed to be stored in reverse order at
    * consecutive locations addressable from the iterator returned by this
    * function.
    */
   const_iterator End() const noexcept
   {
      return m_data->End();
   }

   /*!
    * A synonym for End() const.
    */
   const_iterator ConstEnd() const noexcept
   {
      return End();
   }

   /*!
    * Returns a reference to the mutable vector component at the specified
    * index \a i. Vector indices are relative to zero.
    *
    * Before returning, this function ensures that this instance uniquely
    * references its vector data.
    */
   component& operator []( int i )
   {
      return *At( i );
   }

   /*!
    * Returns a reference to the immutable vector component at the specified
    * index \a i. Vector indices are relative to zero.
    */
   const component& operator []( int i ) const noexcept
   {
      return *At( i );
   }

   /*!
    * Returns a pointer to the first component in this vector.
    *
    * The returned pointer can be used as a common C array, where all vector
    * components are guaranteed to be stored consecutively starting from the
    * pointer returned by this function.
    *
    * This member function does nothing to ensure that this instance uniquely
    * references its vector data. Consequently, the caller must take into
    * account that all modifications made to vector components accessed through
    * the value returned by this function will apply to all instances sharing
    * the same vector data.
    *
    * This function can be used to perform fast operations on vector components
    * avoiding the overhead caused by deep copies of vector data, when such
    * copies are not necessary. Typically this happens when two or more threads
    * work simultaneously on non-overlapping regions of the same vector.
    */
   iterator DataPtr() noexcept
   {
      return m_data->v;
   }

   /*!
    * Returns a pointer to the \a ith component in this vector.
    *
    * All vector components are guaranteed to be stored at consecutive
    * locations addressable from the iterator returned by this function.
    *
    * This member function does not ensure that the data referenced by this
    * vector is unique. See DataPtr() for more information on how to use this
    * member function.
    */
   iterator ComponentPtr( int i ) noexcept
   {
      return m_data->At( i );
   }

#ifndef __PCL_NO_STL_COMPATIBLE_ITERATORS
   /*!
    * STL-compatible iteration. Equivalent to Begin().
    */
   iterator begin()
   {
      return Begin();
   }

   /*!
    * STL-compatible iteration. Equivalent to Begin() const.
    */
   const_iterator begin() const noexcept
   {
      return Begin();
   }

   /*!
    * STL-compatible iteration. Equivalent to End().
    */
   iterator end()
   {
      return End();
   }

   /*!
    * STL-compatible iteration. Equivalent to End() const.
    */
   const_iterator end() const noexcept
   {
      return End();
   }
#endif   // !__PCL_NO_STL_COMPATIBLE_ITERATORS

   /*!
    * Computes spherical coordinates from this three-component vector.
    *
    * \param[out] lon   Reference to a variable where the computed longitude
    *                   in radians, in the range [-pi,+pi], will be stored. If
    *                   the vector points to one pole, that is, if *lat is
    *                   either -pi/2 or +pi/2, the computed longitude will be
    *                   zero.
    *
    * \param[out] lat   Reference to a variable where the computed latitude
    *                   in radians, in the range [-pi/2,+pi/2], will be stored.
    *
    * This function requires a vector with at least three coordinates. For
    * performance reasons, the vector length is not verified.
    *
    * The returned coordinates lie on a sphere of unit radius, that is, only
    * the direction of the input vector is taken into account, never its
    * magnitude.
    *
    * \sa ToSpherical2Pi(), FromSpherical()
    */
   template <typename T1, typename T2>
   void ToSpherical( T1& lon, T2& lat ) const noexcept
   {
      PCL_PRECONDITION( Length() >= 3 )
      double x = *At( 0 );
      double y = *At( 1 );
      double z = *At( 2 );
      double m2 = x*x + y*y;
      lon = T1( (m2 == 0) ? 0.0 : ArcTan( y, x ) );
      lat = T2( (z == 0)  ? 0.0 : ArcTan( z, pcl::Sqrt( m2 ) ) );
   }

   /*!
    * Computes spherical coordinates from this three-component vector.
    *
    * This function is identical to ToSpherical(), but the output longitude
    * component is normalized to the range [0,2pi) in radians.
    *
    * \sa ToSpherical(), FromSpherical()
    */
   template <typename T1, typename T2>
   void ToSpherical2Pi( T1& lon, T2& lat ) const noexcept
   {
      ToSpherical( lon, lat );
      if ( lon < 0 )
         lon += TwoPi();
   }

   /*!
    * Returns a three-component vector with rectangular coordinates computed
    * from spherical coordinates, given by their sines and cosines
    *
    * \param slon  Sine of the longitude.
    * \param clon  Cosine of the longitude.
    * \param slat  Sine of the latitude.
    * \param clat  Cosine of the latitude.
    *
    * The returned vector contains the <em>direction cosines</em> for the
    * specified position on the sphere. It is a unit vector (unit magnitude)
    * pointing from the center of the sphere to the specified location.
    *
    * \sa ToSpherical(), ToSpherical2Pi()
    */
   static GenericVector FromSpherical( double slon, double clon, double slat, double clat )
   {
      return GenericVector( clon*clat, slon*clat, slat );
   }

   /*!
    * Returns a three-component vector with rectangular coordinates computed
    * from the specified spherical coordinates.
    *
    * \param lon  Longitude in radians.
    * \param lat  Latitude in radians.
    *
    * The returned vector contains the <em>direction cosines</em> for the
    * specified position on the sphere. It is a unit vector (unit magnitude)
    * pointing from the center of the sphere to the specified location.
    *
    * \sa ToSpherical()
    */
   template <typename T1, typename T2>
   static GenericVector FromSpherical( const T1& lon, const T2& lat )
   {
      double slon, clon, slat, clat;
      SinCos( double( lon ), slon, clon );
      SinCos( double( lat ), slat, clat );
      return FromSpherical( slon, clon, slat, clat );
   }

   /*!
    * Returns the angle between this vector and another vector \a v, with both
    * vectors being defined in two-dimensional space.
    *
    * The returned value is the angle between both vectors in radians, in the
    * range [-pi,+pi].
    *
    * \note This member function is only defined for vectors of two components.
    * For performance reasons, this condition is not verified; if it doesn't
    * hold, this function will invoke undefined behavior.
    */
   double Angle2D( const GenericVector& v ) const noexcept
   {
      /*
       * https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors
       * https://stackoverflow.com/questions/243945/calculating-a-2d-vectors-cross-product
       */
      component x1 = *At( 0 );
      component y1 = *At( 1 );
      component x2 = v[0];
      component y2 = v[1];
      return ArcTan( x1*y2 - y1*x2, Dot( v ) );
   }

   /*!
    * Returns the angle between this vector and another vector \a v, with both
    * vectors being defined in three-dimensional space.
    *
    * The returned value is the angle between both vectors in radians, in the
    * range [0,pi].
    *
    * \note This member function is only defined for vectors of three
    * components. For performance reasons, this condition is not verified; if
    * it doesn't hold, this function will invoke undefined behavior.
    */
   double Angle3D( const GenericVector& v ) const noexcept
   {
      /*
       * https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors
       */
      return ArcTan( Cross( v ).L2Norm(), Dot( v ) );
   }

   /*!
    * Returns the angle between this vector and another vector \a v, being both
    * vectors defined in three-dimensional space, with sign determined by the
    * direction of an additional 3D vector \a n.
    *
    * The returned value is the angle between both vectors in radians, in the
    * range [-pi,+pi]. The angle is positive if this vector is clockwise with
    * respect to the direction of \a n, negative otherwise.
    *
    * \note This member function is only defined for vectors of three
    * components. For performance reasons, this condition is not verified; if
    * it doesn't hold, this function will invoke undefined behavior.
    */
   double Angle3D( const GenericVector& v, const GenericVector& n ) const
   {
      /*
       * https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors
       */
      GenericVector c = Cross( v );
      return ArcTan( (((n * c) >= 0) ? 1 : -1) * c.L2Norm(), Dot( v ) );
   }

   /*!
    * Generates a sequence of string tokens separated with the specified
    * \a separator string. Returns a reference to the target string \a s.
    *
    * For each vector component, this function appends a string representation
    * (known as a \e token) to the target string \a s. If the vector has more
    * than one component, successive tokens are separated with the specified
    * \a separator.
    *
    * The string type S must have a meaningful %Append() member function and
    * type conversion semantics to transform a vector component to a string.
    * The standard String and IsoString PCL classes provide the required
    * functionality for most scalar types, although it is probably better to
    * use String::ToSeparated() and IsoString::ToSeparated() instead of calling
    * these functions directly.
    */
   template <class S, typename SP>
   S& ToSeparated( S& s, SP separator ) const
   {
      const_iterator i = m_data->Begin(), j = m_data->End();
      if ( i < j )
      {
         s.Append( S( *i ) );
         if ( ++i < j )
            do
            {
               s.Append( separator );
               s.Append( S( *i ) );
            }
            while ( ++i < j );
      }
      return s;
   }

   /*!
    * Generates a sequence of string tokens separated with the specified
    * \a separator string by calling an \a append function. Returns a reference
    * to the target string \a s.
    *
    * For each vector component x, this function appends a string
    * representation (known as a \e token) to the target string \a s by calling
    * the \a append function:
    *
    *\code append( s, S( x ) ); \endcode
    *
    * If the vector has more than one component, successive tokens are
    * separated by calling:
    *
    * \code append( s, S( separator ) ); \endcode
    *
    * The string type S must have type conversion semantics to transform a
    * vector component to a string. The standard String and IsoString PCL
    * classes provide the required functionality for most scalar types,
    * although it is probably better to use String::ToSeparated() and
    * IsoString::ToSeparated() instead of calling these functions directly.
    */
   template <class S, typename SP, class AF>
   S& ToSeparated( S& s, SP separator, AF append ) const
   {
      const_iterator i = m_data->Begin(), j = m_data->End();
      if ( i < j )
      {
         append( s, S( *i ) );
         if ( ++i < j )
         {
            S p( separator );
            do
            {
               append( s, p );
               append( s, S( *i ) );
            }
            while ( ++i < j );
         }
      }
      return s;
   }

   /*!
    * Generates a comma-separated sequence of string tokens. Returns a
    * reference to the target string \a s.
    *
    * This function is equivalent to:
    *
    * \code ToSeparated( s, ',' ); \endcode
    */
   template <class S>
   S& ToCommaSeparated( S& s ) const
   {
      return ToSeparated( s, ',' );
   }

   /*!
    * Generates a space-separated sequence of string tokens. Returns a
    * reference to the target string \a s.
    *
    * This function is equivalent to:
    *
    * \code ToSeparated( s, ' ' ); \endcode
    */
   template <class S>
   S& ToSpaceSeparated( S& s ) const
   {
      return ToSeparated( s, ' ' );
   }

   /*!
    * Generates a tabulator-separated sequence of string tokens. Returns a
    * reference to the target string \a s.
    *
    * This function is equivalent to:
    *
    * \code ToSeparated( s, '\t' ); \endcode
    */
   template <class S>
   S& ToTabSeparated( S& s ) const
   {
      return ToSeparated( s, '\t' );
   }

private:

   /*!
    * \struct Data
    * \internal
    * Reference-counted vector data structure.
    */
   struct Data : public ReferenceCounter
   {
      int        n = 0;       //!< The vector length
      component* v = nullptr; //!< The vector components

      Data() = default;

      Data( int len )
      {
         if ( len > 0 )
            Allocate( len );
      }

      ~Data()
      {
         Deallocate();
      }

      int Length() const noexcept
      {
         return n;
      }

      size_type Size() const noexcept
      {
         return size_type( n )*sizeof( component );
      }

      iterator At( int i ) const noexcept
      {
         return v + i;
      }

      iterator Begin() const noexcept
      {
//          if ( likely( std::is_scalar<component>::value ) )
//             return reinterpret_cast<iterator>( PCL_ASSUME_ALIGNED_32( v ) );
         return v;
      }

      iterator End() const noexcept
      {
         return At( n );
      }

      void Allocate( int len )
      {
         n = len;
         if ( likely( std::is_scalar<component>::value ) )
         {
            v = reinterpret_cast<component*>( PCL_ALIGNED_MALLOC( Size(), 32 ) );
            if ( unlikely( v == nullptr ) )
            {
               n = 0;
               throw std::bad_alloc();
            }
         }
         else
            v = new component[ len ];
      }

      void Deallocate()
      {
         PCL_PRECONDITION( refCount == 0 )
         if ( likely( std::is_scalar<component>::value ) )
            PCL_ALIGNED_FREE( v );
         else
            delete [] v;
         v = nullptr;
         n = 0;
      }
   };

   /*!
    * \internal
    * The reference-counted vector data.
    */
   Data* m_data = nullptr;

   /*!
    * \internal
    * Dereferences vector data and disposes it if it becomes garbage.
    */
   void DetachFromData()
   {
      if ( !m_data->Detach() )
         delete m_data;
   }
};

// ----------------------------------------------------------------------------

/*!
 * \defgroup vector_operators Vector Operators
 *
 * This section includes scalar-to-vector and vector-to-vector arithmetic
 * operator functions that are not members of the GenericVector template class.
 */

/*!
 * Returns the sum of two vectors \a A and \a B.
 *
 * If the specified vectors are incompatible for vector addition (because \a B
 * has less components than \a A), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( const GenericVector<T>& A, const GenericVector<T>& B )
{
   int n = A.Length();
   if ( B.Length() < n )
      throw Error( "Invalid vector addition." );
   GenericVector<T> R( n );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   if ( likely( !A.IsAliasOf( B ) ) )
   {
      typename GenericVector<T>::const_iterator __restrict__ b = B.Begin();
      PCL_IVDEP
      for ( ; a < c; ++a, ++b, ++r )
         *r = *a + *b;
   }
   else
   {
      PCL_IVDEP
      for ( ; a < c; ++a, ++r )
         *r = *a + *a;
   }
   return R;
}

/*!
 * Returns the sum of two vectors \a A (rvalue reference) and \a B.
 *
 * If the specified vectors are incompatible for vector addition (because \a B
 * has less components than \a A), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( GenericVector<T>&& A, const GenericVector<T>& B )
{
   A += B;
   return std::move( A );
}

/*!
 * Returns the sum of two vectors \a A and \a B (rvalue reference).
 *
 * If the specified vectors are incompatible for vector addition (because \a B
 * has less components than \a A), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( const GenericVector<T>& A, GenericVector<T>&& B )
{
   B += A;
   return std::move( B );
}

/*!
 * Returns the sum of two vectors \a A (rvalue reference) and \a B (rvalue
 * reference).
 *
 * If the specified vectors are incompatible for vector addition (because \a B
 * has less components than \a A), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( GenericVector<T>&& A, GenericVector<T>&& B )
{
   A += B;
   return std::move( A );
}

/*!
 * Returns the sum of a vector \a A and a scalar \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( const GenericVector<T>& A, const T& x )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = *a + x;
   return R;
}

/*!
 * Returns the sum of a vector \a A (rvalue reference) and a scalar \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( GenericVector<T>&& A, const T& x )
{
   A += x;
   return std::move( A );
}

/*!
 * Returns the sum of a scalar \a x and a vector \a A.
 *
 * This function exists to implement the commutative property of
 * scalar-to-vector addition; it is equivalent to A + x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( const T& x, const GenericVector<T>& A )
{
   return A + x;
}

/*!
 * Returns the sum of a scalar \a x and a vector \a A (rvalue reference).
 *
 * This function exists to implement the commutative property of
 * scalar-to-vector addition; it is equivalent to A + x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator +( const T& x, GenericVector<T>&& A )
{
   A += x;
   return std::move( A );
}

// ----------------------------------------------------------------------------

/*!
 * Returns the result of subtracting a vector \a B from another vector \a A.
 *
 * If the specified vectors are incompatible for vector addition (because \a B
 * has less components than \a A), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( const GenericVector<T>& A, const GenericVector<T>& B )
{
   int n = A.Length();
   if ( B.Length() < n )
      throw Error( "Invalid vector subtraction." );

   GenericVector<T> R( n );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   if ( likely( !A.IsAliasOf( B ) ) )
   {
      typename GenericVector<T>::const_iterator __restrict__ b = B.Begin();
      PCL_IVDEP
      for ( ; a < c; ++a, ++b, ++r )
         *r = *a - *b;
   }
   else
   {
      PCL_IVDEP
      for ( ; a < c; ++a, ++r )
         *r = *a - *a;
   }
   return R;
}

/*!
 * Returns the result of subtracting a vector \a B from another vector \a A
 * (rvalue reference).
 *
 * If the specified vectors are incompatible for vector addition (because \a B
 * has less components than \a A), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( GenericVector<T>&& A, const GenericVector<T>& B )
{
   A -= B;
   return std::move( A );
}

/*!
 * Returns the result of subtracting a vector \a B (rvalue reference) from
 * another vector \a A.
 *
 * If the specified vectors are incompatible for vector addition (because \a A
 * has less components than \a B), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( const GenericVector<T>& A, GenericVector<T>&& B )
{
   if ( A.Length() < B.Length() )
      throw Error( "Invalid vector subtraction." );

   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::iterator       __restrict__ b = B.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = B.End();
   PCL_IVDEP
   for ( ; b < c; ++a, ++b )
      *b = *a - *b;
   return std::move( B );
}

/*!
 * Returns the result of subtracting a vector \a B (rvalue reference) from
 * another vector \a A (rvalue reference).
 *
 * If the specified vectors are incompatible for vector addition (because \a B
 * has less components than \a A), this function throws an Error exception.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( GenericVector<T>&& A, GenericVector<T>&& B )
{
   A -= B;
   return std::move( A );
}

/*!
 * Returns the subtraction of a scalar \a x from a vector \a A.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( const GenericVector<T>& A, const T& x )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = *a - x;
   return R;
}

/*!
 * Returns the subtraction of a scalar \a x from a vector \a A (rvalue
 * reference).
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( GenericVector<T>&& A, const T& x )
{
   A -= x;
   return std::move( A );
}

/*!
 * Returns the subtraction of a vector \a A from a scalar \a x.
 *
 * This function exists because scalar-to-vector subtraction is not a
 * commutative operation. A - x is not equal to x - A (the resulting vector
 * components have the same magnitudes but opposite signs).
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( const T& x, const GenericVector<T>& A )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = x - *a;
   return R;
}

/*!
 * Returns the subtraction of a vector \a A (rvalue reference) from a scalar
 * \a x.
 *
 * This function exists because scalar-to-vector subtraction is not a
 * commutative operation. A - x is not equal to x - A (the resulting vector
 * components have the same magnitudes but opposite signs).
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator -( const T& x, GenericVector<T>&& A )
{
   typename GenericVector<T>::iterator       __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a )
      *a = x - *a;
   return std::move( A );
}

// ----------------------------------------------------------------------------

/*!
 * Returns the cross product of two vectors \a A and \a B.
 *
 * \note The cross product is only defined for vectors of three components. For
 * performance reasons, this function does not check vector lengths. For
 * vectors of length < 3, this function will invoke undefined behavior.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( const GenericVector<T>& A, const GenericVector<T>& B )
{
   PCL_PRECONDITION( A.Length() == 3 && B.Length() == 3 )
   return GenericVector<T>( A[1]*B[2] - A[2]*B[1],
                            A[2]*B[0] - A[0]*B[2],
                            A[0]*B[1] - A[1]*B[0] );
}

/*!
 * Returns the cross product of two vectors \a A (rvalue reference) and \a B.
 *
 * \note The cross product is only defined for vectors of three components. For
 * performance reasons, this function does not check vector lengths. For
 * vectors of length < 3, this function will invoke undefined behavior.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( GenericVector<T>&& A, const GenericVector<T>& B )
{
   T x = A[1]*B[2] - A[2]*B[1],
     y = A[2]*B[0] - A[0]*B[2],
     z = A[0]*B[1] - A[1]*B[0];
   typename GenericVector<T>::iterator a = A.Begin();
   a[0] = x; a[1] = y; a[2] = z;
   return std::move( A );
}

/*!
 * Returns the cross product of two vectors \a A and \a B (rvalue reference).
 *
 * \note The cross product is only defined for vectors of three components. For
 * performance reasons, this function does not check vector lengths. For
 * vectors of length < 3, this function will invoke undefined behavior.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( const GenericVector<T>& A, GenericVector<T>&& B )
{
   T x = A[1]*B[2] - A[2]*B[1],
     y = A[2]*B[0] - A[0]*B[2],
     z = A[0]*B[1] - A[1]*B[0];
   typename GenericVector<T>::iterator b = B.Begin();
   b[0] = x; b[1] = y; b[2] = z;
   return std::move( B );
}

/*!
 * Returns the cross product of two vectors \a A (rvalue reference) and \a B
 * (rvalue reference).
 *
 * \note The cross product is only defined for vectors of three components. For
 * performance reasons, this function does not check vector lengths. For
 * vectors of length < 3, this function will invoke undefined behavior.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( GenericVector<T>&& A, GenericVector<T>&& B )
{
   T x = A[1]*B[2] - A[2]*B[1],
     y = A[2]*B[0] - A[0]*B[2],
     z = A[0]*B[1] - A[1]*B[0];
   typename GenericVector<T>::iterator a = A.Begin();
   a[0] = x; a[1] = y; a[2] = z;
   return std::move( A );
}

// ----------------------------------------------------------------------------

/*!
 * Returns the dot product of two vectors \a A and \a B.
 *
 * \note For performance reasons, this function does not check whether the
 * specified vectors have compatible lengths; in case \a B has less components
 * than \a A, this function will invoke undefined behavior.
 * \ingroup vector_operators
 */
template <typename T> inline
T operator *( const GenericVector<T>& A, const GenericVector<T>& B ) noexcept
{
   PCL_PRECONDITION( B.Length() >= A.Length() )
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   T r = T( 0 );
   if ( likely( !A.IsAliasOf( B ) ) )
   {
      typename GenericVector<T>::const_iterator __restrict__ b = B.Begin();
      PCL_IVDEP
      for ( ; a < c; ++a, ++b )
         r += *a * *b;
   }
   else
   {
      PCL_IVDEP
      for ( ; a < c; ++a )
         r += *a * *a;
   }
   return r;
}

// ----------------------------------------------------------------------------

/*!
 * Returns the product of a vector \a A by a scalar \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator *( const GenericVector<T>& A, const T& x )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = *a * x;
   return R;
}

/*!
 * Returns the product of a vector \a A (rvalue reference) by a scalar \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator *( GenericVector<T>&& A, const T& x )
{
   A *= x;
   return std::move( A );
}

/*!
 * Returns the product of a scalar \a x and a vector \a A.
 *
 * This function exists to implement the commutative property of
 * scalar-to-vector multiplication. It is equivalent to A * x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator *( const T& x, const GenericVector<T>& A )
{
   return A * x;
}

/*!
 * Returns the product of a scalar \a x and a vector \a A (rvalue reference).
 *
 * This function exists to implement the commutative property of
 * scalar-to-vector multiplication. It is equivalent to A * x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator *( const T& x, GenericVector<T>&& A )
{
   A *= x;
   return std::move( A );
}

// ----------------------------------------------------------------------------

/*!
 * Returns the result of dividing a vector \a A by a scalar \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator /( const GenericVector<T>& A, const T& x )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = *a / x;
   return R;
}

/*!
 * Returns the result of dividing a vector \a A (rvalue reference) by a scalar
 * \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator /( GenericVector<T>&& A, const T& x )
{
   A /= x;
   return std::move( A );
}

/*!
 * Returns the result of dividing a scalar \a x by a vector \a A.
 *
 * This function exists because scalar-to-vector division is not a
 * commutative operation. A/x is not equal to x/A.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator /( const T& x, const GenericVector<T>& A )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = x / *a;
   return R;
}

/*!
 * Returns the result of dividing a scalar \a x by a vector \a A (rvalue
 * reference).
 *
 * This function exists because scalar-to-vector division is not a
 * commutative operation. A/x is not equal to x/A.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator /( const T& x, GenericVector<T>&& A )
{
   typename GenericVector<T>::iterator       __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a )
      *a = x / *a;
   return std::move( A );
}

/*!
 * Returns the result of the element wise division of a vector \a A by another
 * vector \a B.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator /( const GenericVector<T>& A, const GenericVector<T>& B )
{
   int n = A.Length();
   if ( B.Length() < n )
      throw Error( "Invalid vector division." );

   GenericVector<T> R( n );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   if ( likely( !A.IsAliasOf( B ) ) )
   {
      typename GenericVector<T>::const_iterator __restrict__ b = B.Begin();
      PCL_IVDEP
      for ( ; a < c; ++a, ++b, ++r )
         *r = *a / *b;
   }
   else
   {
      PCL_IVDEP
      for ( ; a < c; ++a, ++r )
         *r = *a / *a;
   }
   return R;
}

/*!
 * Returns the result of the element-wise division of an r-value reference to a
 * vector \a A by a vector \a B.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator /( GenericVector<T>&& A, const GenericVector<T>& B )
{
   return A /= B;
}

// ----------------------------------------------------------------------------

/*!
 * Returns the result of raising a vector \a A to a scalar \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( const GenericVector<T>& A, const T& x )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = pcl::Pow( *a, x );
   return R;
}

/*!
 * Returns the result of raising a vector \a A (rvalue reference) to a scalar
 * \a x.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( GenericVector<T>&& A, const T& x )
{
   A ^= x;
   return std::move( A );
}

/*!
 * Returns the result of raising a scalar \a x to a vector \a A.
 *
 * This function exists because scalar-to-vector exponentiation is not a
 * commutative operation. A^x is not equal to x^A.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( const T& x, const GenericVector<T>& A )
{
   GenericVector<T> R( A.Length() );
   typename GenericVector<T>::iterator       __restrict__ r = R.Begin();
   typename GenericVector<T>::const_iterator __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a, ++r )
      *r = pcl::Pow( x, *a );
   return R;
}

/*!
 * Returns the result of raising a scalar \a x to a vector \a A (rvalue
 * reference).
 *
 * This function exists because scalar-to-vector exponentiation is not a
 * commutative operation. A^x is not equal to x^A.
 * \ingroup vector_operators
 */
template <typename T> inline
GenericVector<T> operator ^( const T& x, GenericVector<T>&& A )
{
   typename GenericVector<T>::iterator       __restrict__ a = A.Begin();
   typename GenericVector<T>::const_iterator __restrict__ c = A.End();
   PCL_IVDEP
   for ( ; a < c; ++a )
      *a = pcl::Pow( x, *a );
   return std::move( A );
}

// ----------------------------------------------------------------------------

#ifndef __PCL_NO_VECTOR_INSTANTIATE

/*!
 * \defgroup vector_types Vector Types
 */

/*!
 * \class pcl::I8Vector
 * \ingroup vector_types
 * \brief 8-bit signed integer vector.
 *
 * %I8Vector is a template instantiation of GenericVector for \c int8.
 */
typedef GenericVector<int8>         I8Vector;

/*!
 * \class pcl::CharVector
 * \ingroup vector_types
 * \brief 8-bit signed integer vector.
 *
 * %CharVector is an alias for I8Vector. It is a template instantiation of
 * GenericVector for \c int8.
 */
typedef I8Vector                    CharVector;

/*!
 * \class pcl::UI8Vector
 * \ingroup vector_types
 * \brief 8-bit unsigned integer vector.
 *
 * %UI8Vector is a template instantiation of GenericVector for \c uint8.
 */
typedef GenericVector<uint8>        UI8Vector;

/*!
 * \class pcl::ByteVector
 * \ingroup vector_types
 * \brief 8-bit unsigned integer vector.
 *
 * %ByteVector is an alias for UI8Vector. It is a template instantiation of
 * GenericVector for \c uint8.
 */
typedef UI8Vector                   ByteVector;

/*!
 * \class pcl::I16Vector
 * \ingroup vector_types
 * \brief 16-bit signed integer vector.
 *
 * %I16Vector is a template instantiation of GenericVector for \c int16.
 */
typedef GenericVector<int16>        I16Vector;

/*!
 * \class pcl::UI16Vector
 * \ingroup vector_types
 * \brief 16-bit unsigned integer vector.
 *
 * %UI16Vector is a template instantiation of GenericVector for \c uint16.
 */
typedef GenericVector<uint16>       UI16Vector;

/*!
 * \class pcl::I32Vector
 * \ingroup vector_types
 * \brief 32-bit signed integer vector.
 *
 * %I32Vector is a template instantiation of GenericVector for \c int32.
 */
typedef GenericVector<int32>        I32Vector;

/*!
 * \class pcl::IVector
 * \ingroup vector_types
 * \brief 32-bit signed integer vector.
 *
 * %IVector is an alias for I32Vector. It is a template instantiation of
 * GenericVector for \c int32.
 */
typedef I32Vector                   IVector;

/*!
 * \class pcl::UI32Vector
 * \ingroup vector_types
 * \brief 32-bit unsigned integer vector.
 *
 * %UI32Vector is a template instantiation of GenericVector for \c uint32.
 */
typedef GenericVector<uint32>       UI32Vector;

/*!
 * \class pcl::UIVector
 * \ingroup vector_types
 * \brief 32-bit unsigned integer vector.
 *
 * %UIVector is an alias for UI32Vector. It is a template instantiation of
 * GenericVector for \c uint32.
 */
typedef UI32Vector                  UIVector;

/*!
 * \class pcl::I64Vector
 * \ingroup vector_types
 * \brief 64-bit integer vector.
 *
 * %I64Vector is a template instantiation of GenericVector for \c int64.
 */
typedef GenericVector<int64>        I64Vector;

/*!
 * \class pcl::UI64Vector
 * \ingroup vector_types
 * \brief 64-bit unsigned integer vector.
 *
 * %UI64Vector is a template instantiation of GenericVector for \c uint64.
 */
typedef GenericVector<uint64>       UI64Vector;

/*!
 * \class pcl::SzVector
 * \ingroup vector_types
 * \brief size_type integer vector.
 *
 * %SzVector is a template instantiation of GenericVector for \c size_type.
 */
typedef GenericVector<size_type>    SzVector;

/*!
 * \class pcl::F32Vector
 * \ingroup vector_types
 * \brief 32-bit floating point real vector.
 *
 * %F32Vector is a template instantiation of GenericVector for \c float.
 */
typedef GenericVector<float>        F32Vector;

/*!
 * \class pcl::FVector
 * \ingroup vector_types
 * \brief 32-bit floating point real vector.
 *
 * %FVector is an alias for F32Vector. It is a template instantiation of
 * GenericVector for \c float.
 */
typedef F32Vector                   FVector;

/*!
 * \class pcl::F64Vector
 * \ingroup vector_types
 * \brief 64-bit floating point real vector.
 *
 * %F64Vector is a template instantiation of GenericVector for \c double.
 */
typedef GenericVector<double>       F64Vector;

/*!
 * \class pcl::DVector
 * \ingroup vector_types
 * \brief 64-bit floating point real vector.
 *
 * %DVector is an alias for F64Vector. It is a template instantiation of
 * GenericVector for \c double.
 */
typedef F64Vector                   DVector;

/*!
 * \class pcl::Vector
 * \ingroup vector_types
 * \brief 64-bit floating point real vector.
 *
 * %Vector is an alias for DVector. It is a template instantiation of
 * GenericVector for \c double.
 */
typedef DVector                     Vector;

/*!
 * \class pcl::C32Vector
 * \ingroup vector_types
 * \brief 32-bit floating point complex vector.
 *
 * %C32Vector is a template instantiation of GenericVector for \c Complex32.
 */
typedef GenericVector<Complex32>    C32Vector;

/*!
 * \class pcl::C64Vector
 * \ingroup vector_types
 * \brief 64-bit floating point complex vector.
 *
 * %C64Vector is a template instantiation of GenericVector for \c Complex64.
 */
typedef GenericVector<Complex64>    C64Vector;

#ifndef _MSC_VER

/*!
 * \class pcl::F80Vector
 * \ingroup vector_types
 * \brief 80-bit extended precision floating point real vector.
 *
 * %F80Vector is a template instantiation of GenericVector for \c long
 * \c double.
 *
 * \note This template instantiation is not available on Windows with Visual
 * C++ compilers.
 */
typedef GenericVector<long double>  F80Vector;

/*!
 * \class pcl::LDVector
 * \ingroup vector_types
 * \brief 80-bit extended precision floating point real vector.
 *
 * %LDVector is an alias for F80Vector. It is a template instantiation of
 * GenericVector for \c long \c double.
 *
 * \note This template instantiation is not available on Windows with Visual
 * C++ compilers.
 */
typedef F80Vector                   LDVector;

#endif   // !_MSC_VER

#endif   // !__PCL_NO_VECTOR_INSTANTIATE

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_Vector_h

// ----------------------------------------------------------------------------
// EOF pcl/Vector.h - Released 2020-12-17T15:46:28Z
