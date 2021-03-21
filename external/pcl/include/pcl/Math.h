//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/Math.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_Math_h
#define __PCL_Math_h

/// \file pcl/Math.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/Memory.h>
#include <pcl/Selection.h>
#include <pcl/Sort.h>
#include <pcl/Utility.h>

#include <cmath>
#include <cstdlib>
#include <limits>

#ifdef _MSC_VER
#  include <intrin.h> // for __cpuid()
#endif

#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __PCL_MACOSX )
#  define __PCL_HAVE_SSE2  1
#  include <emmintrin.h>
#endif

/*
 * sincos() is only available as a GNU extension:
 *
 * http://man7.org/linux/man-pages/man3/sincos.3.html
 *
 * Unfortunately, it is not part of the C++ standard library because of the
 * anachronic dependency on errno.
 */
#if !defined( _MSC_VER ) && !defined( __clang__ ) && defined( __GNUC__ )
#  define __PCL_HAVE_SINCOS 1
#endif

#ifdef __PCL_QT_INTERFACE
#  include <QtWidgets/QtWidgets>
#endif

/*
 * Number of histogram bins used by fast histogram-based median calculation
 * algorithm implementations.
 */
#define __PCL_MEDIAN_HISTOGRAM_LENGTH  8192

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \defgroup hw_identification_functions Hardware Identification Routines
 */

/*!
 * Returns an integer representing the highest set of Streaming SIMD Extensions
 * instructions (SSE) supported by the running processor. This function is a
 * portable wrapper to the CPUID x86 instruction.
 *
 * The returned value can be one of:
 *
 * <pre>
 *  0 : No SSE instructions supported
 *  1 : SSE instructions set supported
 *  2 : SSE2 instructions set supported
 *  3 : SSE3 instructions set supported
 * 41 : SSE4.1 instructions set supported
 * 42 : SSE4.2 instructions set supported
 * </pre>
 *
 * \ingroup hw_identification_functions
 */
inline int MaxSSEInstructionSetSupported() noexcept
{
   int32 edxFlags = 0;
   int32 ecxFlags = 0;

#ifdef _MSC_VER
   int cpuInfo[ 4 ];
   __cpuid( cpuInfo, 1 );
   edxFlags = cpuInfo[3];
   ecxFlags = cpuInfo[2];
#else
   asm volatile( "mov $0x00000001, %%eax\n\t"
                 "cpuid\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%ecx, %1\n"
                  : "=r" (edxFlags), "=r" (ecxFlags)  // output operands
                  :                                   // input operands
                  : "%eax", "%ebx", "%ecx", "%edx" ); // clobbered registers
#endif

   if ( ecxFlags & (1u << 20) ) // SSE4.2
      return 42;
   if ( ecxFlags & (1u << 19) ) // SSE4.1
      return 41;
   if ( ecxFlags & 1u )         // SSE3
      return 3;
   if ( edxFlags & (1u << 26) ) // SSE2
      return 2;
   if ( edxFlags & (1u << 25) ) // SSE
      return 1;
   return 0;
}

// ----------------------------------------------------------------------------

/*!
 * \defgroup fpclassification_functions Floating Point Number Classification
 */

#define __PCL_FLOAT_SGNMASK   0x80000000
#define __PCL_FLOAT_EXPMASK   0x7f800000
#define __PCL_FLOAT_SIGMASK   0x007fffff

/*!
 * Returns true iff the specified 32-bit floating point number is \e finite.
 * A number is finite if it is neither NaN (Not a Number) nor positive or
 * negative infinity.
 *
 * \ingroup fpclassification_functions
 * \sa IsNaN( float ), IsInfinity( float )
 */
inline bool IsFinite( float x ) noexcept
{
   union { float f; uint32 u; } v = { x };
   return (v.u & __PCL_FLOAT_EXPMASK) != __PCL_FLOAT_EXPMASK;
}

/*!
 * Returns true iff the specified 32-bit floating point number is \e NaN. A NaN
 * (Not A Number) is a special value in the IEEE 754 standard used to represent
 * an undefined or unrepresentable value, such as the result of invalid
 * operations like 0/0, or real operations with complex results as the square
 * root of a negative number.
 *
 * \ingroup fpclassification_functions
 * \sa IsFinite( float ), IsInfinity( float )
 */
inline bool IsNaN( float x ) noexcept
{
   union { float f; uint32 u; } v = { x };
   return (v.u & __PCL_FLOAT_EXPMASK) == __PCL_FLOAT_EXPMASK &&
          (v.u & __PCL_FLOAT_SIGMASK) != 0;
}

/*!
 * Returns a nonzero integer value if the specified 32-bit floating point
 * number is an \e infinity.
 *
 * This function returns:
 *
 * <pre>
 * +1 if \a x is <em>positive infinity</em>.
 * -1 if \a x is <em>negative infinity</em>.
 *  0 if \a x is either NaN or a finite value.
 * </pre>
 *
 * \ingroup fpclassification_functions
 * \sa IsFinite( float ), IsNaN( float )
 */
inline int IsInfinity( float x ) noexcept
{
   union { float f; uint32 u; } v = { x };
   if ( (v.u & __PCL_FLOAT_EXPMASK) == __PCL_FLOAT_EXPMASK &&
        (v.u & __PCL_FLOAT_SIGMASK) == 0 )
      return ((v.u & __PCL_FLOAT_SGNMASK) == 0) ? +1 : -1;
   return 0;
}

/*!
 * Returns true iff the specified 32-bit floating point number is a negative
 * zero.
 *
 * \ingroup fpclassification_functions
 */
inline bool IsNegativeZero( float x ) noexcept
{
   union { float f; uint32 u; } v = { x };
   return v.u == __PCL_FLOAT_SGNMASK;
}

#define __PCL_DOUBLE_SGNMASK  0x80000000
#define __PCL_DOUBLE_EXPMASK  0x7ff00000
#define __PCL_DOUBLE_SIGMASK  0x000fffff

/*!
 * Returns true iff the specified 64-bit floating point number is \e finite.
 * A number is finite if it is neither NaN (Not a Number) nor positive or
 * negative infinity.
 *
 * \ingroup fpclassification_functions
 * \sa IsNaN( double ), IsInfinity( double )
 */
inline bool IsFinite( double x ) noexcept
{
   union { double d; uint32 u[2]; } v = { x };
   return (v.u[1] & __PCL_DOUBLE_EXPMASK) != __PCL_DOUBLE_EXPMASK;
}

/*!
 * Returns true iff the specified 64-bit floating point number is \e NaN. A NaN
 * (Not A Number) is a special value in the IEEE 754 standard used to represent
 * an undefined or unrepresentable value, such as the result of invalid
 * operations like 0/0, or real operations with complex results as the square
 * root of a negative number.
 *
 * \ingroup fpclassification_functions
 * \sa IsFinite( double ), IsInfinity( double )
 */
inline bool IsNaN( double x ) noexcept
{
   union { double d; uint32 u[2]; } v = { x };
   return (v.u[1] & __PCL_DOUBLE_EXPMASK) == __PCL_DOUBLE_EXPMASK &&
          (v.u[0] != 0 || (v.u[1] & __PCL_DOUBLE_SIGMASK) != 0);
}

/*!
 * Returns a nonzero integer value if the specified 64-bit floating point
 * number is an \e infinity.
 *
 * This function returns:
 *
 * <pre>
 * +1 if \a x is <em>positive infinity</em>.
 * -1 if \a x is <em>negative infinity</em>.
 *  0 if \a x is either NaN or a finite value.
 * </pre>
 *
 * \ingroup fpclassification_functions
 * \sa IsFinite( double ), IsNaN( double )
 */
inline int IsInfinity( double x ) noexcept
{
   union { double d; uint32 u[2]; } v = { x };
   if ( v.u[0] == 0 &&
       (v.u[1] & __PCL_DOUBLE_SIGMASK) == 0 &&
       (v.u[1] & __PCL_DOUBLE_EXPMASK) == __PCL_DOUBLE_EXPMASK )
      return ((v.u[1] & __PCL_DOUBLE_SGNMASK) == 0) ? +1 : -1;
   return 0;
}

/*!
 * Returns true iff the specified 64-bit floating point number is a negative
 * zero.
 *
 * \ingroup fpclassification_functions
 */
inline bool IsNegativeZero( double x ) noexcept
{
   union { double d; uint32 u[2]; } v = { x };
   return v.u[1] == __PCL_DOUBLE_SGNMASK &&
          v.u[0] == 0;
}

// ----------------------------------------------------------------------------

/*!
 * \defgroup mathematical_functions Mathematical Functions
 */

/*!
 * Absolute value of \a x.
 * \ingroup mathematical_functions
 */
inline float Abs( float x ) noexcept
{
   return std::fabs( x );
}

/*!
 * \ingroup mathematical_functions
 */
inline double Abs( double x ) noexcept
{
   return std::fabs( x );
}

/*!
 * \ingroup mathematical_functions
 */
inline long double Abs( long double x ) noexcept
{
   return std::fabs( x );
}

/*!
 * \ingroup mathematical_functions
 */
inline signed int Abs( signed int x ) noexcept
{
   return ::abs( x );
}

/*!
 * \ingroup mathematical_functions
 */
#if defined( __PCL_MACOSX ) && defined( __clang__ ) // turn off warning due to broken cstdlib in Xcode
_Pragma("clang diagnostic push")
_Pragma("clang diagnostic ignored \"-Wabsolute-value\"")
#endif
inline signed long Abs( signed long x ) noexcept
{
   return ::abs( x );
}
#if defined( __PCL_MACOSX ) && defined( __clang__ )
_Pragma("clang diagnostic pop")
#endif

/*!
 * \ingroup mathematical_functions
 */
#if defined( _MSC_VER )
inline __int64 Abs( __int64 x ) noexcept
{
   return (x < 0) ? -x : +x;
}
#elif defined( __PCL_MACOSX ) && defined( __clang__ )
inline constexpr signed long long Abs( signed long long x ) noexcept
{
   return (x < 0) ? -x : +x;
}
#else
inline signed long long Abs( signed long long x ) noexcept
{
   return ::abs( x );
}
#endif

/*!
 * \ingroup mathematical_functions
 */
inline signed short Abs( signed short x ) noexcept
{
   return (signed short)::abs( int( x ) );
}

/*!
 * \ingroup mathematical_functions
 */
inline signed char Abs( signed char x ) noexcept
{
   return (signed char)::abs( int( x ) );
}

/*!
 * \ingroup mathematical_functions
 */
inline wchar_t Abs( wchar_t x ) noexcept
{
   return (wchar_t)::abs( int( x ) );
}

/*!
 * \ingroup mathematical_functions
 */
inline constexpr unsigned int Abs( unsigned int x ) noexcept
{
   return x;
}

/*!
 * \ingroup mathematical_functions
 */
inline constexpr unsigned long Abs( unsigned long x ) noexcept
{
   return x;
}

/*!
 * \ingroup mathematical_functions
 */
#ifdef _MSC_VER
inline unsigned __int64 Abs( unsigned __int64 x ) noexcept
{
   return x;
}
#else
inline constexpr unsigned long long Abs( unsigned long long x ) noexcept
{
   return x;
}
#endif

/*!
 * \ingroup mathematical_functions
 */
inline constexpr unsigned short Abs( unsigned short x ) noexcept
{
   return x;
}

/*!
 * \ingroup mathematical_functions
 */
inline constexpr unsigned char Abs( unsigned char x ) noexcept
{
   return x;
}

// ----------------------------------------------------------------------------

/*!
 * The pi constant: 3.141592...
 * \ingroup mathematical_functions
 */
inline constexpr long double Pi() noexcept
{
   return (long double)( 0.31415926535897932384626433832795029e+01L );
}

/*!
 * Twice the pi constant: 0.6283185...
 * \ingroup mathematical_functions
 */
inline constexpr long double TwoPi() noexcept
{
   return (long double)( 0.62831853071795864769252867665590058e+01L );
}

// ----------------------------------------------------------------------------

/*!
 * Merges a complex angle given by degrees and arcminutes into single degrees.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Angle( int d, T m ) noexcept
{
   return d + m/60;
}

/*!
 * Merges a complex angle given by degrees, arcminutes and arcseconds into
 * single degrees.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Angle( int d, int m, T s ) noexcept
{
   return Angle( d, m + s/60 );
}

// ----------------------------------------------------------------------------

/*!
 * Inverse cosine function (arccosine).
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcCos( T x ) noexcept
{
   PCL_PRECONDITION( T( -1 ) <= x && x <= T( 1 ) )
   return std::acos( x );
}

// ----------------------------------------------------------------------------

/*!
 * Inverse sine function (arcsine).
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcSin( T x ) noexcept
{
   PCL_PRECONDITION( T( -1 ) <= x && x <= T( 1 ) )
   return std::asin( x );
}

// ----------------------------------------------------------------------------

/*!
 * Inverse tangent function (arctangent).
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcTan( T x ) noexcept
{
   return std::atan( x );
}

// ----------------------------------------------------------------------------

/*!
 * Arctangent of y/x, result in the proper quadrant.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcTan( T y, T x ) noexcept
{
   return std::atan2( y, x );
}

// ----------------------------------------------------------------------------

/*!
 * Arctangent of y/x, proper quadrant, result in the interval [0,2pi).
 * \ingroup mathematical_functions
 */
template <typename T> inline T ArcTan2Pi( T y, T x ) noexcept
{
   T r = ArcTan( y, x );
   if ( r < 0 )
      r = static_cast<T>( r + TwoPi() );
   return r;
}

// ----------------------------------------------------------------------------

/*!
 * The ceil function: lowest integer >= x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Ceil( T x ) noexcept
{
   return std::ceil( x );
}

// ----------------------------------------------------------------------------

/*!
 * Cosine function.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Cos( T x ) noexcept
{
   return std::cos( x );
}

// ----------------------------------------------------------------------------

/*!
 * Hyperbolic Cosine function.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Cosh( T x ) noexcept
{
   return std::cosh( x );
}

// ----------------------------------------------------------------------------

/*!
 * Cotangent of x, equal to Cos(x)/Sin(x) or 1/Tan(x).
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Cotan( T x ) noexcept
{
   return T( 1 )/std::tan( x );
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from radians to degrees.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Deg( T x ) noexcept
{
   return static_cast<T>( 0.572957795130823208767981548141051700441964e+02L * x );
}

// ----------------------------------------------------------------------------

/*!
 * The exponential function e**x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Exp( T x ) noexcept
{
   return std::exp( x );
}

// ----------------------------------------------------------------------------

/*!
 * The floor function: highest integer <= x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Floor( T x ) noexcept
{
   return std::floor( x );
}

// ----------------------------------------------------------------------------

/*!
 * Fractional part of x.
 * The returned value is within (-1,+1), and has the same sign as x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Frac( T x ) noexcept
{
   return std::modf( x, &x );
}

// ----------------------------------------------------------------------------

/*!
 * Calculates base-2 mantissa and exponent.
 * The arguments \a m and \a p receive the values of the base-2 mantissa and
 * exponent, respectively, such that: 0.5 <= m < 1.0, x = m * 2**p
 * \ingroup mathematical_functions
 */
template <typename T> inline void Frexp( T x, T& m, int& p ) noexcept
{
   m = std::frexp( x, &p );
}

// ----------------------------------------------------------------------------

/*!
 * Haversine function:
 *
 * <tt>Hav( x ) = (1 - Cos( x ))/2</tt>
 *
 * The haversine is useful to work with tiny angles.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Hav( T x ) noexcept
{
   return (1 - Cos( x ))/2;
}

// ----------------------------------------------------------------------------

/*!
 * Calculates m * 2**p.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Ldexp( T m, int p ) noexcept
{
   return std::ldexp( m, p );
}

// ----------------------------------------------------------------------------

/*!
 * Natural (base e) logarithm of \a x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Ln( T x ) noexcept
{
   PCL_PRECONDITION( x >= 0 )
   return std::log( x );
}

// ----------------------------------------------------------------------------

struct PCL_CLASS FactorialCache
{
   constexpr static int s_cacheSize = 127;
   static const double  s_lut[ s_cacheSize+1 ];
};

/*!
 * The factorial of \a n &ge; 0.
 *
 * A static lookup table is used to speed up for \a n <= 127.
 *
 * \ingroup mathematical_functions
 */
inline double Factorial( int n ) noexcept
{
   PCL_PRECONDITION( n >= 0 )
   if ( n <= FactorialCache::s_cacheSize )
      return FactorialCache::s_lut[n];
   double x = FactorialCache::s_lut[FactorialCache::s_cacheSize];
   for ( int m = FactorialCache::s_cacheSize; ++m <= n; )
      x *= m;
   return x;
}

/*!
 * The natural logarithm of the factorial of \a n &ge; 0.
 *
 * For \a n <= 127 computes the natural logarithm of the factorial function
 * directly. For \a n > 127 computes a series approximation, so that the
 * function won't overflow even for very large arguments.
 *
 * \ingroup mathematical_functions
 */
inline double LnFactorial( int n ) noexcept
{
   PCL_PRECONDITION( n >= 0 )
   if ( n <= FactorialCache::s_cacheSize )
      return Ln( FactorialCache::s_lut[n] );
   double x = n + 1;
// return (x - 0.5)*Ln( x ) - x + 0.5*Ln( TwoPi() ) + 1/12/x - 1/(360*x*x*x);
   return (x - 0.5)*Ln( x ) - x + 0.91893853320467267 + 1/12/x - 1/(360*x*x*x);
}

/*!
 * \class Fact
 * \brief Factorial function.
 *
 * We use a static lookup table to speed up for \a n <= 127.
 *
 * Example of use:
 *
 * \code double factorialOfEight = Fact<double>()( 8 ); // = 40320 \endcode
 *
 * The implementation of this class is thread-safe.
 *
 * \deprecated This class is deprecated and subject to removal in a future
 * version of PCL. For newly produced code, use the pcl::Factorial() and
 * pcl::LnFactorial() functions instead.
 *
 * \ingroup mathematical_functions
 */
template <typename T> struct PCL_CLASS Fact : public FactorialCache
{
   /*!
    * Returns the factorial of \a n &ge; 0.
    */
   T operator()( int n ) const noexcept
   {
      PCL_PRECONDITION( n >= 0 )
      if ( n <= s_cacheSize )
         return T( s_lut[n] );
      double x = s_lut[s_cacheSize];
      for ( int m = s_cacheSize; ++m <= n; )
         x *= m;
      return T( x );
   }

   /*!
    * Returns the natural logarithm of the factorial of \a n &ge; 0. For
    * \a n <= 127 computes the natural logarithm of the factorial function
    * directly. For \a n > 127 computes a series approximation, so that the
    * function won't overflow even for very large arguments.
    */
   T Ln( int n ) const noexcept
   {
      PCL_PRECONDITION( n >= 0 )
      if ( n <= s_cacheSize )
         return T( pcl::Ln( s_lut[n] ) );
      double x = n + 1;
//    return T( (x - 0.5)*pcl::Ln( x ) - x + 0.5*pcl::Ln( TwoPi() ) + 1/12/x - 1/(360*x*x*x) );
      return T( (x - 0.5)*pcl::Ln( x ) - x + 0.91893853320467267 + 1/12/x - 1/(360*x*x*x) );
   }
};

// ----------------------------------------------------------------------------

/*!
 * Natural (base e) logarithm of two.
 * \ingroup mathematical_functions
 */
inline constexpr long double Ln2() noexcept
{
   return (long double)( 0.6931471805599453094172321214581766e+00L );
}

// ----------------------------------------------------------------------------

/*!
 * Base 10 Logarithm of x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Log( T x ) noexcept
{
   PCL_PRECONDITION( x >= 0 )
   return std::log10( x );
}

// ----------------------------------------------------------------------------

/*!
 * Base 10 Logarithm of two.
 * \ingroup mathematical_functions
 */
inline constexpr long double Log2() noexcept
{
   // Use the relation:
   //    log10(2) = ln(2)/ln(10)
   return (long double)( 0.3010299956639811952137388947244930416265e+00L );
}

// ----------------------------------------------------------------------------

/*!
 * Base 2 Logarithm of x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Log2( T x ) noexcept
{
   // Use the relation:
   //    log2(x) = ln(x)/ln(2)
   PCL_PRECONDITION( x >= 0 )
   return std::log( x )/Ln2();
}

// ----------------------------------------------------------------------------

/*!
 * Base 2 Logarithm of e.
 * \ingroup mathematical_functions
 */
inline constexpr long double Log2e() noexcept
{
   // Use the relation:
   //    log2(e) = 1/ln(2)
   return (long double)( 0.14426950408889634073599246810018920709799e+01L );
}

// ----------------------------------------------------------------------------

/*!
 * Base 2 Logarithm of ten.
 * \ingroup mathematical_functions
 */
inline constexpr long double Log2T() noexcept
{
   // Use the relation:
   //    log2(10) = 1/log(2)
   return (long double)( 0.33219280948873623478703194294893900118996e+01L );
}

// ----------------------------------------------------------------------------

/*!
 * Base \a n logarithm of \a x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T LogN( T n, T x ) noexcept
{
   PCL_PRECONDITION( x >= 0 )
   return std::log( x )/std::log( n );
}

// ----------------------------------------------------------------------------

/*!
 * Remainder of x/y.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Mod( T x, T y ) noexcept
{
   return std::fmod( x, y );
}

// ----------------------------------------------------------------------------

/*!
 * Horner's algorithm to evaluate the polynomial function with the specified
 * array \a c of \a n + 1 coefficients:
 *
 * <pre>
 * y = c[0] + c[1]*x + c[2]*x**2 + ... + c[n]*x**n
 * </pre>
 *
 * The array \a c of coefficients must provide contiguous storage for at least
 * \a n + 1 values of type T. The degree \a n must be >= 0; otherwise this
 * function invokes undefined behavior.
 *
 * \ingroup mathematical_functions
 */
template <typename T, typename C> inline T Poly( T x, C c, int n ) noexcept
{
   PCL_PRECONDITION( n >= 0 )
   T y;
   for ( c += n, y = *c; n > 0; --n )
   {
      y *= x;
      y += *--c;
   }
   return y;
}

/*!
 * Horner's algorithm to evaluate the polynomial function:
 *
 * <pre>
 * y = c[0] + c[1]*x + c[2]*x**2 + ... + c[n]*x**n
 * </pre>
 *
 * The specified coefficients initializer list \a c must not be empty;
 * otherwise this function invokes undefined behavior.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline T Poly( T x, std::initializer_list<T> c ) noexcept
{
   PCL_PRECONDITION( c.size() > 0 )
   return Poly( x, c.begin(), int( c.size() )-1 );
}

// ----------------------------------------------------------------------------

/*!
 * Sign function:
 *
 * <pre>
 * -1 if x &le; 0
 *  0 if x = 0
 * +1 if x &ge; 0
 * </pre>
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr int Sign( T x ) noexcept
{
   return (x < 0) ? -1 : ((x > 0) ? +1 : 0);
}

// ----------------------------------------------------------------------------

/*!
 * Sign character:
 *
 * <pre>
 * '-' if x &le; 0
 * ' ' if x = 0
 * '+' if x &ge; 0
 * </pre>
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr char SignChar( T x ) noexcept
{
   return (x < 0) ? '-' : ((x > 0) ? '+' : ' ');
}

// ----------------------------------------------------------------------------

/*!
 * Sine function
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Sin( T x ) noexcept
{
   return std::sin( x );
}

// ----------------------------------------------------------------------------

/*!
 * Hyperbolic Sine function.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Sinh( T x ) noexcept
{
   return std::sinh( x );
}

// ----------------------------------------------------------------------------

#ifdef __PCL_HAVE_SINCOS

inline void __pcl_sincos__( float x, float& sx, float& cx ) noexcept
{
   ::sincosf( x, &sx, &cx );
}

inline void __pcl_sincos__( double x, double& sx, double& cx ) noexcept
{
   ::sincos( x, &sx, &cx );
}

inline void __pcl_sincos__( long double x, long double& sx, long double& cx ) noexcept
{
   ::sincosl( x, &sx, &cx );
}

#endif

/*!
 * Sine and cosine of \a x.
 *
 * The arguments \a sx and \a cx will receive, respectively, the values of the
 * sine and cosine of x.
 *
 * If supported by the underlying standard math library, this function is
 * roughly twice as fast as calling Sin() and Cos() separately. For code that
 * spend a significant amount of time calculating sines and cosines, this
 * optimization is critical.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline void SinCos( T x, T& sx, T& cx ) noexcept
{
#ifdef __PCL_HAVE_SINCOS
   __pcl_sincos__( x, sx, cx );
#else
   sx = std::sin( x );
   cx = std::cos( x );
#endif
}

// ----------------------------------------------------------------------------

/*!
 * Integer and fractional parts of \a x.
 *
 * The arguments \a i and \a f receive, respectively, the integer (truncated)
 * part and the fractional part of \a x. It holds that \a x = \a i + \a f, i.e.
 * both \a i and \a f have the same sign as \a x.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline void Split( T x, T& i, T& f ) noexcept
{
   f = std::modf( x, &i );
}

// ----------------------------------------------------------------------------

/*!
 * Square root function.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Sqrt( T x ) noexcept
{
   return std::sqrt( x );
}

// ----------------------------------------------------------------------------

/*!
 * Tangent function.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Tan( T x ) noexcept
{
   return std::tan( x );
}

// ----------------------------------------------------------------------------

/*!
 * Hyperbolic Tangent function.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Tanh( T x ) noexcept
{
   return std::tanh( x );
}

// ----------------------------------------------------------------------------

/*!
 * Truncated integer part of \a x.
 * \ingroup mathematical_functions
 */
template <typename T> inline T Trunc( T x ) noexcept
{
   (void)std::modf( x, &x );
   return x;
}

// ----------------------------------------------------------------------------

#ifdef __PCL_HAVE_SSE2

inline int __pcl_trunci__( float x ) noexcept
{
   return _mm_cvtt_ss2si( _mm_load_ss( &x ) );
}

inline int __pcl_trunci__( double x ) noexcept
{
   return _mm_cvttsd_si32( _mm_load_sd( &x ) );
}

inline int __pcl_trunci__( long double x ) noexcept
{
   return int( x );
}

#endif

/*!
 * TruncInt function: Truncated integer part of \a x as a 32-bit signed
 * integer.
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int TruncInt( T x ) noexcept
{
   PCL_PRECONDITION( x >= int_min && x <= int_max )
#ifdef __PCL_NO_PERFORMANCE_CRITICAL_MATH_ROUTINES
   return int( x );
#else
# ifdef __PCL_HAVE_SSE2
   return __pcl_trunci__( x );
# else
   return int( x );
# endif
#endif
}

/*!
 * TruncI function: Truncated integer part of \a x as a 32-bit signed integer.
 * to a 32-bit signed integer.
 *
 * \deprecated Use TruncInt() in newly produced code.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int TruncI( T x ) noexcept
{
   return TruncInt( x );
}

#define TruncInt32( x ) TruncInt( x )
#define TruncI32( x )   TruncInt( x )

// ----------------------------------------------------------------------------

#ifdef __PCL_HAVE_SSE2

#if defined( __x86_64__ ) || defined( _M_X64 )

inline int64 __pcl_trunci64__( float x ) noexcept
{
   return _mm_cvttss_si64( _mm_load_ss( &x ) );
}

inline int64 __pcl_trunci64__( double x ) noexcept
{
   return _mm_cvttsd_si64( _mm_load_sd( &x ) );
}

#else

inline int64 __pcl_trunci64__( float x ) noexcept
{
   return int64( _mm_cvtt_ss2si( _mm_load_ss( &x ) ) );
}

inline int64 __pcl_trunci64__( double x ) noexcept
{
   return int64( x );
}

#endif

inline int64 __pcl_trunci64__( long double x ) noexcept
{
   return int64( x );
}

#endif // __PCL_HAVE_SSE2

/*!
 * TruncInt64 function: Truncated integer part of \a x as a 64-bit signed
 * integer.
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int64 TruncInt64( T x ) noexcept
{
   PCL_PRECONDITION( x >= int64_min && x <= int64_max )
#ifdef __PCL_NO_PERFORMANCE_CRITICAL_MATH_ROUTINES
   return int64( Trunc( x ) );
#else
# ifdef __PCL_HAVE_SSE2
   return __pcl_trunci64__( x );
# else
   return int64( Trunc( x ) );
# endif
#endif
}

/*!
 * TruncI64 function: Truncated integer part of \a x as a 64-bit signed
 * integer.
 *
 * \deprecated Use TruncInt64() in newly produced code.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int64 TruncI64( T x ) noexcept
{
   return TruncInt64( x );
}

// ----------------------------------------------------------------------------

/*!
 * General power function: \a x raised to \a y.
 *
 * When you know some of the arguments in advance, faster alternatives are:
 *
 * \li Use <tt>Pow10I\<T\>()( y )</tt> when x == 10 and y is an integer
 * \li Use <tt>PowI( x, y )</tt> when x != 10 and y is an integer
 * \li Use <tt>Pow10( y )</tt> when x == 10 and y is not an integer
 *
 * Otherwise, you can also use: <tt>Pow2( y*Log2( x ) )</tt>
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Pow( T x, T y ) noexcept
{
   PCL_PRECONDITION( y < T( int_max ) )
   return std::pow( x, y );
}

// ----------------------------------------------------------------------------

/*!
 * \class Pow10I
 * \brief Exponential function 10**n, \a n being a signed integer.
 *
 * Example of use:
 *
 * \code double x = Pow10I<double>()( 5 ); // x = 10^5 \endcode
 *
 * \ingroup mathematical_functions
 */
template <typename T> struct PCL_CLASS Pow10I
{
   T operator ()( int n ) const noexcept
   {
      // Use fast table lookups and squaring up to |n| <= 50.
      static const T lut[] =
      {
#define ___( x ) static_cast<T>( x )
         ___( 1.0e+00 ), ___( 1.0e+01 ), ___( 1.0e+02 ), ___( 1.0e+03 ), ___( 1.0e+04 ),
         ___( 1.0e+05 ), ___( 1.0e+06 ), ___( 1.0e+07 ), ___( 1.0e+08 ), ___( 1.0e+09 ),
         ___( 1.0e+10 ), ___( 1.0e+11 ), ___( 1.0e+12 ), ___( 1.0e+13 ), ___( 1.0e+14 ),
         ___( 1.0e+15 ), ___( 1.0e+16 ), ___( 1.0e+17 ), ___( 1.0e+18 ), ___( 1.0e+19 ),
         ___( 1.0e+20 ), ___( 1.0e+21 ), ___( 1.0e+22 ), ___( 1.0e+23 ), ___( 1.0e+24 ),
         ___( 1.0e+25 ), ___( 1.0e+26 ), ___( 1.0e+27 ), ___( 1.0e+28 ), ___( 1.0e+29 ),
         ___( 1.0e+30 ), ___( 1.0e+31 ), ___( 1.0e+32 ), ___( 1.0e+33 ), ___( 1.0e+34 ),
         ___( 1.0e+35 ), ___( 1.0e+36 ), ___( 1.0e+37 ), ___( 1.0e+38 ), ___( 1.0e+39 ),
         ___( 1.0e+40 ), ___( 1.0e+41 ), ___( 1.0e+42 ), ___( 1.0e+43 ), ___( 1.0e+44 ),
         ___( 1.0e+45 ), ___( 1.0e+46 ), ___( 1.0e+47 ), ___( 1.0e+48 ), ___( 1.0e+49 )
#undef ___
      };
      static const int N = ItemsInArray( lut );
      int i = ::abs( n );
      double x;
      if ( i < N )
         x = lut[i];
      else
      {
         x = lut[N-1];
         while ( (i -= N-1) >= N )
            x *= x;
         if ( i != 0 )
            x *= lut[i];
      }
      return T( (n >= 0) ? x : 1/x );
   }
};

// ----------------------------------------------------------------------------

/*!
 * The exponential function 10**x.
 * \ingroup mathematical_functions
 */
template <typename T> inline T Pow10( T x ) noexcept
{
   int i = TruncInt( x );
   return (i == x) ? Pow10I<T>()( i ) : T( std::pow( 10, x ) );
}

// ----------------------------------------------------------------------------

/*!
 * Bitwise rotate left function: Rotates \a x to the left by \a n bits.
 *
 * The template argument T must be an unsigned arithmetic type (uint8, uint16,
 * uint32 or uint64). The bit count \a n must be smaller than the number of
 * bits required to store an instance of T; for example, if T is uint32, \a n
 * must be in the range [0,31].
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline T RotL( T x, uint32 n ) noexcept
{
   static_assert( std::is_unsigned<T>::value,
                  "RotL() can only be used for unsigned integer scalar types" );
   const uint8 mask = 8*sizeof( x ) - 1;
   const uint8 r = uint8( n & mask );
   return (x << r) | (x >> ((-r) & mask));
   // Or equivalently, but less optimized:
   //return (x << r) | (x >> (1+mask-r));
}

/*!
 * Bitwise rotate right function: Rotates \a x to the right by \a n bits.
 *
 * The template argument T must be an unsigned arithmetic type (uint8, uint16,
 * uint32 or uint64). The bit count \a n must be smaller than the number of
 * bits required to store an instance of T; for example, if T is uint32, \a n
 * must be in the range [0,31].
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline T RotR( T x, uint32 n ) noexcept
{
   static_assert( std::is_unsigned<T>::value,
                  "RotR() can only be used for unsigned integer scalar types" );
   const uint8 mask = 8*sizeof( x ) - 1;
   const uint8 r = uint8( n & mask );
   return (x >> r) | (x << ((-r) & mask));
   // Or equivalently, but less optimized:
   //return (x >> r) | (x << (1+mask-r));
}

// ----------------------------------------------------------------------------

/*!
 * Round function: x rounded to the nearest integer (double precision version).
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \ingroup mathematical_functions
 */
inline double Round( double x ) noexcept
{
#ifdef __PCL_NO_PERFORMANCE_CRITICAL_MATH_ROUTINES

   return floor( x + 0.5 );

#else

#  ifdef _MSC_VER
#    ifdef _M_X64
   return double( _mm_cvtsd_si64( _mm_load_sd( &x ) ) );
#    else
   __asm fld      x
   __asm frndint
#    endif
#  else
   double r;
   asm volatile( "frndint\n": "=t" (r) : "0" (x) );
   return r;
#  endif

#endif
}

/*!
 * Round function: x rounded to the nearest integer (single precision version).
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \ingroup mathematical_functions
 */
inline float Round( float x ) noexcept
{
#ifdef __PCL_NO_PERFORMANCE_CRITICAL_MATH_ROUTINES

   return floorf( x + 0.5F );

#else

#  ifdef _MSC_VER
#    ifdef _M_X64
   return float( _mm_cvtss_si32( _mm_load_ss( &x ) ) );
#    else
   __asm fld      x
   __asm frndint
#    endif
#  else
   float r;
   asm volatile( "frndint\n": "=t" (r) : "0" (x) );
   return r;
#  endif

#endif
}

/*!
 * Round function: x rounded to the nearest integer (long double version).
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \ingroup mathematical_functions
 */
inline long double Round( long double x ) noexcept
{
#ifdef __PCL_NO_PERFORMANCE_CRITICAL_MATH_ROUTINES

   return floorl( x + 0.5L );

#else

#  ifdef _MSC_VER
#    ifdef _M_X64
   double _x = x;
   return (long double)_mm_cvtsd_si64( _mm_load_sd( &_x ) );
#    else
   __asm fld      x
   __asm frndint
#    endif
#  else
   long double r;
   asm volatile( "frndint\n": "=t" (r) : "0" (x) );
   return r;
#  endif

#endif
}

// ----------------------------------------------------------------------------

/*!
 * RoundInt function: Rounds \a x to the nearest integer and converts the
 * result to a 32-bit signed integer.
 *
 * This function follows the Banker's rounding rule: a perfect half is always
 * rounded to the nearest even digit. Some examples:
 *
 * <pre>
 * RoundInt( 0.5 ) -> 0
 * RoundInt( 1.5 ) -> 2
 * RoundInt( 2.5 ) -> 2
 * RoundInt( 3.5 ) -> 4
 * </pre>
 *
 * By contrast, arithmetic rounding rounds a perfect half to the nearest digit,
 * either odd or even. For example:
 *
 * <pre>
 * RoundIntArithmetic( 0.5 ) -> 1
 * RoundIntArithmetic( 1.5 ) -> 2
 * RoundIntArithmetic( 2.5 ) -> 3
 * RoundIntArithmetic( 3.5 ) -> 4
 * </pre>
 *
 * Banker's rounding (also known as Gaussian rounding) is statistically more
 * accurate than the usual arithmetic rounding, but it causes aliasing problems
 * in some specific algorithms that depend critically on uniform rounding, such
 * as nearest neighbor upsampling.
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \sa RoundIntArithmetic(), RoundIntBanker()
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int RoundInt( T x ) noexcept
{
   PCL_PRECONDITION( x >= int_min && x <= int_max )

#ifdef __PCL_NO_PERFORMANCE_CRITICAL_MATH_ROUTINES

   return int( Round( x ) );

#else

   volatile union { double d; int32 i; } v = { x + 6755399441055744.0 };
   return v.i; // ### NB: Assuming little-endian architecture.

/*
   ### Deprecated code - The above code based on magic numbers is faster and
                         more portable across platforms and compilers.

   // Default FPU rounding mode assumed to be nearest integer.
   int r;

#  ifdef _MSC_VER
   __asm fld      x
   __asm fistp    dword ptr r
#  else
   asm volatile( "fistpl %0\n" : "=m" (r) : "t" (x) : "st" );
#  endif

   return r;
*/

#endif
}

/*!
 * RoundI function: Rounds \a x to the nearest integer and converts the result
 * to a 32-bit signed integer.
 *
 * \deprecated Use RoundInt() in newly produced code.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int RoundI( T x ) noexcept
{
   return RoundInt( x );
}

/*!
 * Rounds \a x to the nearest integer using the Banker's rounding rule, and
 * converts the result to a 32-bit signed integer.
 *
 * This function is a convenience synonym for RoundInt().
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int RoundIntBanker( T x ) noexcept
{
   return RoundInt( x );
}

/*!
 * Rounds \a x to the nearest integer using the arithmetic rounding rule, and
 * converts the result to a 32-bit signed integer.
 *
 * Arithmetic rounding rounds a perfect half to the nearest digit, either odd
 * or even. For example:
 *
 * <pre>
 * RoundIntArithmetic( 0.5 ) -> 1
 * RoundIntArithmetic( 1.5 ) -> 2
 * RoundIntArithmetic( 2.5 ) -> 3
 * RoundIntArithmetic( 3.5 ) -> 4
 * </pre>
 *
 * See the RoundInt() function for more information on rounding rules.
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \sa RoundInt(), RoundIntBanker()
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline int RoundIntArithmetic( T x ) noexcept
{
   PCL_PRECONDITION( x >= int_min && x <= int_max )

   int i = TruncInt( x );
   if ( i < 0 )
   {
      if ( x - i <= T( -0.5 ) )
         return i-1;
   }
   else
   {
      if ( x - i >= T( +0.5 ) )
         return i+1;
   }
   return i;
}

// ----------------------------------------------------------------------------

/*!
 * RoundInt64 function: Rounds \a x to the nearest integer and converts the
 * result to a 64-bit signed integer.
 *
 * Since the default IEEE 754 rounding mode follows Banker's rounding rule,
 * this is what you should expect when calling this function. See the
 * documentation for RoundInt() for more information on rounding modes.
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \ingroup mathematical_functions
 */
inline int64 RoundInt64( double x ) noexcept
{
#ifdef __PCL_NO_PERFORMANCE_CRITICAL_MATH_ROUTINES

   return int64( Round( x ) );

#else

   // ### N.B.: Default FPU rounding mode assumed to be nearest integer.

#  ifdef _MSC_VER
#    ifdef _M_X64
   return _mm_cvtsd_si64( _mm_load_sd( &x ) );
#    else
   int64 r;
   __asm fld      x
   __asm fistp    qword ptr r
   return r;
#    endif
#  else
   int64 r;
   asm volatile( "fistpll %0\n" : "=m" (r) : "t" (x) : "st" );
   return r;
#  endif

#endif
}

/*!
 * RoundI64 function: Rounds \a x to the nearest integer and converts the
 * result to a 64-bit signed integer.
 *
 * \deprecated Use RoundInt64() in newly produced code.
 */
inline int64 RoundI64( double x ) noexcept
{
   return RoundInt64( x );
}

/*!
 * Rounds \a x to the nearest integer using the arithmetic rounding rule, and
 * converts the result to a 64-bit signed integer.
 *
 * Arithmetic rounding rounds a perfect half to the nearest digit, either odd
 * or even. For example:
 *
 * <pre>
 * RoundIntArithmetic( 0.5 ) -> 1
 * RoundIntArithmetic( 1.5 ) -> 2
 * RoundIntArithmetic( 2.5 ) -> 3
 * RoundIntArithmetic( 3.5 ) -> 4
 * </pre>
 *
 * See the RoundInt() function for more information on rounding rules.
 *
 * \note This is a performance-critical routine. It has been implemented using
 * high-performance, low-level techniques that may include inline assembly code
 * and/or compiler intrinsic functions.
 *
 * \sa RoundInt(), RoundIntBanker()
 *
 * \ingroup mathematical_functions
 */
inline int64 RoundInt64Arithmetic( double x ) noexcept
{
   int64 i = TruncInt64( x );
   if ( i < 0 )
   {
      if ( x - i <= -0.5 )
         return i-1;
   }
   else
   {
      if ( x - i >= +0.5 )
         return i+1;
   }
   return i;
}

// ----------------------------------------------------------------------------

/*!
 * General rounding function: \a x rounded to \a n fractional digits.
 * \ingroup mathematical_functions
 */
template <typename T> inline T Round( T x, int n ) noexcept
{
   PCL_PRECONDITION( n >= 0 )
   T p = Pow10I<T>()( n ); return Round( p*x )/p;
}

// ----------------------------------------------------------------------------

/*!
 * The exponential function 2**x.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Pow2( T x ) noexcept
{
   // Use the relation:
   //    2**x = e**(x*ln(2))
   return Exp( x*Ln2() );
}

// ----------------------------------------------------------------------------

/*!
 * \class Pow2I
 * \brief Exponential function 2**n, n being a signed integer.
 *
 * Example of use:
 *
 * \code float x = Pow2I<float>()( -2 ); // x = 1/4 \endcode
 *
 * \ingroup mathematical_functions
 */
template <typename T> struct PCL_CLASS Pow2I
{
   T operator ()( int n ) const noexcept
   {
      // We shift left a single bit in 31-bit chunks.
      int i = ::abs( n ), p;
      double x = uint32( 1 ) << (p = Min( i, 31 ));
      while ( (i -= p) != 0 )
         x *= uint32( 1 ) << (p = Min( i, 31 ));
      return T( (n >= 0) ? x : 1/x );
   }
};

// ----------------------------------------------------------------------------

/*!
 * The exponential function x**n, where \a n is a signed integer.
 * \ingroup mathematical_functions
 */
template <typename T> inline T PowI( T x, int n ) noexcept
{
   if ( n == 0 )
      return 1;

   int i = Abs( n );
   T r = x;
   if ( i > 1 )
   {
      do
         r *= r;
      while ( (i >>= 1) >= 2 );

      if ( (n & 1) != 0 )
         r *= x;
   }
   return (n >= 0) ? r : 1/r;
}

// ----------------------------------------------------------------------------

/*!
 * Inverse hyperbolic sine function.
 *
 * <tt>ArcSinh( x ) = Ln( x + Sqrt( 1 + x**2 ) )</tt>
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcSinh( T x ) noexcept
{
   return Ln( x + Sqrt( 1 + x*x ) );
}

// ----------------------------------------------------------------------------

/*!
 * Inverse hyperbolic cosine function.
 *
 * <tt>ArcCosh( x ) = 2*Ln( Sqrt( (x + 1)/2 ) + Sqrt( (x - 1)/2 ) )</tt>
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcCosh( T x ) noexcept
{
   return 2*Ln( Sqrt( (x + 1)/2 ) + Sqrt( (x - 1)/2 ) );
}

// ----------------------------------------------------------------------------

/*!
 * Inverse hyperbolic tangent function.
 *
 * <tt>ArcTanh( x ) = (Ln( 1 + x ) - Ln( 1 - x ))/2</tt>
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcTanh( T x ) noexcept
{
   return (Ln( 1 + x ) - Ln( 1 - x ))/2;
}

// ----------------------------------------------------------------------------

/*!
 * Inverse haversine (archaversine) function:
 *
 * <tt>ArcHav( x ) = 2*ArcSin( Sqrt( x ) )</tt>
 *
 * The haversine is useful to work with tiny angles.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T ArcHav( T x ) noexcept
{
   return 2*ArcSin( Sqrt( x ) );
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from degrees to radians.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Rad( T x ) noexcept
{
   return static_cast<T>( 0.174532925199432957692369076848861272222e-01L * x );
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from radians to arcminutes.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T RadMin( T x ) noexcept
{
   return Deg( x )*60;
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from radians to arcseconds.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T RadSec( T x ) noexcept
{
   return Deg( x )*3600;
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from arcminutes to radians.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T MinRad( T x ) noexcept
{
   return Rad( x/60 );
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from arcseconds to radians.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T SecRad( T x ) noexcept
{
   return Rad( x/3600 );
}

/*!
 * Conversion from arcseconds to radians (a synonym for SecRad()).
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T AsRad( T x ) noexcept
{
   return SecRad( x );
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from milliarcseconds (mas) to radians.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T MasRad( T x ) noexcept
{
   return Rad( x/3600000 );
}

// ----------------------------------------------------------------------------

/*!
 * Conversion from microarcseconds (uas) to radians.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T UasRad( T x ) noexcept
{
   return Rad( x/3600000000 );
}

// ----------------------------------------------------------------------------

/*!
 * An angle in radians reduced to the (-2pi,+2pi) range, that is, the remainder
 * of x/(2*pi).
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Mod2Pi( T x ) noexcept
{
   return Mod( x, static_cast<T>( TwoPi() ) );
}

// ----------------------------------------------------------------------------

/*!
 * An angle in radians normalized to the [0,2pi) range.
 * \ingroup mathematical_functions
 */
template <typename T> inline constexpr T Norm2Pi( T x ) noexcept
{
   return ((x = Mod2Pi( x )) < 0) ? x + static_cast<T>( TwoPi() ) : x;
}

// ----------------------------------------------------------------------------

/*!
 * Rotates a point on the plane.
 *
 * \param[out] x,y   %Point coordinates. On output, these variables will
 *                   receive the corresponding rotated coordinates.
 *
 * \param sa, ca     Sine and cosine of the rotation angle.
 *
 * \param xc,yc      Coordinates of the center of rotation.
 *
 * \ingroup mathematical_functions
 */
template <typename T, typename T1, typename T2>
inline void Rotate( T& x, T& y, T1 sa, T1 ca, T2 xc, T2 yc ) noexcept
{
   T1 dx = T1( x ) - T1( xc );
   T1 dy = T1( y ) - T1( yc );
   x = T( T1( xc ) + ca*dx + sa*dy );
   y = T( T1( yc ) - sa*dx + ca*dy );
}

/*!
 * Rotates a point on the plane.
 *
 * This is a template instantiation of Rotate( T&, T&, T1, T1, T2, T2 ) for
 * the \c int type.
 *
 * Rotated coordinates are rounded to the nearest integers.
 *
 * \ingroup mathematical_functions
 */
template <typename T1, typename T2>
inline void Rotate( int& x, int& y, T1 sa, T1 ca, T2 xc, T2 yc ) noexcept
{
   T1 dx = T1( x ) - T1( xc );
   T1 dy = T1( y ) - T1( yc );
   x = RoundInt( T1( xc ) + ca*dx + sa*dy );
   y = RoundInt( T1( yc ) - sa*dx + ca*dy );
}

/*!
 * Rotates a point on the plane.
 *
 * This is a template instantiation of Rotate( T&, T&, T1, T1, T2, T2 ) for
 * the \c long type.
 *
 * Rotated coordinates are rounded to the nearest integers.
 *
 * \ingroup mathematical_functions
 */
template <typename T1, typename T2>
inline void Rotate( long& x, long& y, T1 sa, T1 ca, T2 xc, T2 yc ) noexcept
{
   T1 dx = T1( x ) - T1( xc );
   T1 dy = T1( y ) - T1( yc );
   x = (long)RoundInt( T1( xc ) + ca*dx + sa*dy );
   y = (long)RoundInt( T1( yc ) - sa*dx + ca*dy );
}

/*!
 * Rotates a point on the plane.
 *
 * This is a template instantiation of Rotate( T&, T&, T1, T1, T2, T2 ) for
 * the \c int64 type.
 *
 * Rotated coordinates are rounded to the nearest integers.
 *
 * \ingroup mathematical_functions
 */
template <typename T1, typename T2>
inline void Rotate( int64& x, int64& y, T1 sa, T1 ca, T2 xc, T2 yc ) noexcept
{
   T1 dx = T1( x ) - T1( xc );
   T1 dy = T1( y ) - T1( yc );
   x = RoundInt64( T1( xc ) + ca*dx + sa*dy );
   y = RoundInt64( T1( yc ) - sa*dx + ca*dy );
}

/*!
 * Rotates a point on the plane.
 *
 * \param[out] x,y   %Point coordinates. On output, these variables will
 *                   receive the corresponding rotated coordinates.
 *
 * \param a          Rotation angle in radians.
 *
 * \param xc,yc      Coordinates of the center of rotation.
 *
 * Instantiations for integer types round rotated coordinated to the nearest
 * integers.
 *
 * \ingroup mathematical_functions
 */
template <typename T, typename T1, typename T2>
inline void Rotate( T& x, T& y, T1 a, T2 xc, T2 yc ) noexcept
{
   T1 sa, ca; SinCos( a, sa, ca ); Rotate( x, y, sa, ca, xc, yc );
}

// ----------------------------------------------------------------------------

/*!
 * Computes the norm of the elements in the sequence [i,j). For any real p > 0,
 * the norm N is given by:
 *
 * <pre>
 * N = sum( abs( x )^p )^(1/p)
 * </pre>
 *
 * for all x in [i,j).
 *
 * \ingroup mathematical_functions
 * \sa L1Norm(), L2Norm()
 */
template <typename T> inline double Norm( const T* i, const T* j, double p ) noexcept
{
   PCL_PRECONDITION( p > 0 )
   double N = 0;
   for ( ; i < j; ++i )
      N += Pow( Abs( double( *i ) ), p );
   return Pow( N, 1/p );
}

/*!
 * Computes the L1 norm (or Manhattan norm) of the elements in the sequence
 * [i,j). The L1 norm is the sum of the absolute values of the elements.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline double L1Norm( const T* i, const T* j ) noexcept
{
   double N = 0;
   for ( ; i < j; ++i )
      N += Abs( *i );
   return N;
}

/*!
 * Computes the L2 norm (or Euclidean norm) of the elements in the sequence
 * [i,j). The L2 norm is the square root of the sum of squared elements.
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline double L2Norm( const T* i, const T* j ) noexcept
{
   double N = 0;
   for ( ; i < j; ++i )
      N += double( *i ) * *i;
   return Sqrt( N );
}

/*!
 * Computes the L2 norm (or Euclidean norm) of the elements in the sequence
 * [i,j). This function is a synonym for L2Norm().
 *
 * \ingroup mathematical_functions
 */
template <typename T> inline double Norm( const T* i, const T* j ) noexcept
{
   return L2Norm( i, j );
}

// ----------------------------------------------------------------------------

/*!
 * Computes the Julian date (JD) corresponding to a time point expressed as a
 * date and a day fraction, providing the result by its separate integer and
 * fractional parts.
 *
 * \param[out] jdi   On output, the integer part of the resulting JD.
 *
 * \param[out] jdf   On output, the fractional part of the resulting JD.
 *
 * \param year       The year of the date. Positive and negative years are
 *                   supported. Years are counted arithmetically: the year zero
 *                   is the year before the year +1, that is, what historians
 *                   call the year 1 B.C.
 *
 * \param month      The month of the date. Usually in the [1,12] range but can
 *                   be any integer number.
 *
 * \param day        The day of the date. Usually in the [1,31] range but can
 *                   be any integer number.
 *
 * \param dayf       The day fraction. The default value is zero, which
 *                   computes the JD at zero hours. Usually in the [0,1) range
 *                   but can be any real number.
 *
 * This routine, as well as JDToCalendarTime(), implement modified versions of
 * the original algorithms due to Jean Meeus. Our modifications allow for
 * negative Julian dates, which extends the range of allowed dates to the past
 * considerably. We developed these modifications in the context of large-scale
 * solar system ephemeris calculations.
 *
 * The computed value is the JD corresponding to the specified date and day
 * fraction, which is equal to the sum of the \a jdi and \a jdf variables.
 *
 * \b References
 *
 * Meeus, Jean (1991), <em>Astronomical Algorithms</em>, Willmann-Bell, Inc.,
 * chapter 7.
 *
 * \ingroup mathematical_functions
 */
void PCL_FUNC CalendarTimeToJD( int& jdi, double& jdf, int year, int month, int day, double dayf = 0 ) noexcept;

/*!
 * Computes the Julian date (JD) corresponding to a time point expressed as a
 * date and a day fraction.
 *
 * \param year    The year of the date. Positive and negative years are
 *                supported. Years are counted arithmetically: the year zero is
 *                the year before the year +1, that is, what historians call
 *                the year 1 B.C.
 *
 * \param month   The month of the date. Usually in the [1,12] range but can be
 *                any integer number.
 *
 * \param day     The day of the date. Usually in the [1,31] range but can be
 *                any integer number.
 *
 * \param dayf    The day fraction. The default value is zero, which computes
 *                the JD at zero hours. Usually in the [0,1) range but can be
 *                any real number.
 *
 * This routine, as well as JDToCalendarTime(), implement modified versions of
 * the original algorithms due to Jean Meeus. Our modifications allow for
 * negative Julian dates, which extends the range of allowed dates to the past
 * considerably. We developed these modifications in the context of large-scale
 * solar system ephemeris calculations.
 *
 * The returned value is the JD corresponding to the specified date and day
 * fraction.
 *
 * Because of the numerical precision of the double type (IEEE 64-bit floating
 * point), this routine can return JD values accurate only to within one
 * millisecond.
 *
 * \b References
 *
 * Meeus, Jean (1991), <em>Astronomical Algorithms</em>, Willmann-Bell, Inc.,
 * chapter 7.
 *
 * \ingroup mathematical_functions
 */
inline double CalendarTimeToJD( int year, int month, int day, double dayf = 0 ) noexcept
{
   int jdi;
   double jdf;
   CalendarTimeToJD( jdi, jdf, year, month, day, dayf );
   return jdi + jdf;
}

/*!
 * Computes the date and day fraction corresponding to a time point expressed
 * as a Julian date (JD), specified by its separate integer and fractional
 * parts.
 *
 * \param[out] year  On output, this variable receives the year of the
 *                   resulting date.
 *
 * \param[out] month On output, this variable receives the month of the
 *                   resulting date in the range [1,12].
 *
 * \param[out] day   On output, this variable receives the day of the
 *                   resulting date in the range [1,31]. Different month day
 *                   counts and leap years are taken into account, so the
 *                   returned day corresponds to an existing calendar date.
 *
 * \param[out] dayf  On output, this variable receives the day fraction for the
 *                   specified time point, in the [0,1) range.
 *
 * \param jdi        The integer part of the input Julian date.
 *
 * \param jdf        The fractional part of the input Julian date.
 *
 * The input time point must be equal to the sum of \a jdi and \a jdf.
 *
 * For more information about the implemented algorithms and references, see
 * the documentation for CalendarTimeToJD().
 *
 * \ingroup mathematical_functions
 */
void PCL_FUNC JDToCalendarTime( int& year, int& month, int& day, double& dayf, int jdi, double jdf ) noexcept;

/*!
 * Computes the date and day fraction corresponding to a time point expressed
 * as a Julian date (JD).
 *
 * \param[out] year  On output, this variable receives the year of the
 *                   resulting date.
 *
 * \param[out] month On output, this variable receives the month of the
 *                   resulting date in the range [1,12].
 *
 * \param[out] day   On output, this variable receives the day of the
 *                   resulting date in the range [1,31]. Different month day
 *                   counts and leap years are taken into account, so the
 *                   returned day corresponds to an existing calendar date.
 *
 * \param[out] dayf  On output, this variable receives the day fraction for the
 *                   specified time point, in the [0,1) range.
 *
 * \param jd         The input time point as a Julian date.
 *
 * Because of the numerical precision of the double type (IEEE 64-bit floating
 * point), this routine can handle JD values accurate only to within one
 * millisecond.
 *
 * For more information about the implemented algorithms and references, see
 * the documentation for CalendarTimeToJD().
 *
 * \ingroup mathematical_functions
 */
inline void JDToCalendarTime( int& year, int& month, int& day, double& dayf, double jd ) noexcept
{
   JDToCalendarTime( year, month, day, dayf, TruncInt( jd ), Frac( jd ) );
}

// ----------------------------------------------------------------------------

/*!
 * Conversion of a decimal scalar \a d to the equivalent sexagesimal decimal
 * components \a sign, \a s1, \a s2 and \a s3, such that:
 *
 * <pre>
 * d = sign * (s1 + (s2 + s3/60)/60)
 * </pre>
 *
 * with the following constraints:
 *
 * <pre>
 * sign = -1 iff d &lt; 0
 * sign = +1 iff d &ge; 0
 * 0 &le; s1
 * 0 &le; s2 &lt; 60
 * 0 &le; s3 &lt; 60
 * </pre>
 *
 * \ingroup mathematical_functions
 */
template <typename S1, typename S2, typename S3, typename D>
inline void DecimalToSexagesimal( int& sign, S1& s1, S2& s2, S3& s3, const D& d ) noexcept
{
   double t1 = Abs( d );
   double t2 = Frac( t1 )*60;
   double t3 = Frac( t2 )*60;
   sign = (d < 0) ? -1 : +1;
   s1 = S1( TruncInt( t1 ) );
   s2 = S2( TruncInt( t2 ) );
   s3 = S3( t3 );
}

/*!
 * Conversion of the sexagesimal decimal components \a sign, \a s1, \a s2 and
 * \a s3 to their equivalent decimal scalar. The returned value is equal to:
 *
 * <pre>
 * ((sign < 0) ? -1 : +1)*(Abs( s1 ) + (s2 + s3/60)/60);
 * </pre>
 *
 * \ingroup mathematical_functions
 */
template <typename S1, typename S2, typename S3>
inline double SexagesimalToDecimal( int sign, const S1& s1, const S2& s2 = S2( 0 ), const S3& s3 = S3( 0 ) ) noexcept
{
   double d = Abs( s1 ) + (s2 + s3/60)/60;
   return (sign < 0) ? -d : d;
}

// ----------------------------------------------------------------------------

/*!
 * \defgroup statistical_functions Statistical Functions
 */

/*!
 * Returns the sum of elements in a sequence [i,j).
 *
 * For empty sequences, this function returns zero.
 *
 * This is a straight implementation of a floating point sum, which is subject
 * to severe roundoff errors if the number of summed elements is large. One way
 * to improve on this problem is to sort the input set by decreasing order of
 * absolute value \e before calling this function. A much better solution, but
 * computationally expensive, is a compensated summation algorithm such as
 * Kahan summation, which we have implemented in the StableSum() routine.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double Sum( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   double sum = 0;
   while ( i < j )
      sum += double( *i++ );
   return sum;
}

/*!
 * Computes the sum of elements in a sequence [i,j) using a numerically stable
 * summation algorithm to minimize roundoff error.
 *
 * For empty sequences, this function returns zero.
 *
 * In the current PCL versions, this function implements the Kahan summation
 * algorithm to reduce roundoff error to the machine's floating point error.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StableSum( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   double sum = 0;
   double eps = 0;
   while ( i < j )
   {
      double y = double( *i++ ) - eps;
      double t = sum + y;
      eps = (t - sum) - y;
      sum = t;
   }
   return sum;
}

/*!
 * Returns the sum of the absolute values of the elements in a sequence [i,j).
 *
 * For empty sequences, this function returns zero.
 *
 * See the remarks made for the Sum() function, which are equally applicable in
 * this case. See StableModulus() for a (slow) numerically stable version of
 * this function.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double Modulus( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   double S = 0;
   while ( i < j )
      S += Abs( double( *i++ ) );
   return S;
}

/*!
 * Computes the sum of the absolute values of the elements in a sequence [i,j)
 * using a numerically stable summation algorithm to minimize roundoff error.
 *
 * For empty sequences, this function returns zero.
 *
 * In the current PCL versions, this function implements the Kahan summation
 * algorithm to reduce roundoff error to the machine's floating point error.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StableModulus( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   double sum = 0;
   double eps = 0;
   while ( i < j )
   {
      double y = Abs( double( *i++ ) ) - eps;
      double t = sum + y;
      eps = (t - sum) - y;
      sum = t;
   }
   return sum;
}

/*!
 * Returns the sum of the squares of the elements in a sequence [i,j).
 *
 * For empty sequences, this function returns zero.
 *
 * See the remarks made for the Sum() function, which are equally applicable in
 * this case. See StableSumOfSquares() for a (slow) numerically stable version
 * of this function.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double SumOfSquares( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   double Q = 0;
   while ( i < j )
   {
      double f = double( *i++ );
      Q += f*f;
   }
   return Q;
}

/*!
 * Computes the sum of the squares of the elements in a sequence [i,j) using a
 * numerically stable summation algorithm to minimize roundoff error.
 *
 * For empty sequences, this function returns zero.
 *
 * In the current PCL versions, this function implements the Kahan summation
 * algorithm to reduce roundoff error to the machine's floating point error.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StableSumOfSquares( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   double sum = 0;
   double eps = 0;
   while ( i < j )
   {
      double f = double( *i++ );
      double y = f*f - eps;
      double t = sum + y;
      eps = (t - sum) - y;
      sum = t;
   }
   return sum;
}

/*!
 * Returns the arithmetic mean of a sequence [i,j).
 *
 * For empty sequences, this function returns zero.
 *
 * See the remarks made for the Sum() function, which are equally applicable in
 * this case. See StableMean() for a (slow) numerically stable version of this
 * function.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double Mean( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;
   return Sum( i, j )/n;
}

/*!
 * Computes the arithmetic mean of a sequence [i,j) using a numerically stable
 * summation algorithm to minimize roundoff error.
 *
 * For empty sequences, this function returns zero.
 *
 * In the current PCL versions, this function implements the Kahan summation
 * algorithm to reduce roundoff error to the machine's floating point error.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StableMean( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;
   return StableSum( i, j )/n;
}

/*!
 * Returns the variance of a sequence [i,j) with respect to the specified
 * \a center value.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * This implementation uses a two-pass compensated summation algorithm to
 * minimize roundoff errors (see the references).
 *
 * \b References
 *
 * William H. Press et al., <em>Numerical Recipes in C: The Art of Scientific
 * Computing, Second Edition</em> (1997 reprint) Cambridge University Press,
 * page 613.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double Variance( const T* __restrict__ i, const T* __restrict__ j, double center ) noexcept
{
   distance_type n = j - i;
   if ( n < 2 )
      return 0;
   double var = 0, eps = 0;
   do
   {
      double d = double( *i++ ) - center;
      var += d*d;
      eps += d;
   }
   while ( i < j );
   return (var - eps*eps/n)/(n - 1);
}

/*!
 * Returns the variance from the mean of a sequence [i,j).
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * This implementation uses a two-pass compensated summation algorithm to
 * minimize roundoff errors (see References).
 *
 * \b References
 *
 * William H. Press et al., <em>Numerical Recipes in C: The Art of Scientific
 * Computing, Second Edition</em> (1997 reprint) Cambridge University Press,
 * page 613.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double Variance( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   distance_type n = j - i;
   if ( n < 2 )
      return 0;
   double m = 0;
   for ( const T* f = i; f < j; ++f )
      m += double( *f );
   m /= n;
   double var = 0, eps = 0;
   do
   {
      double d = double( *i++ ) - m;
      var += d*d;
      eps += d;
   }
   while ( i < j );
   return (var - eps*eps/n)/(n - 1);
}

/*!
 * Returns the standard deviation of a sequence [i,j) with respect to the
 * specified \a center value.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StdDev( const T* __restrict__ i, const T* __restrict__ j, double center ) noexcept
{
   return Sqrt( Variance( i, j, center ) );
}

/*!
 * Returns the standard deviation from the mean of a sequence [i,j).
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StdDev( const T* __restrict__ i, const T* __restrict__ j ) noexcept
{
   return Sqrt( Variance( i, j ) );
}

/*!
 * Returns the median value of a sequence [i,j).
 *
 * For scalar data types the following algorithms are used:
 *
 * \li Hard-coded, fast selection networks for small sequences of 32 or less
 * elements.
 *
 * \li A quick selection algorithm for sequences of up to about 2M elements.
 * The actual limit has been determined empirically and can vary across PCL
 * versions. This single-threaded algorithm can use up to 16 MiB of additional
 * memory allocated dynamically (for 8-byte types such as \c double).
 *
 * \li A parallelized, fast histogram-based algorithm for sequences larger than
 * the limit described above. This algorithm has negligible additional memory
 * space requirements.
 *
 * For non-scalar data types, this function requires the following type
 * conversion operator publicly defined for the type T:
 *
 * \code T::operator double() const; \endcode
 *
 * This operator will be used to generate a temporary dynamic array of
 * \c double values with the length of the input sequence, which will be used
 * to compute the median with the algorithms enumerated above.
 *
 * \b References (selection networks)
 *
 * \li Knuth, D. E., <em>The Art of Computer Programming, volume 3:
 * Sorting and Searching,</em> Addison Wesley, 1973.
 *
 * \li Hillis, W. D., <em>Co-evolving parasites improve simulated
 * evolution as an optimization procedure.</em> Langton, C. et al. (Eds.),
 * Artificial Life II. Addison Wesley, 1992.
 *
 * \li Hugues Juille, <em>Evolution of Non-Deterministic Incremental
 * Algorithms as a New Approach for Search in State Spaces,</em> 1995
 *
 * \b References (quick select algorithm)
 *
 * \li William H. Press et al., <em>Numerical Recipes 3rd Edition: The Art of
 * Scientific Computing,</em> Cambridge University Press, 2007, Section 8.5.
 *
 * \li Robert Sedgewick, Kevin Wayne, <em>Algorithms, 4th Edition,</em>
 * Addison-Wesley Professional, 2011, pp 345-347.
 *
 * \ingroup statistical_functions
 */
template <class T> inline double Median( const T* __restrict__ i, const T* __restrict__ j )
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;
   if ( n == 1 )
      return double( *i );
   double* d = new double[ n ];
   double* __restrict__ t = d;
   do
      *t++ = double( *i++ );
   while ( i < j );
   double m = double( *pcl::Select( d, t, n >> 1 ) );
   if ( (n & 1) == 0 )
      m = (m + double( *pcl::Select( d, t, (n >> 1)-1 ) ))/2;
   delete [] d;
   return m;
}

double PCL_FUNC Median( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j );
double PCL_FUNC Median( const signed char* __restrict__ i, const signed char* __restrict__ j );
double PCL_FUNC Median( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j );
double PCL_FUNC Median( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j );
double PCL_FUNC Median( const signed short* __restrict__ i, const signed short* __restrict__ j );
double PCL_FUNC Median( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j );
double PCL_FUNC Median( const signed int* __restrict__ i, const signed int* __restrict__ j );
double PCL_FUNC Median( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j );
double PCL_FUNC Median( const signed long* __restrict__ i, const signed long* __restrict__ j );
double PCL_FUNC Median( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j );
double PCL_FUNC Median( const signed long long* __restrict__ i, const signed long long* __restrict__ j );
double PCL_FUNC Median( const float* __restrict__ i, const float* __restrict__ j );
double PCL_FUNC Median( const double* __restrict__ i, const double* __restrict__ j );
double PCL_FUNC Median( const long double* __restrict__ i, const long double* __restrict__ j );

#define CMPXCHG( a, b )  \
   if ( i[b] < i[a] ) pcl::Swap( i[a], i[b] )

#define MEAN( a, b ) \
   (double( a ) + double( b ))/2

/*!
 * Returns the median value of a sequence [i,j), altering the existing order of
 * elements in the input sequence.
 *
 * This function is intended for sequences of non-scalar objects where the
 * order of elements is irrelevant, and hence generation of a working duplicate
 * is unnecessary. The following type conversion operator must be publicly
 * defined for the type T:
 *
 * \code T::operator double() const; \endcode
 *
 * The following algorithms are used:
 *
 * \li Hard-coded, fast selection networks for sequences of 9 or less elements.
 *
 * \li A quick selection algorithm for sequences larger than 9 elements.
 *
 * \note This is a \e destructive median calculation algorithm: it alters the
 * existing order of items in the input [i,j) sequence.
 *
 * \b References (selection networks)
 *
 * \li Knuth, D. E., <em>The Art of Computer Programming, volume 3:
 * Sorting and Searching,</em> Addison Wesley, 1973.
 *
 * \li Hillis, W. D., <em>Co-evolving parasites improve simulated
 * evolution as an optimization procedure.</em> Langton, C. et al. (Eds.),
 * Artificial Life II. Addison Wesley, 1992.
 *
 * \li Hugues Juille, <em>Evolution of Non-Deterministic Incremental
 * Algorithms as a New Approach for Search in State Spaces,</em> 1995
 *
 * \b References (quick select algorithm)
 *
 * \li William H. Press et al., <em>Numerical Recipes 3rd Edition: The Art of
 * Scientific Computing,</em> Cambridge University Press, 2007, Section 8.5.
 *
 * \li Robert Sedgewick, Kevin Wayne, <em>Algorithms, 4th Edition,</em>
 * Addison-Wesley Professional, 2011, pp 345-347.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double MedianDestructive( T* __restrict__ i, T* __restrict__ j ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;

   switch ( n )
   {
   case  1: // !?
      return i[0];
   case  2:
      return MEAN( i[0], i[1] );
   case  3:
      CMPXCHG( 0, 1 ); CMPXCHG( 1, 2 );
      return pcl::Max( i[0], i[1] );
   case  4:
      CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 ); CMPXCHG( 0, 2 );
      CMPXCHG( 1, 3 );
      return MEAN( i[1], i[2] );
   case  5:
      CMPXCHG( 0, 1 ); CMPXCHG( 3, 4 ); CMPXCHG( 0, 3 );
      CMPXCHG( 1, 4 ); CMPXCHG( 1, 2 ); CMPXCHG( 2, 3 );
      return pcl::Max( i[1], i[2] );
   case  6:
      CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 ); CMPXCHG( 0, 2 );
      CMPXCHG( 1, 3 ); CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 );
      CMPXCHG( 0, 4 ); CMPXCHG( 1, 5 ); CMPXCHG( 1, 4 );
      CMPXCHG( 2, 4 ); CMPXCHG( 3, 5 ); CMPXCHG( 3, 4 );
      return MEAN( i[2], i[3] );
   case  7:
      CMPXCHG( 0, 5 ); CMPXCHG( 0, 3 ); CMPXCHG( 1, 6 );
      CMPXCHG( 2, 4 ); CMPXCHG( 0, 1 ); CMPXCHG( 3, 5 );
      CMPXCHG( 2, 6 ); CMPXCHG( 2, 3 ); CMPXCHG( 3, 6 );
      CMPXCHG( 4, 5 ); CMPXCHG( 1, 4 ); CMPXCHG( 1, 3 );
      return pcl::Min( i[3], i[4] );
   case  8:
      CMPXCHG( 0, 4 ); CMPXCHG( 1, 5 ); CMPXCHG( 2, 6 );
      CMPXCHG( 3, 7 ); CMPXCHG( 0, 2 ); CMPXCHG( 1, 3 );
      CMPXCHG( 4, 6 ); CMPXCHG( 5, 7 ); CMPXCHG( 2, 4 );
      CMPXCHG( 3, 5 ); CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 );
      CMPXCHG( 4, 5 ); CMPXCHG( 6, 7 ); CMPXCHG( 1, 4 );
      CMPXCHG( 3, 6 );
      return MEAN( i[3], i[4] );
   case  9:
      CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 ); CMPXCHG( 7, 8 );
      CMPXCHG( 0, 1 ); CMPXCHG( 3, 4 ); CMPXCHG( 6, 7 );
      CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 ); CMPXCHG( 7, 8 );
      CMPXCHG( 0, 3 ); CMPXCHG( 5, 8 ); CMPXCHG( 4, 7 );
      CMPXCHG( 3, 6 ); CMPXCHG( 1, 4 ); CMPXCHG( 2, 5 );
      CMPXCHG( 4, 7 ); CMPXCHG( 4, 2 ); CMPXCHG( 6, 4 );
      return pcl::Min( i[2], i[4] );
   default:
      {
         double m = double( *pcl::Select( i, j, n >> 1 ) );
         if ( n & 1 )
            return m;
         return MEAN( m, double( *pcl::Select( i, j, (n >> 1)-1 ) ) );
      }
   }
}

#undef CMPXCHG

#define CMPXCHG( a, b )  \
   if ( p( i[b], i[a] ) ) pcl::Swap( i[a], i[b] )

/*!
 * Returns the median value of a sequence [i,j), altering the existing order of
 * elements in the input sequence.
 *
 * Element comparison is given by a binary predicate \a p such that p( a, b )
 * is true for any pair a, b of elements such that a precedes b.
 *
 * We use fast, hard-coded selection networks for sequences of 9 or less
 * elements, and a quick selection algorithm for larger sets.
 *
 * See the documentation of MedianDestructive( T*, T* ) for more information
 * and references.
 *
 * \ingroup statistical_functions
 */
template <typename T, class BP> inline double MedianDestructive( T* __restrict__ i, T* __restrict__ j, BP p ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;

   switch ( n )
   {
   case  1: // !?
      return i[0];
   case  2:
      return MEAN( i[0], i[1] );
   case  3:
      CMPXCHG( 0, 1 ); CMPXCHG( 1, 2 );
      return pcl::Max( i[0], i[1] );
   case  4:
      CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 ); CMPXCHG( 0, 2 );
      CMPXCHG( 1, 3 );
      return MEAN( i[1], i[2] );
   case  5:
      CMPXCHG( 0, 1 ); CMPXCHG( 3, 4 ); CMPXCHG( 0, 3 );
      CMPXCHG( 1, 4 ); CMPXCHG( 1, 2 ); CMPXCHG( 2, 3 );
      return pcl::Max( i[1], i[2] );
   case  6:
      CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 ); CMPXCHG( 0, 2 );
      CMPXCHG( 1, 3 ); CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 );
      CMPXCHG( 0, 4 ); CMPXCHG( 1, 5 ); CMPXCHG( 1, 4 );
      CMPXCHG( 2, 4 ); CMPXCHG( 3, 5 ); CMPXCHG( 3, 4 );
      return MEAN( i[2], i[3] );
   case  7:
      CMPXCHG( 0, 5 ); CMPXCHG( 0, 3 ); CMPXCHG( 1, 6 );
      CMPXCHG( 2, 4 ); CMPXCHG( 0, 1 ); CMPXCHG( 3, 5 );
      CMPXCHG( 2, 6 ); CMPXCHG( 2, 3 ); CMPXCHG( 3, 6 );
      CMPXCHG( 4, 5 ); CMPXCHG( 1, 4 ); CMPXCHG( 1, 3 );
      return pcl::Min( i[3], i[4] );
   case  8:
      CMPXCHG( 0, 4 ); CMPXCHG( 1, 5 ); CMPXCHG( 2, 6 );
      CMPXCHG( 3, 7 ); CMPXCHG( 0, 2 ); CMPXCHG( 1, 3 );
      CMPXCHG( 4, 6 ); CMPXCHG( 5, 7 ); CMPXCHG( 2, 4 );
      CMPXCHG( 3, 5 ); CMPXCHG( 0, 1 ); CMPXCHG( 2, 3 );
      CMPXCHG( 4, 5 ); CMPXCHG( 6, 7 ); CMPXCHG( 1, 4 );
      CMPXCHG( 3, 6 );
      return MEAN( i[3], i[4] );
   case  9:
      CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 ); CMPXCHG( 7, 8 );
      CMPXCHG( 0, 1 ); CMPXCHG( 3, 4 ); CMPXCHG( 6, 7 );
      CMPXCHG( 1, 2 ); CMPXCHG( 4, 5 ); CMPXCHG( 7, 8 );
      CMPXCHG( 0, 3 ); CMPXCHG( 5, 8 ); CMPXCHG( 4, 7 );
      CMPXCHG( 3, 6 ); CMPXCHG( 1, 4 ); CMPXCHG( 2, 5 );
      CMPXCHG( 4, 7 ); CMPXCHG( 4, 2 ); CMPXCHG( 6, 4 );
      return pcl::Min( i[2], i[4] );
   default:
      {
         double m = double( *pcl::Select( i, j, n >> 1, p ) );
         if ( n & 1 )
            return m;
         return MEAN( m, double( *pcl::Select( i, j, (n >> 1)-1, p ) ) );
      }
   }
}

#undef CMPXCHG
#undef MEAN

/*!
 * Returns the k-th order statistic of a sequence [i,j).
 *
 * For scalar data types the following algorithms are used:
 *
 * \li A quick selection algorithm for sequences of up to about 2M elements.
 * The actual limit has been determined empirically and can vary across PCL
 * versions. This single-threaded algorithm can use up to 16 MiB of additional
 * memory allocated dynamically (for 8-byte types such as \c double).
 *
 * \li A parallelized, fast histogram-based algorithm for sequences larger than
 * the limit described above. This algorithm has negligible additional memory
 * space requirements.
 *
 * For non-scalar data types, this function requires the following type
 * conversion operator publicly defined for the type T:
 *
 * \code T::operator double() const; \endcode
 *
 * This operator will be used to generate a temporary dynamic array of
 * \c double values with the length of the input sequence, which will be used
 * to compute the median with the quick selection algorithm.
 *
 * \b References (quick select algorithm)
 *
 * \li William H. Press et al., <em>Numerical Recipes 3rd Edition: The Art of
 * Scientific Computing,</em> Cambridge University Press, 2007, Section 8.5.
 *
 * \li Robert Sedgewick, Kevin Wayne, <em>Algorithms, 4th Edition,</em>
 * Addison-Wesley Professional, 2011, pp 345-347.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double OrderStatistic( const T* __restrict__ i, const T* __restrict__ j, distance_type k )
{
   distance_type n = j - i;
   if ( n < 1 || k < 0 || k >= n )
      return 0;
   if ( n == 1 )
      return double( *i );
   double* d = new double[ n ];
   double* t = d;
   do
      *t++ = double( *i++ );
   while ( i < j );
   double s = *pcl::Select( d, t, k );
   delete [] d;
   return s;
}

double PCL_FUNC OrderStatistic( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const signed char* __restrict__ i, const signed char* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const signed short* __restrict__ i, const signed short* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const signed int* __restrict__ i, const signed int* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const signed long* __restrict__ i, const signed long* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const signed long long* __restrict__ i, const signed long long* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const float* __restrict__ i, const float* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const double* __restrict__ i, const double* __restrict__ j, distance_type k );
double PCL_FUNC OrderStatistic( const long double* __restrict__ i, const long double* __restrict__ j, distance_type k );

/*!
 * Returns the k-th order statistic of a sequence [i,j), altering the existing
 * order of elements in the input sequence.
 *
 * This function is intended for sequences of non-scalar objects where the
 * order of elements is irrelevant, and hence generation of a working duplicate
 * is unnecessary. The following type conversion operator must be publicly
 * defined for the type T:
 *
 * <\code> T::operator double() const; \endcode
 *
 * The quick selection algorithm is used to find the k-th element in the
 * ordered sequence.
 *
 * \note This is a \e destructive algorithm: it alters the existing order of
 * items in the input [i,j) sequence.
 *
 * \b References (quick select algorithm)
 *
 * \li William H. Press et al., <em>Numerical Recipes 3rd Edition: The Art of
 * Scientific Computing,</em> Cambridge University Press, 2007, Section 8.5.
 *
 * \li Robert Sedgewick, Kevin Wayne, <em>Algorithms, 4th Edition,</em>
 * Addison-Wesley Professional, 2011, pp 345-347.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double OrderStatisticDestructive( T* __restrict__ i, T* __restrict__ j, distance_type k ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 || k < 0 || k >= n )
      return 0;
   if ( n == 1 )
      return double( *i );
   return double( *pcl::Select( i, j, k ) );
}

/*!
 * Returns the k-th order statistic of a sequence [i,j), altering the existing
 * order of elements in the input sequence.
 *
 * Element comparison is given by a binary predicate \a p such that p( a, b )
 * is true for any pair a, b of elements such that a precedes b.
 *
 * See the documentation of OrderStatisticDestructive( T*, T*, distance_type )
 * for more information and references.
 *
 * \ingroup statistical_functions
 */
template <typename T, class BP> inline double OrderStatisticDestructive( const T* __restrict__ i, const T* __restrict__ j, distance_type k, BP p ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 || k < 0 || k >= n )
      return 0;
   if ( n == 1 )
      return double( *i );
   return double( *pcl::Select( i, j, k, p ) );
}

/*!
 * Computes the two-sided, asymmetric trimmed mean of a sequence [i,j).
 *
 * The returned value is the arithmetic mean of a sequence [I+l,J-h-1], where
 * [I,J) is the input sequence [i,j) sorted in ascending order.
 *
 * Let n = j-i be the length of the input sequence. For empty sequences
 * (n &le; 0) or completely truncated sequences (l+h >= n), this function
 * returns zero. Otherwise the returned value is the arithmetic mean of the
 * nonrejected n-l-h elements in the sorted sequence, as described above.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double TrimmedMean( const T* __restrict__ i, const T* __restrict__ j, distance_type l = 1, distance_type h = 1 )
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;
   if ( l+h < 1 )
      return Sum( i, j )/n;
   if ( l+h >= n )
      return 0;
   for ( double s = 0,
         t0 = OrderStatistic( i, j, l ),
         t1 = OrderStatistic( i, j, n-h-1 ); ; )
   {
      double x = double( *i );
      if ( x >= t0 )
         if ( x <= t1 )
            s += x;
      if ( ++i == j )
         return s/(n - l - h);
   }
}

/*!
 * Computes the two-sided, asymmetric trimmed mean of a sequence [i,j),
 * possibly altering the existing order of elements in the input sequence.
 *
 * The returned value is the arithmetic mean of a sequence [I+l,J-h-1], where
 * [I,J) represents the input sequence [i,j) sorted in ascending order.
 *
 * Let n = j-i be the length of the input sequence. For empty sequences
 * (n &le; 0) or completely truncated sequences (l+h >= n), this function
 * returns zero. Otherwise the returned value is the arithmetic mean of the
 * nonrejected n-l-h elements in the sorted sequence, as described above.
 *
 * \note This is a \e destructive trimmed mean calculation algorithm: it may
 * alter the existing order of items in the input [i,j) sequence.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double TrimmedMeanDestructive( T* __restrict__ i, T* __restrict__ j, distance_type l = 1, distance_type h = 1 ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;
   if ( l+h < 1 )
      return Sum( i, j )/n;
   if ( l+h >= n )
      return 0;
   for ( double s = 0,
         t0 = OrderStatisticDestructive( i, j, l ),
         t1 = OrderStatisticDestructive( i, j, n-h-1 ); ; )
   {
      double x = double( *i );
      if ( x >= t0 )
         if ( x <= t1 )
            s += x;
      if ( ++i == j )
         return s/(n - l - h);
   }
}

/*!
 * Computes the two-sided, asymmetric trimmed mean of squares of a sequence
 * [i,j).
 *
 * The returned value is the arithmetic mean of squares of a sequence
 * [I+l,J-h-1], where [I,J) represents the input sequence [i,j) sorted in
 * ascending order.
 *
 * Let n = j-i be the length of the input sequence. For empty sequences
 * (n &le; 0) or completely truncated sequences (l+h >= n), this function
 * returns zero. Otherwise the returned value is the arithmetic mean of the
 * squared nonrejected n-l-h elements in the sorted sequence, as described
 * above.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double TrimmedMeanOfSquares( const T* __restrict__ i, const T* __restrict__ j, distance_type l = 1, distance_type h = 1 )
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;
   if ( l+h < 1 )
      return Sum( i, j )/n;
   if ( l+h >= n )
      return 0;
   for ( double s = 0,
         t0 = OrderStatistic( i, j, l ),
         t1 = OrderStatistic( i, j, n-h-1 ); ; )
   {
      double x = double( *i );
      if ( x >= t0 )
         if ( x <= t1 )
            s += x*x;
      if ( ++i == j )
         return s/(n - l - h);
   }
}

/*!
 * Computes the two-sided, asymmetric trimmed mean of squares of a sequence
 * [i,j), possibly altering the existing order of elements in the input
 * sequence.
 *
 * The returned value is the arithmetic mean of squares of a sequence
 * [I+l,J-h-1], where [I,J) represents the input sequence [i,j) sorted in
 * ascending order.
 *
 * Let n = j-i be the length of the input sequence. For empty sequences
 * (n &le; 0) or completely truncated sequences (l+h >= n), this function
 * returns zero. Otherwise the returned value is the arithmetic mean of the
 * squared nonrejected n-l-h elements in the sorted sequence, as described
 * above.
 *
 * \note This is a \e destructive trimmed mean of squares calculation
 * algorithm: it may alter the existing order of items in the input [i,j)
 * sequence.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double TrimmedMeanOfSquaresDestructive( T* __restrict__ i, T* __restrict__ j, distance_type l = 1, distance_type h = 1 ) noexcept
{
   distance_type n = j - i;
   if ( n < 1 )
      return 0;
   if ( l+h < 1 )
      return Sum( i, j )/n;
   if ( l+h >= n )
      return 0;
   for ( double s = 0,
         t0 = OrderStatisticDestructive( i, j, l ),
         t1 = OrderStatisticDestructive( i, j, n-h-1 ); ; )
   {
      double x = double( *i );
      if ( x >= t0 )
         if ( x <= t1 )
            s += x*x;
      if ( ++i == j )
         return s/(n - l - h);
   }
}

/*!
 * Returns the average absolute deviation of the values in a sequence [i,j)
 * with respect to the specified \a center value.
 *
 * When the median of the sequence is used as the center value, this function
 * returns the average absolute deviation from the median, which is a
 * well-known estimator of dispersion.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * See the remarks made for the Sum() function, which are equally applicable in
 * this case. See StableAvgDev() for a (slow) numerically stable version of
 * this function.
 *
 * \note To make the average absolute deviation about the median consistent
 * with the standard deviation of a normal distribution, it must be
 * multiplied by the constant 1.2533.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double AvgDev( const T* __restrict__ i, const T* __restrict__ j, double center ) noexcept
{
   distance_type n = j - i;
   if ( n < 2 )
      return 0;
   double d = 0;
   do
      d += Abs( double( *i++ ) - center );
   while ( i < j );
   return d/n;
}

/*!
 * Returns the average absolute deviation of the values in a sequence [i,j)
 * with respect to the specified \a center value, using a numerically stable
 * summation algorithm to minimize roundoff error.
 *
 * When the median of the sequence is used as the center value, this function
 * returns the average absolute deviation from the median, which is a
 * well-known estimator of dispersion.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * In the current PCL versions, this function implements the Kahan summation
 * algorithm to reduce roundoff error to the machine's floating point error.
 *
 * \note To make the average absolute deviation about the median consistent
 * with the standard deviation of a normal distribution, it must be
 * multiplied by the constant 1.2533.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StableAvgDev( const T* __restrict__ i, const T* __restrict__ j, double center ) noexcept
{
   distance_type n = j - i;
   if ( n < 2 )
      return 0;
   double sum = 0;
   double eps = 0;
   do
   {
      double y = Abs( double( *i++ ) - center ) - eps;
      double t = sum + y;
      eps = (t - sum) - y;
      sum = t;
   }
   while ( i < j );
   return sum/n;
}

/*!
 * Returns the average absolute deviation from the median of the values in a
 * sequence [i,j).
 *
 * The average absolute deviation from the median is a well-known estimator of
 * dispersion.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * See the remarks made for the Sum() function, which are equally applicable in
 * this case. See StableAvgDev() for a (slow) numerically stable version of
 * this function.
 *
 * \note To make the average absolute deviation about the median consistent
 * with the standard deviation of a normal distribution, it must be
 * multiplied by the constant 1.2533.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double AvgDev( const T* __restrict__ i, const T* __restrict__ j )
{
   distance_type n = j - i;
   if ( n < 2 )
      return 0;
   double m = Median( i, j );
   double d = 0;
   do
      d += Abs( double( *i++ ) - m );
   while ( i < j );
   return d/n;
}

/*!
 * Computes the average absolute deviation from the median of the values in a
 * sequence [i,j) using a numerically stable summation algorithm to minimize
 * roundoff error.
 *
 * The average absolute deviation from the median is a well-known estimator of
 * dispersion.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * In the current PCL versions, this function implements the Kahan summation
 * algorithm to reduce roundoff error to the machine's floating point error.
 *
 * \note To make the average absolute deviation about the median consistent
 * with the standard deviation of a normal distribution, it must be
 * multiplied by the constant 1.2533.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double StableAvgDev( const T* __restrict__ i, const T* __restrict__ j )
{
   return pcl::StableAvgDev( i, j, pcl::Median( i, j ) );
}

/*!
 * \struct TwoSidedEstimate
 * \brief Two-sided descriptive statistical estimate.
 *
 * This POD structure is returned by functions implementing two-sided scale
 * estimators. Given a sample X = {x_0,...,x_n-1} and a reference center value
 * m (typically, the median of X), a two-sided scale estimate is computed as
 * two separate components: A <em>low estimate</em> for all x in X such that
 * x &le; m, and a <em>high estimate</em> for all x in X such that x > m.
 *
 * Two-sided scale estimates are important in normalization for accurate
 * outlier rejection and sample distribution characterization, especially for
 * skewed or asymmetric distributions.
 *
 * \ingroup statistical_functions
 * \sa TwoSidedAvgDev(), TwoSidedMAD(), TwoSidedBiweightMidvariance()
 */
struct TwoSidedEstimate
{
   double low = 0;  //!< Low estimate component.
   double high = 0; //!< High estimate component.

   /*!
    * Default constructor. Both components are initialized to zero.
    */
   TwoSidedEstimate() = default;

   /*!
    * Copy constructor.
    */
   TwoSidedEstimate( const TwoSidedEstimate& ) = default;

   /*!
    * Move constructor.
    */
   TwoSidedEstimate( TwoSidedEstimate&& ) = default;

   /*!
    * Copy assignment operator.
    */
   TwoSidedEstimate& operator =( const TwoSidedEstimate& ) = default;

   /*!
    * Move assignment operator.
    */
   TwoSidedEstimate& operator =( TwoSidedEstimate&& ) = default;

   /*!
    * Constructor from separate low and high components.
    */
   template <typename T1, typename T2>
   TwoSidedEstimate( const T1& l, const T2& h )
      : low( double( l ) )
      , high( double( h ) )
   {
   }

   /*!
    * Constructor from a unique component value \a x, which is assigned to both
    * the low and high estimate components.
    */
   template <typename T>
   TwoSidedEstimate( const T& x )
   {
      low = high = double( x );
   }

   /*!
    * Returns true iff this two-sided scale estimate is valid. A two-sided
    * scale estimate is valid if both the low and high components are finite,
    * positive and nonzero with respect to the machine epsilon for the type
    * \c double.
    */
   bool IsValid() const noexcept
   {
      return IsFinite( low ) && low > std::numeric_limits<double>::epsilon()
          && IsFinite( high ) && high > std::numeric_limits<double>::epsilon();
   }

   /*!
    * Conversion to scalar. Returns the arithmetic mean of the low and high
    * estimates if both are nonzero. Returns the nonzero estimate otherwise if
    * it exists, zero otherwise.
    */
   explicit operator double() const noexcept
   {
      if ( low != 0 )
      {
         if ( high != 0 )
            return (low + high)/2;
         return low;
      }
      return high;
   }

   /*!
    * Assignment-multiplication by a scalar. Returns a reference to this
    * object.
    */
   TwoSidedEstimate& operator *=( double x ) noexcept
   {
      low *= x;
      high *= x;
      return *this;
   }

   /*!
    * Assignment-division by a scalar. Returns a reference to this object.
    */
   TwoSidedEstimate& operator /=( double x ) noexcept
   {
      low /= x;
      high /= x;
      return *this;
   }

   /*!
    * Assignment-division by a two-sided estimate. Returns a reference to this
    * object.
    */
   TwoSidedEstimate& operator /=( const TwoSidedEstimate& e ) noexcept
   {
      low /= e.low;
      high /= e.high;
      return *this;
   }

   /*!
    * Returns the result of multiplying this two-sided estimate by a scalar.
    */
   TwoSidedEstimate operator *( double x ) const noexcept
   {
      return { low*x, high*x };
   }

   /*!
    * Returns the result of dividing this two-sided estimate by a scalar.
    */
   TwoSidedEstimate operator /( double x ) const noexcept
   {
      return { low/x, high/x };
   }

   /*!
    * Returns the result of the component wise division of this two-sided
    * estimate by another two-sided estimate.
    */
   TwoSidedEstimate operator /( const TwoSidedEstimate& e ) const noexcept
   {
      return { low/e.low, high/e.high };
   }
};

/*!
 * Returns the component wise square root of a two-sided estimate.
 * \ingroup statistical_functions
 */
inline TwoSidedEstimate Sqrt( const TwoSidedEstimate& e ) noexcept
{
   return { Sqrt( e.low ), Sqrt( e.high ) };
}

/*!
 * Returns the component wise exponent function of a two-sided estimate.
 * \ingroup statistical_functions
 */
template <typename T> inline TwoSidedEstimate Pow( const TwoSidedEstimate& e, T x ) noexcept
{
   double x_ = double( x );
   return { Pow( e.low, x_ ), Pow( e.high, x_ ) };
}

/*!
 * Returns the two-sided average absolute deviation of the values in a sequence
 * [i,j) with respect to the specified \a center value.
 *
 * When the median of the sequence is used as the center value, this function
 * returns the average absolute deviation from the median, which is a
 * well-known estimator of dispersion.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * See the remarks made for the Sum() function, which are equally applicable in
 * this case. See StableAvgDev() for a (slow) numerically stable version of
 * this function.
 *
 * \note To make the average absolute deviation about the median consistent
 * with the standard deviation of a normal distribution, it must be
 * multiplied by the constant 1.2533.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline TwoSidedEstimate TwoSidedAvgDev( const T* __restrict__ i, const T* __restrict__ j, double center ) noexcept
{
   double dl = 0, dh = 0;
   distance_type nl = 0, nh = 0;
   while ( i < j )
   {
      double x = double( *i++ );
      if ( x <= center )
      {
         dl += center - x;
         ++nl;
      }
      else
      {
         dh += x - center;
         ++nh;
      }
   }
   return { (nl > 1) ? dl/nl : 0.0,
            (nh > 1) ? dh/nh : 0.0 };
}

/*!
 * Returns the two-sided average absolute deviation from the median of the
 * values in a sequence [i,j).
 *
 * The average absolute deviation from the median is a well-known estimator of
 * dispersion.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * See the remarks made for the Sum() function, which are equally applicable in
 * this case. See StableAvgDev() for a (slow) numerically stable version of
 * this function.
 *
 * \note To make the average absolute deviation about the median consistent
 * with the standard deviation of a normal distribution, it must be
 * multiplied by the constant 1.2533.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline TwoSidedEstimate TwoSidedAvgDev( const T* __restrict__ i, const T* __restrict__ j )
{
   return pcl::TwoSidedAvgDev( i, j, pcl::Median( i, j ) );
}

/*!
 * Returns the median absolute deviation (MAD) of the values in a sequence
 * [i,j) with respect to the specified \a center value.
 *
 * The MAD is a well-known robust estimator of scale.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \note To make the MAD estimator consistent with the standard deviation of
 * a normal distribution, its result must be multiplied by the constant 1.4826.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double MAD( const T* __restrict__ i, const T* __restrict__ j, double center )
{
   distance_type n = j - i;
   if ( n < 2 )
      return 0;
   double* d = new double[ n ];
   double* p = d;
   do
      *p++ = Abs( double( *i++ ) - center );
   while ( i < j );
   double m = pcl::Median( d, d+n );
   delete [] d;
   return m;
}

double PCL_FUNC MAD( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j, double center );
double PCL_FUNC MAD( const signed char* __restrict__ i, const signed char* __restrict__ j, double center );
double PCL_FUNC MAD( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j, double center );
double PCL_FUNC MAD( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j, double center );
double PCL_FUNC MAD( const signed short* __restrict__ i, const signed short* __restrict__ j, double center );
double PCL_FUNC MAD( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j, double center );
double PCL_FUNC MAD( const signed int* __restrict__ i, const signed int* __restrict__ j, double center );
double PCL_FUNC MAD( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j, double center );
double PCL_FUNC MAD( const signed long* __restrict__ i, const signed long* __restrict__ j, double center );
double PCL_FUNC MAD( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j, double center );
double PCL_FUNC MAD( const signed long long* __restrict__ i, const signed long long* __restrict__ j, double center );
double PCL_FUNC MAD( const float* __restrict__ i, const float* __restrict__ j, double center );
double PCL_FUNC MAD( const double* __restrict__ i, const double* __restrict__ j, double center );
double PCL_FUNC MAD( const long double* __restrict__ i, const long double* __restrict__ j, double center );

/*!
 * Returns the median absolute deviation from the median (MAD) for the values
 * in a sequence [i,j).
 *
 * The MAD is a well-known robust estimator of scale.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \note To make the MAD estimator consistent with the standard deviation of
 * a normal distribution, its result must be multiplied by the constant 1.4826.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline double MAD( const T* __restrict__ i, const T* __restrict__ j )
{
   return pcl::MAD( i, j, pcl::Median( i, j ) );
}

/*!
 * Returns the two-sided median absolute deviation (MAD) of the values in a
 * sequence [i,j) with respect to the specified \a center value.
 *
 * The MAD is a well-known robust estimator of scale.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \note To make the MAD estimator consistent with the standard deviation of
 * a normal distribution, its result must be multiplied by the constant 1.4826.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline TwoSidedEstimate TwoSidedMAD( const T* __restrict__ i, const T* __restrict__ j, double center )
{
   distance_type n = j - i;
   if ( n < 2 )
      return 0;
   double* d = new double[ n ];
   double* __restrict__ p = d;
   double* __restrict__ q = d + n;
   do
   {
      double x = double( *i++ );
      if ( x <= center )
         *p++ = center - x;
      else
         *--q = x - center;
   }
   while( i < j );
   double l = pcl::Median( d, p );
   double h = pcl::Median( q, d+n );
   delete [] d;
   return { l, h };
}

TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned char* __restrict__ i, const unsigned char* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed char* __restrict__ i, const signed char* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const wchar_t* __restrict__ i, const wchar_t* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned short* __restrict__ i, const unsigned short* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed short* __restrict__ i, const signed short* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned int* __restrict__ i, const unsigned int* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed int* __restrict__ i, const signed int* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned long* __restrict__ i, const unsigned long* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed long* __restrict__ i, const signed long* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const unsigned long long* __restrict__ i, const unsigned long long* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const signed long long* __restrict__ i, const signed long long* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const float* __restrict__ i, const float* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const double* __restrict__ i, const double* __restrict__ j, double center );
TwoSidedEstimate PCL_FUNC TwoSidedMAD( const long double* __restrict__ i, const long double* __restrict__ j, double center );

/*!
 * Returns the two-sided median absolute deviation from the median (MAD) for
 * the values in a sequence [i,j).
 *
 * The MAD is a well-known robust estimator of scale.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \note To make the MAD estimator consistent with the standard deviation of
 * a normal distribution, its result must be multiplied by the constant 1.4826.
 *
 * \ingroup statistical_functions
 */
template <typename T> inline TwoSidedEstimate TwoSidedMAD( const T* __restrict__ i, const T* __restrict__ j )
{
   return pcl::TwoSidedMAD( i, j, pcl::Median( i, j ) );
}

/*!
 * Returns the Sn scale estimator of Rousseeuw and Croux for a sequence [x,xn):
 *
 * <pre>
 * Sn = c * low_median( high_median( |x_i - x_j| ) )
 * </pre>
 *
 * where low_median() is the order statistic of rank (n + 1)/2, and
 * high_median() is the order statistic of rank n/2 + 1. n >= 2 is the number
 * of elements in the sequence: n = xn - x.
 *
 * This implementation is a direct C++ translation of the original FORTRAN
 * code by the authors (see References). The algorithm has O(n*log_2(n)) time
 * complexity and uses O(n) additional storage.
 *
 * The constant c = 1.1926 must be used to make the Sn estimator converge to
 * the standard deviation of a pure normal distribution. However, this
 * implementation does not apply it (it uses c=1 implicitly), for
 * consistency with other implementations of scale estimators.
 *
 * \b References
 *
 * P.J. Rousseeuw and C. Croux (1993), <em>Alternatives to the Median Absolute
 * Deviation,</em> Journal of the American Statistical Association, Vol. 88,
 * pp. 1273-1283.
 *
 * \note This is a \e destructive algorithm: it may alter the initial order of
 * items in the specified [x,xn) sequence.
 *
 * \ingroup statistical_functions
 */
template <typename T> double Sn( T* __restrict__ x, T* __restrict__ xn )
{
   /*
    * N.B.: In the code below, lines commented with an asterisk (*) have been
    * modified with respect to the FORTRAN original to account for zero-based
    * array indices.
    */

   distance_type n = xn - x;
   if ( n < 2 )
      return 0;

   pcl::Sort( x, xn );

   double* a2 = new double[ n ];
   a2[0] = double( x[n >> 1] ) - double( x[0] );                                 // *

   distance_type nh = (n + 1) >> 1;

   for ( distance_type i = 2; i <= nh; ++i )
   {
      distance_type nA = i-1;
      distance_type nB = n - i;
      distance_type diff = nB - nA;
      distance_type leftA = 1;
      distance_type leftB = 1;
      distance_type rightA = nB;
      distance_type Amin = (diff >> 1) + 1;
      distance_type Amax = (diff >> 1) + nA;

      while ( leftA < rightA )
      {
         distance_type length = rightA - leftA + 1;
         distance_type even = (length & 1) == 0;
         distance_type half = (length - 1) >> 1;
         distance_type tryA = leftA + half;
         distance_type tryB = leftB + half;

         if ( tryA < Amin )
            leftA = tryA + even;
         else
         {
            if ( tryA > Amax )
            {
               rightA = tryA;
               leftB = tryB + even;
            }
            else
            {
               double medA = double( x[i-1] ) - double( x[i-2 - tryA + Amin] );  // *
               double medB = double( x[tryB + i-1] ) - double( x[i-1] );         // *
               if ( medA >= medB )
               {
                  rightA = tryA;
                  leftB = tryB + even;
               }
               else
                  leftA = tryA + even;
            }
         }
      }

      if ( leftA > Amax )
         a2[i-1] = double( x[leftB + i-1] ) - double( x[i-1] );                  // *
      else
      {
         double medA = double( x[i-1] ) - double( x[i-2 - leftA + Amin] );       // *
         double medB = double( x[leftB + i-1] ) - double( x[i-1] );
         a2[i-1] = pcl::Min( medA, medB );                                       // *
      }
   }

   for ( distance_type i = nh + 1; i < n; ++i )
   {
      distance_type nA = n - i;
      distance_type nB = i - 1;
      distance_type diff = nB - nA;
      distance_type leftA = 1;
      distance_type leftB = 1;
      distance_type rightA = nB;
      distance_type Amin = (diff >> 1) + 1;
      distance_type Amax = (diff >> 1) + nA;

      while ( leftA < rightA )
      {
         distance_type length = rightA - leftA + 1;
         distance_type even = (length & 1) == 0;
         distance_type half = (length - 1) >> 1;
         distance_type tryA = leftA + half;
         distance_type tryB = leftB + half;

         if ( tryA < Amin)
            leftA = tryA + even;
         else
         {
            if ( tryA > Amax )
            {
               rightA = tryA;
               leftB = tryB + even;
            }
            else
            {
               double medA = double( x[i + tryA - Amin] ) - double( x[i-1] );    // *
               double medB = double( x[i-1] ) - double( x[i-1 - tryB] );         // *
               if ( medA >= medB )
               {
                  rightA = tryA;
                  leftB = tryB + even;
               }
               else
                  leftA = tryA + even;
            }
         }
      }

      if ( leftA > Amax )
         a2[i-1] = double( x[i-1] ) - double( x[i-1 - leftB] );                  // *
      else
      {
         double medA = double( x[i + leftA - Amin] ) - double( x[i-1] );         // *
         double medB = double( x[i-1] ) - double( x[i-1 - leftB] );              // *
         a2[i-1] = pcl::Min( medA, medB );            // *
      }
   }

   a2[n-1] = double( x[n-1] ) - double( x[nh-1] );                               // *

   /*
    * Correction for a finite sample
    */
   double cn;
   switch ( n )
   {
   case  2: cn = 0.743; break;
   case  3: cn = 1.851; break;
   case  4: cn = 0.954; break;
   case  5: cn = 1.351; break;
   case  6: cn = 0.993; break;
   case  7: cn = 1.198; break;
   case  8: cn = 1.005; break;
   case  9: cn = 1.131; break;
   default: cn = (n & 1) ? n/(n - 0.9) : 1.0; break;
   }

   double sn = cn * *pcl::Select( a2, a2+n, nh-1 );

   delete [] a2;
   return sn;
}

/*!
 * \internal
 * Auxiliary routine for Qn().
 *
 * Algorithm to compute the weighted high median in O(n) time.
 *
 * The weighted high median is defined as the smallest a[j] such that the sum
 * of the weights of all a[i] <= a[j] is strictly greater than half of the
 * total weight.
 */
inline double __pcl_whimed__( double* a, distance_type* iw, distance_type n,
                              double* acand, distance_type* iwcand )
{
   distance_type wtotal = 0;
   for ( distance_type i = 0; i < n; ++i )
      wtotal += iw[i];

   for ( distance_type nn = n, wrest = 0; ; )
   {
      double trial = *pcl::Select( a, a+nn, nn >> 1 ); // *

      distance_type wleft = 0;
      distance_type wmid = 0;
      distance_type wright = 0;
      for ( distance_type i = 0; i < nn; ++i )
         if ( a[i] < trial )
            wleft += iw[i];
         else if ( a[i] > trial )
            wright += iw[i];
         else
            wmid += iw[i];

      if ( 2*(wrest + wleft) > wtotal )
      {
         distance_type kcand = 0;
         for ( distance_type i = 0; i < nn; ++i )
            if ( a[i] < trial )
            {
               acand[kcand] = a[i];
               iwcand[kcand] = iw[i];
               ++kcand;
            }
          nn = kcand;
      }
      else
      {
         if ( 2*(wrest + wleft + wmid) > wtotal )
            return trial;

         distance_type kcand = 0;
         for ( distance_type i = 0; i < nn; ++i )
            if ( a[i] > trial )
            {
               acand[kcand] = a[i];
               iwcand[kcand] = iw[i];
               ++kcand;
            }
         nn = kcand;
         wrest += wleft + wmid;
      }

      for ( distance_type i = 0; i < nn; ++i )
      {
         a[i] = acand[i];
         iw[i] = iwcand[i];
      }
   }
}

/*!
 * Returns the Qn scale estimator of Rousseeuw and Croux for a sequence [x,xn):
 *
 * <pre>
 * Qn = c * first_quartile( |x_i - x_j| : i < j )
 * </pre>
 *
 * where first_quartile() is the order statistic of rank (n + 1)/4. n >= 2 is
 * the number of elements in the sequence: n = xn - x.
 *
 * This implementation is a C++ translation of the original FORTRAN code by the
 * authors (see References). The algorithm has O(n*log_2(n)) time complexity
 * and the implementation requires about O(9*n) additional storage.
 *
 * The constant c = 2.2219 must be used to make the Qn estimator converge to
 * the standard deviation of a pure normal distribution. However, this
 * implementation does not apply it (it uses c=1 implicitly), for consistency
 * with other implementations of scale estimators.
 *
 * \b References
 *
 * P.J. Rousseeuw and C. Croux (1993), <em>Alternatives to the Median Absolute
 * Deviation,</em> Journal of the American Statistical Association, Vol. 88,
 * pp. 1273-1283.
 *
 * \note This is a \e destructive algorithm: it may alter the initial order of
 * items in the specified [x,xn) sequence.
 *
 * \ingroup statistical_functions
 */
template <typename T> double Qn( T* __restrict__ x, T* __restrict__ xn )
{
   distance_type n = xn - x;
   if ( n < 2 )
      return 0;

   double*        y      = new double[ n ];
   double*        work   = new double[ n ];
   double*        acand  = new double[ n ];
   distance_type* iwcand = new distance_type[ n ];
   distance_type* left   = new distance_type[ n ];
   distance_type* right  = new distance_type[ n ];
   distance_type* P      = new distance_type[ n ];
   distance_type* Q      = new distance_type[ n ];
   distance_type* weight = new distance_type[ n ];

   distance_type h = (n >> 1) + 1;
   distance_type k = (h*(h - 1)) >> 1;
   for ( distance_type i = 0; i < n; ++i )
   {
      y[i] = double( x[i] );
      left[i] = n - i + 1;                                                       // *
      right[i] = (i <= h) ? n : n - i + h; // N.B. The original code is "right[i] = n"
   }

   pcl::Sort( y, y+n );

   distance_type nL = (n*(n + 1)) >> 1;
   distance_type nR = n*n;
   distance_type knew = k + nL;

   bool found = false;
   double qn;

   while ( nR-nL > n )
   {
      distance_type j = 0;                                                       // *
      for ( distance_type i = 1; i < n; ++i )                                    // *
         if ( left[i] <= right[i] )
         {
            weight[j] = right[i] - left[i] + 1;
            work[j] = double( y[i] ) - y[n - left[i] - (weight[j] >> 1)];
            ++j;
         }
      qn = __pcl_whimed__( work, weight, j, acand, iwcand );

      for ( distance_type i = n-1, j = 0; i >= 0; --i )                          // *
      {
         while ( j < n && double( y[i] ) - y[n-j-1] < qn )
            ++j;
         P[i] = j;
      }

      for ( distance_type i = 0, j = n+1; i < n; ++i )                           // *
      {
         while ( double( y[i] ) - y[n-j+1] > qn )
            --j;
         Q[i] = j;
      }

      double sumP = 0;
      double sumQ = 0;
      for ( distance_type i = 0; i < n; ++i )
      {
         sumP += P[i];
         sumQ += Q[i] - 1;
      }

      if ( knew <= sumP )
      {
         for ( distance_type i = 0; i < n; ++i )
            right[i] = P[i];
         nR = sumP;
      }
      else if ( knew > sumQ )
      {
         for ( distance_type i = 0; i < n; ++i )
            left[i] = Q[i];
         nL = sumQ;
      }
      else
      {
         found = true;
         break;
      }
   }

   if ( !found )
   {
      distance_type j = 0;
      for ( distance_type i = 1; i < n; ++i )
         for ( distance_type jj = left[i]; jj <= right[i]; ++jj, ++j )
            work[j] = double( y[i] ) - y[n-jj];                                  // *
      qn = *pcl::Select( work, work+j, knew-nL-1 );                              // *
   }

   /*
    * Correction for a finite sample
    */
   double dn;
   switch ( n )
   {
   case  2: dn = 0.399; break;
   case  3: dn = 0.994; break;
   case  4: dn = 0.512; break;
   case  5: dn = 0.844; break;
   case  6: dn = 0.611; break;
   case  7: dn = 0.857; break;
   case  8: dn = 0.669; break;
   case  9: dn = 0.872; break;
   default: dn = (n & 1) ? n/(n + 1.4) : n/(n + 3.8); break;
   }
   qn *= dn;

   delete [] y;
   delete [] work;
   delete [] acand;
   delete [] iwcand;
   delete [] left;
   delete [] right;
   delete [] P;
   delete [] Q;
   delete [] weight;

   return qn;
}

/*!
 * Returns a biweight midvariance (BWMV) for the elements in a sequence [x,xn).
 *
 * \param x, xn   Define a sequence of sample data points for which the BWMV
 *                estimator will be calculated.
 *
 * \param center  Reference center value. Normally, the median of the sample
 *                should be used.
 *
 * \param sigma   A reference estimate of dispersion. Normally, the median
 *                absolute deviation from the median (MAD) of the sample should
 *                be used.
 *
 * \param k       Rejection limit in sigma units. The default value is k=9.
 *
 * \param reducedLength    If true, reduce the sample size to exclude rejected
 *                elements. This tends to approximate the true dispersion of
 *                the data more accurately for relatively small samples, or
 *                samples with large amounts of outliers. Note that this
 *                departs from the standard definition of biweight midvariance,
 *                where the total number of data items is used to scale the
 *                variance estimate. If false, use the full sample size,
 *                including rejected elements, which gives a standard biweight
 *                midvariance estimate.
 *
 * The square root of the biweight midvariance is a robust estimator of scale.
 * It is an efficient estimator with respect to many statistical distributions
 * (about 87% Gaussian efficiency), and appears to have a breakdown point close
 * to 0.5 (the same as MAD).
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \note To make the BWMV estimator consistent with the standard deviation of
 * a normal distribution, its square root must be multiplied by the constant
 * 0.991.
 *
 * \b References
 *
 * Rand R. Wilcox (2017), <em>Introduction to Robust Estimation and Hypothesis
 * Testing, 4th Edition</em>, Elsevier Inc., Section 3.12.1.
 *
 * \ingroup statistical_functions
 */
template <typename T>
double BiweightMidvariance( const T* __restrict__ x, const T* __restrict__ xn, double center,
                            double sigma, int k = 9, bool reducedLength = false ) noexcept
{
   distance_type n = xn - x;
   if ( n < 2 )
      return 0;

   double kd = k * sigma;
   if ( kd < 0 || 1 + kd == 1 )
      return 0;

   double num = 0, den = 0;
   distance_type nr = 0;
   for ( ; x < xn; ++x )
   {
      double xc = double( *x ) - center;
      double y = xc/kd;
      if ( Abs( y ) < 1 )
      {
         double y2 = y*y;
         double y21 = 1 - y2;
         num += xc*xc * y21*y21*y21*y21;
         den += y21 * (1 - 5*y2);
         ++nr;
      }
   }

   den *= den;
   return (1 + den != 1) ? (reducedLength ? nr : n)*num/den : 0.0;
}

/*!
 * Returns a two-sided biweight midvariance (BWMV) for the elements in a
 * sequence [x,xn).
 *
 * \param x, xn   Define a sequence of sample data points for which the
 *                two-sided BWMV estimator will be calculated.
 *
 * \param center  Reference center value. Normally, the median of the sample
 *                should be used.
 *
 * \param sigma   A reference two-sided estimate of dispersion. Normally, the
 *                two-sided median absolute deviation from the median (MAD) of
 *                the sample should be used. See the TwoSidedMAD() function.
 *
 * \param k       Rejection limit in sigma units. The default value is k=9.
 *
 * \param reducedLength    If true, reduce the sample size to exclude rejected
 *                elements. Size reduction is performed separately for the low
 *                and high halves of the data. This tends to approximate the
 *                true dispersion of the data more accurately for relatively
 *                small samples, or samples with large amounts of outliers.
 *                Note that this departs from the standard definition of
 *                biweight midvariance, where the total number of data items is
 *                used to scale the variance estimate. If false, use the full
 *                sample size, including rejected elements, which gives a
 *                standard biweight midvariance estimate.
 *
 * See BiweightMidvariance() for more information and references.
 *
 * \ingroup statistical_functions
 */
template <typename T>
TwoSidedEstimate TwoSidedBiweightMidvariance( const T* __restrict__ x, const T* __restrict__ xn, double center,
                                              const TwoSidedEstimate& sigma, int k = 9, bool reducedLength = false ) noexcept
{
   double kd0 = k * sigma.low;
   double kd1 = k * sigma.high;
   if ( kd0 < 0 || 1 + kd0 == 1 || kd1 < 0 || 1 + kd1 == 1 )
      return 0;

   double num0 = 0, den0 = 0, num1 = 0, den1 = 0;
   size_type n0 = 0, n1 = 0, nr0 = 0, nr1 = 0;
   for ( ; x < xn; ++x )
   {
      double xc = double( *x ) - center;
      bool low = xc <= 0;
      if ( low )
         ++n0;
      else
         ++n1;

      double y = xc/(low ? kd0 : kd1);
      if ( pcl::Abs( y ) < 1 )
      {
         double y2 = y*y;
         double y21 = 1 - y2;
         double num = xc*xc * y21*y21*y21*y21;
         double den = y21 * (1 - 5*y2);
         if ( low )
         {
            num0 += num;
            den0 += den;
            ++nr0;
         }
         else
         {
            num1 += num;
            den1 += den;
            ++nr1;
         }
      }
   }

   den0 *= den0;
   den1 *= den1;
   return { (n0 >= 2 && 1 + den0 != 1) ? (reducedLength ? nr0 : n0)*num0/den0 : 0.0,
            (n1 >= 2 && 1 + den1 != 1) ? (reducedLength ? nr1 : n1)*num1/den1 : 0.0 };
}

/*!
 * Returns a percentage bend midvariance (PBMV) for the elements in a sequence
 * [x,xn).
 *
 * \param x, xn   Define a sequence of sample data points for which the PBWV
 *                estimator will be calculated.
 *
 * \param center  Reference center value. Normally, the median of the sample
 *                should be used.
 *
 * \param beta    Rejection parameter in the [0,0.5] range. Higher values
 *                improve robustness to outliers (i.e., increase the breakdown
 *                point of the estimator) at the expense of a lower efficiency.
 *                The default value is beta=0.2.
 *
 * The square root of the percentage bend midvariance is a robust estimator of
 * scale. With the default beta=0.2, its Gaussian efficiency is 67%. With
 * beta=0.1, its efficiency is 85% but its breakdown is only 0.1.
 *
 * For sequences of less than two elements, this function returns zero.
 *
 * \b References
 *
 * Rand R. Wilcox (2012), <em>Introduction to Robust Estimation and Hypothesis
 * Testing, 3rd Edition</em>, Elsevier Inc., Section 3.12.3.
 *
 * \ingroup statistical_functions
 */
template <typename T>
double BendMidvariance( const T* __restrict__ x, const T* __restrict__ xn, double center, double beta = 0.2 )
{
   distance_type n = xn - x;
   if ( n < 2 )
      return 0;

   beta = Range( beta, 0.0, 0.5 );
   distance_type m = Floor( (1 - beta)*n + 0.5 );

   double* w = new double[ n ];
   for ( distance_type i = 0; i < n; ++i )
      w[i] = Abs( double( x[i] ) - center );
   double wb = *pcl::Select( w, w+n, m );
   delete [] w;
   if ( 1 + wb == 1 )
      return 0;

   double num = 0;
   distance_type den = 0;
   for ( ; x < xn; ++x )
   {
      double y = (double( *x ) - center)/wb;
      double f = Max( -1.0, Min( 1.0, y ) );
      num += f*f;
      if ( Abs( y ) < 1 )
         ++den;
   }

   den *= den;
   return (1 + den != 1) ? n*wb*wb*num/den : 0.0;
}

// ----------------------------------------------------------------------------

/*!
 * \defgroup special_functions Special Functions
 */

/*!
 * Evaluation of the regularized incomplete beta function I_x( a, b ).
 *
 * \param a,b     The a and b parameters of the beta function being evaluated.
 *
 * \param x       Function evaluation point. Must be in the range [0,1].
 *
 * \param eps     Relative accuracy of the returned function evaluation. The
 *                default value is 1.0e-8.
 *
 * This implementation is adapted from original code by Lewis Van Winkle,
 * released under zlib license:
 *
 * <pre>
 * https://codeplea.com/incomplete-beta-function-c
 * https://github.com/codeplea/incbeta
 * </pre>
 *
 * Copyright (c) 2016, 2017 Lewis Van Winkle
 *
 * \ingroup special_functions
 */
inline double IncompleteBeta( double a, double b, double x, double eps = 1.0e-8 ) noexcept
{
   if ( x < 0 || x > 1 )
      return std::numeric_limits<double>::infinity();

   /*
    * The continued fraction converges nicely for x < (a+1)/(a+b+2)
    */
   if ( x > (a + 1)/(a + b + 2) )
      return 1 - IncompleteBeta( b, a, 1 - x ); // Use the fact that beta is symmetric

    /*
     * Find the first part before the continued fraction.
     */
    double lbeta_ab = lgamma( a ) + lgamma( b ) - lgamma( a + b );
    double front = Exp( Ln( x )*a + Ln( 1 - x )*b - lbeta_ab )/a;

    /*
     * Use Lentz's algorithm to evaluate the continued fraction.
     */
   const double tiny = 1.0e-30;
   double f = 1, c = 1, d = 0;
   for ( int i = 0; i <= 200; ++i )
   {
      int m = i >> 1;
      double numerator;
      if ( i & 1 )
         numerator = -((a + m)*(a + b + m)*x)/((a + 2*m)*(a + 2*m + 1)); // Odd term
      else if ( i > 0 )
         numerator = (m*(b - m)*x)/((a + 2*m - 1)*(a + 2*m)); // Even term
      else
         numerator = 1; // First numerator is 1.0

      /*
       * Do an iteration of Lentz's algorithm.
       */
      d = 1 + numerator*d;
      if ( Abs( d ) < tiny )
         d = tiny;
      d = 1/d;
      c = 1 + numerator/c;
      if ( Abs( c ) < tiny )
         c = tiny;
      double cd = c*d;
      f *= cd;
      if ( Abs( 1 - cd ) < eps )
         return front*(f - 1);
   }

   // Needed more loops, did not converge.
   return std::numeric_limits<double>::infinity();
}

// ----------------------------------------------------------------------------

/*!
 * \defgroup hash_functions Non-Cryptographic Hash Functions
 */

/*!
 * Computes a 64-bit non-cryptographic hash function.
 *
 * \param data    Address of the first byte of the input data block.
 *
 * \param size    Length in bytes of the input data block.
 *
 * \param seed    Optional seed value for initialization of the hash function.
 *                If \a seed is zero or is not specified, the seed will be set
 *                equal to the length of the data block.
 *
 * Returns a 64-bit hash value computed from the input data block.
 *
 * <pre>
 * Test vector: "The quick brown fox jumps over the lazy dog"
 * Hash64 checksum = 9a11f5e9468d7425
 *
 * Test vector: "" (empty string)\n
 * Hash64 checksum = ef46db3751d8e999
 * </pre>
 *
 * This function implements the xxHash algorithm by Yann Collet. Our code is an
 * adaptation of the original code by the author:
 *
 * <pre>
 * https://github.com/Cyan4973/xxHash
 * </pre>
 *
 * Copyright (C) 2012-2014, Yann Collet. \n
 * The original code has been released under the BSD 2-Clause License:
 *
 * <pre>
 * http://www.opensource.org/licenses/bsd-license.php
 * </pre>
 *
 * \ingroup hash_functions
 */
inline uint64 Hash64( const void* data, size_type size, uint64 seed = 0 ) noexcept
{
#define PRIME64_1 11400714785074694791ULL
#define PRIME64_2 14029467366897019727ULL
#define PRIME64_3  1609587929392839161ULL
#define PRIME64_4  9650029242287828579ULL
#define PRIME64_5  2870177450012600261ULL

   const uint8* p = (const uint8*)data;
   const uint8* end = p + size;
   uint64 h64;

   if ( seed == 0 )
      seed = size;

   if ( size >= 32 )
   {
      const uint8* limit = end - 32;
      uint64 v1 = seed + PRIME64_1 + PRIME64_2;
      uint64 v2 = seed + PRIME64_2;
      uint64 v3 = seed + 0;
      uint64 v4 = seed - PRIME64_1;

      do
      {
         v1 += *(uint64*)p * PRIME64_2;
         p += 8;
         v1 = RotL( v1, 31 );
         v1 *= PRIME64_1;
         v2 += *(uint64*)p * PRIME64_2;
         p += 8;
         v2 = RotL( v2, 31 );
         v2 *= PRIME64_1;
         v3 += *(uint64*)p * PRIME64_2;
         p += 8;
         v3 = RotL( v3, 31 );
         v3 *= PRIME64_1;
         v4 += *(uint64*)p * PRIME64_2;
         p += 8;
         v4 = RotL( v4, 31 );
         v4 *= PRIME64_1;
      }
      while ( p <= limit );

      h64 = RotL( v1, 1 ) + RotL( v2, 7 ) + RotL( v3, 12 ) + RotL( v4, 18 );

      v1 *= PRIME64_2;
      v1 = RotL( v1, 31 );
      v1 *= PRIME64_1;
      h64 ^= v1;
      h64 = h64 * PRIME64_1 + PRIME64_4;

      v2 *= PRIME64_2;
      v2 = RotL( v2, 31 );
      v2 *= PRIME64_1;
      h64 ^= v2;
      h64 = h64 * PRIME64_1 + PRIME64_4;

      v3 *= PRIME64_2;
      v3 = RotL( v3, 31 );
      v3 *= PRIME64_1;
      h64 ^= v3;
      h64 = h64 * PRIME64_1 + PRIME64_4;

      v4 *= PRIME64_2;
      v4 = RotL( v4, 31 );
      v4 *= PRIME64_1;
      h64 ^= v4;
      h64 = h64 * PRIME64_1 + PRIME64_4;
   }
   else
   {
      h64 = seed + PRIME64_5;
   }

   h64 += size;

   while ( p+8 <= end )
   {
      uint64 k1 = *(uint64*)p;
      k1 *= PRIME64_2;
      k1 = RotL( k1, 31 );
      k1 *= PRIME64_1;
      h64 ^= k1;
      h64 = RotL( h64, 27 ) * PRIME64_1 + PRIME64_4;
      p += 8;
   }

   if ( p+4 <= end )
   {
      h64 ^= (uint64)(*(uint32*)p) * PRIME64_1;
      h64 = RotL( h64, 23 ) * PRIME64_2 + PRIME64_3;
      p += 4;
   }

   while ( p < end )
   {
      h64 ^= *p * PRIME64_5;
      h64 = RotL( h64, 11 ) * PRIME64_1;
      ++p;
   }

   h64 ^= h64 >> 33;
   h64 *= PRIME64_2;
   h64 ^= h64 >> 29;
   h64 *= PRIME64_3;
   h64 ^= h64 >> 32;

   return h64;

#undef PRIME64_1
#undef PRIME64_2
#undef PRIME64_3
#undef PRIME64_4
#undef PRIME64_5
}

/*!
 * Computes a 32-bit non-cryptographic hash function.
 *
 * \param data    Address of the first byte of the input data block.
 *
 * \param size    Length in bytes of the input data block.
 *
 * \param seed    Optional seed value for initialization of the hash function.
 *                If \a seed is zero or is not specified, the seed will be set
 *                equal to the length of the data block.
 *
 * Returns a 32-bit hash value computed from the input data block.
 *
 * <pre>
 * Test vector: "The quick brown fox jumps over the lazy dog"\n
 * Hash32 checksum = 752cd1b8
 *
 * Test vector: "" (empty string)\n
 * Hash32 checksum = 2cc5d05
 * </pre>
 *
 * This function implements the xxHash algorithm by Yann Collet. Our code is an
 * adaptation of the original code by the author:
 *
 * <pre>
 * https://github.com/Cyan4973/xxHash
 * </pre>
 *
 * Copyright (C) 2012-2014, Yann Collet. \n
 * The original code has been released under the BSD 2-Clause License:
 *
 * <pre>
 * http://www.opensource.org/licenses/bsd-license.php
 * </pre>
 *
 * \ingroup hash_functions
 */
inline uint32 Hash32( const void* data, size_type size, uint32 seed = 0 ) noexcept
{
#define PRIME32_1 2654435761U
#define PRIME32_2 2246822519U
#define PRIME32_3 3266489917U
#define PRIME32_4  668265263U
#define PRIME32_5  374761393U

   const uint8* p = (const uint8*)data;
   const uint8* end = p + size;
   uint32 h32;

   if ( seed == 0 )
      seed = uint32( size );

   if ( size >= 16 )
   {
      const uint8* limit = end - 16;
      uint32 v1 = seed + PRIME32_1 + PRIME32_2;
      uint32 v2 = seed + PRIME32_2;
      uint32 v3 = seed + 0;
      uint32 v4 = seed - PRIME32_1;

      do
      {
         v1 += *(uint32*)p * PRIME32_2;
         v1 = RotL( v1, 13 );
         v1 *= PRIME32_1;
         p += 4;
         v2 += *(uint32*)p * PRIME32_2;
         v2 = RotL( v2, 13 );
         v2 *= PRIME32_1;
         p += 4;
         v3 += *(uint32*)p * PRIME32_2;
         v3 = RotL( v3, 13 );
         v3 *= PRIME32_1;
         p += 4;
         v4 += *(uint32*)p * PRIME32_2;
         v4 = RotL( v4, 13 );
         v4 *= PRIME32_1;
         p += 4;
      }
      while ( p <= limit );

      h32 = RotL( v1, 1 ) + RotL( v2, 7 ) + RotL( v3, 12 ) + RotL( v4, 18 );
   }
   else
   {
      h32  = seed + PRIME32_5;
   }

   h32 += (uint32)size;

   while ( p+4 <= end )
   {
      h32 += *(uint32*)p * PRIME32_3;
      h32  = RotL( h32, 17 ) * PRIME32_4 ;
      p+=4;
   }

   while ( p < end )
   {
      h32 += *p * PRIME32_5;
      h32 = RotL( h32, 11 ) * PRIME32_1 ;
      ++p;
   }

   h32 ^= h32 >> 15;
   h32 *= PRIME32_2;
   h32 ^= h32 >> 13;
   h32 *= PRIME32_3;
   h32 ^= h32 >> 16;

   return h32;

#undef PRIME32_1
#undef PRIME32_2
#undef PRIME32_3
#undef PRIME32_4
#undef PRIME32_5
}

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_Math_h

// ----------------------------------------------------------------------------
// EOF pcl/Math.h - Released 2020-12-17T15:46:29Z
