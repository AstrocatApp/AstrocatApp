//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/Random.h - Released 2020-12-17T15:46:28Z
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

#ifndef __PCL_Random_h
#define __PCL_Random_h

/// \file pcl/Random.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/AutoPointer.h>
#include <pcl/Vector.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \defgroup random_numbers Random Number Generation
 */

/*!
 * Returns a 64-bit random generator seed.
 *
 * On UNIX/Linux platforms, this function reads the /dev/urandom system device
 * to acquire a high-quality random seed. On Windows, the rand_s() CRT function
 * is invoked with the same purpose.
 *
 * In the extremely rare cases where a system random seed cannot be obtained,
 * the time() function is used to get a unique initialization value.
 *
 * Subsequent calls to this function are guaranteed to return unique values.
 *
 * This function is thread-safe. It can be safely called from multiple
 * execution threads running concurrently.
 *
 * \ingroup random_numbers
 */
extern uint64 RandomSeed64();

/*!
 * Returns a 32-bit random generator seed.
 *
 * This function simply calls RandomSeed64() and returns the XOR combination of
 * the 32-bit words in the 64-bit random seed.
 *
 * As RandomSeed64(), this function is thread-safe and is guaranteed to return
 * a unique value on each call.
 *
 * \ingroup random_numbers
 */
inline uint32 RandomSeed32()
{
   union { uint64 u64; uint32 u32[ 2 ]; } seed;
   seed.u64 = RandomSeed64();
   return seed.u32[0] ^ seed.u32[1];
}

// ----------------------------------------------------------------------------

class FastMersenneTwister;

// ----------------------------------------------------------------------------

/*!
 * \class RandomNumberGenerator
 * \brief Mersenne Twister (MT19937) pseudo-random number generator.
 *
 * \deprecated This class has been deprecated. Use the XoShiRo256ss and
 * XoRoShiRo1024ss classes for all newly produced code.
 *
 * Generation of pseudo-random numbers with user-selectable range and
 * probability distributions.
 *
 * This generator supports the uniform, normal (Gaussian) and Poisson
 * distributions. In addition, the upper range of generated uniform deviates
 * can be arbitrarily defined.
 *
 * %RandomNumberGenerator is a functional class. The function call operator()()
 * returns pseudo-random numbers in the range [0,ymax], where ymax is the
 * user-defined arbitrary upper range.
 *
 * Example of use:
 *
 * \code
 * RandomNumberGenerator R, R1( 10 ); // R's ymax = 1, R1's ymax = 10
 * // ...
 * double y = R();   // y = random uniform deviate in the range [0,1]
 * double z = R1();  // z = random uniform deviate in the range [0,10]
 * \endcode
 *
 * <b>References</b>
 *
 * Based on an adaptation of SIMD-oriented Fast Mersenne Twister (SFMT) by
 * Mutsuo Saito and Makoto Matsumoto (Hiroshima University).
 *
 * Currently PCL implements a SFMT generator with a period of 2^19937-1.
 *
 * SFMT Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * \ingroup random_numbers
 */
class PCL_CLASS RandomNumberGenerator
{
public:

   /*!
    * Constructs a %RandomNumberGenerator object.
    *
    * \param ymax    Upper bound of uniform deviates. The function call
    *                operator double operator()() and the Uniform() member
    *                function (which are synonyms) will return uniform
    *                pseudo-random deviates in the range [0,ymax]. The default
    *                value is 1.0.
    *
    * \param seed    32-bit initialization seed. If this parameter is zero, a
    *                unique random seed will be generated automatically. The
    *                default value is zero.
    */
   RandomNumberGenerator( double ymax = 1.0, uint32 seed = 0 );

   /*!
    * Destroys a %RandomNumberGenerator object.
    */
   virtual ~RandomNumberGenerator();

   /*!
    * Generates a floating point uniform deviate in the range [0,UpperBound()]
    */
   double operator ()()
   {
      return m_rmax*Rand32();
   }

   /*!
    * Generates a 32-bit unsigned integer uniform deviate.
    */
   uint32 Rand32();

   /*!
    * Generates a floating point uniform deviate in the range [0,1] (i.e.,
    * ignoring UpperBound()).
    */
   double Rand1()
   {
      return double( Rand32() )/uint32_max;
   }

   /*!
    * Generates a floating point uniform deviate in the range [0,UpperBound()]
    *
    * This is a convenience alias for operator()().
    */
   double Uniform()
   {
      return operator()();
   }

   /*!
    * Generates a floating point normal deviate with the specified \a mean and
    * standard deviation \a sigma.
    */
   double Normal( double mean = 0, double sigma = 1 );

   /*!
    * Generates a floating point normal deviate with the specified \a mean and
    * standard deviation \a sigma.
    *
    * This is a convenience alias for Normal( mean, sigma ).
    */
   double Gaussian( double mean = 0, double sigma = 1 )
   {
      return Normal( mean, sigma );
   }

   /*!
    * Generates a discrete random deviate from a Poisson distribution with the
    * specified expected value \a lambda.
    */
   int Poisson( double lambda );

   /*!
    * Returns the current upper bound of this random number generator.
    */
   double UpperBound() const
   {
      return m_ymax;
   }

   /*!
    * Sets the upper bound \a ymax > 0 for this random number generator.
    */
   void SetUpperBound( double ymax )
   {
      PCL_PRECONDITION( ymax > 0 )
      PCL_PRECONDITION( 1 + ymax != 1 )
      m_rmax = (m_ymax = ymax)/double( uint32_max );
      m_normal = false;
   }

private:

   AutoPointer<FastMersenneTwister> m_generator;
   double                           m_ymax;
   double                           m_rmax;
   bool                             m_normal;
   double                           m_vs;     // second result from Box–Muller transform
   DVector                          m_lambda; // precalculated for current Poisson lambda
};

// ----------------------------------------------------------------------------

/*!
 * \class XorShift1024
 * \brief Implementation of the XorShift1024* pseudo-random number generator.
 *
 * \deprecated This class has been deprecated. Use the XoShiRo256ss and
 * XoRoShiRo1024ss classes for all newly produced code.
 *
 * Generation of pseudo-random uniform deviates using the xorshift1024*
 * generator developed in 2014 by Sebastiano Vigna. This is a fast, top-quality
 * generator with a period of 2^1024-1, passing strong statistical test suites.
 *
 * Examples of use:
 *
 * \code
 * XorShift1024 X; // initialized automatically
 * // ...
 * double x = X();      // x = random uniform deviate in the range [0,1)
 * uint64 y = X.UI64(); // y = 64-bit unsigned integer random uniform deviate
 * uint32 z = X.UI32(); // z = 32-bit unsigned integer random uniform deviate
 * uint32 t = X.UIN( 100 ); // t = integer uniform deviate in the range [0,99]
 * \endcode
 *
 * <b>References</b>
 *
 * Sebastiano Vigna (2014), <em>An experimental exploration of Marsaglia's
 * xorshift generators, scrambled</em>, arXiv:1402.6246
 *
 * Sebastiano Vigna (2014), <em>Further scramblings of Marsaglia's xorshift
 * generators</em>, arXiv:1404.0390
 *
 * See also: http://xorshift.di.unimi.it/
 *
 * \ingroup random_numbers
 */
class PCL_CLASS XorShift1024
{
public:

   /*!
    * Constructs a %XorShift1024 pseudo-random number generator.
    *
    * \param seed    64-bit initialization seed. If this parameter is zero, a
    *                unique random seed will be generated automatically. The
    *                default value is zero.
    */
   XorShift1024( uint64 seed = 0 ) noexcept( false )
   {
      Initialize( seed );
   }

   /*!
    * Returns a double precision uniform random deviate in the [0,1) range.
    */
   double operator ()() noexcept
   {
      return 5.4210108624275221703311e-20 * UI64(); // 1.0/(2^64 -1)
   }

   /*!
    * Returns a 64-bit unsigned integer uniform random deviate.
    */
   uint64 UI64() noexcept
   {
      uint64 s0 = m_s[m_p];
      uint64 s1 = m_s[m_p = (m_p + 1) & 15];
      s1 ^= s1 << 31; // a
      s1 ^= s1 >> 11; // b
      s0 ^= s0 >> 30; // c
      return (m_s[m_p] = s0 ^ s1) * 1181783497276652981ull;
   }

   /*!
    * Returns a 32-bit unsigned integer uniform random deviate.
    */
   uint32 UI32() noexcept
   {
      return uint32( UI64() );
   }

   /*!
    * Returns a 64-bit unsigned integer uniform random deviate in the range
    * [0,n-1].
    */
   uint64 UI64N( uint64 n ) noexcept
   {
      return UI64() % n;
   }

   /*!
    * Returns an unsigned integer uniform random deviate in the range [0,n-1].
    */
   uint32 UIN( uint32 n ) noexcept
   {
      return UI64() % n;
   }

   /*!
    * A synonym for UIN().
    */
   uint32 UI32N( uint32 n ) noexcept
   {
      return UIN( n );
   }

   /*!
    * Reinitializes this generator with a new \a seed.
    *
    * If the specified \a seed is zero, a unique, high-quality random seed will
    * be generated automatically by calling RandomSeed64().
    */
   void Initialize( uint64 x )
   {
      if ( x == 0 )
         x = RandomSeed64();
      // Use a xorshift64* generator to initialize the state space.
      for ( int i = 0; i < 16; ++i )
      {
         x ^= x >> 12; // a
         x ^= x << 25; // b
         x ^= x >> 27; // c
         m_s[i] = x * 2685821657736338717ull;
      }
      m_p = 0;
   }

private:

   uint64 m_s[ 16 ]; // state space
   int m_p;          // current index
};

// ----------------------------------------------------------------------------

/*!
 * \class XoRoShiRo1024ss
 * \brief Base class of xoshiro and xoroshiro pseudo-random number generators.
 */
class PCL_CLASS XoShiRoBase
{
public:

   /*!
    * Default constructor.
    */
   XoShiRoBase() = default;

protected:

   /*!
    * \internal
    * The left rotation function used by the generator.
    */
   static uint64 RotL( const uint64 x, int k ) noexcept
   {
      return (x << k) | (x >> (64 - k));
   }

   /*!
    * \internal
    * The SplitMix64 generator used for state space initialization, as
    * recommended by Blackman/Vigna.
    */
   static uint64 SplitMix64( uint64& x ) noexcept
   {
      uint64 z = (x += 0x9e3779b97f4a7c15);
      z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
      z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
      return z ^ (z >> 31);
   }

   /*!
    * \internal
    * Conversion of a 64-bit unsigned inteter to 64-bit floating point with
    * uniform probability over the entire 53-bit significant digits.
    * See: http://prng.di.unimi.it/#remarks
    */
   static double UI64ToDouble( uint64 x ) noexcept
   {
      return (x >> 11) * 0x1.0p-53;
   }
};

// ----------------------------------------------------------------------------

/*!
 * \class XoShiRo256ss
 * \brief Implementation of the xoshiro256** pseudo-random number generator.
 *
 * Generation of pseudo-random uniform deviates using the xoroshiro1024**
 * generator developed in 2019 by David Blackman and Sebastiano Vigna. This is
 * a fast, top-quality generator with a period of 2^256-1, passing strong
 * statistical test suites&mdash;actually, it passes all tests we are aware of.
 *
 * Examples of use:
 *
 * \code
 * XoShiRo256ss X; // initialized automatically
 * // ...
 * double x = X();      // x = random uniform deviate in the range [0,1)
 * uint64 y = X.UI64(); // y = 64-bit unsigned integer random uniform deviate
 * uint32 z = X.UI32(); // z = 32-bit unsigned integer random uniform deviate
 * uint32 t = X.UIN( 100 ); // t = integer uniform deviate in the range [0,99]
 * \endcode
 *
 * <b>References</b>
 *
 * David Blackman and Sebastiano Vigna (2019), <em>Scrambled linear
 * pseudorandom number generators</em> (preprint).
 *
 * See also: http://prng.di.unimi.it/
 *
 * \ingroup random_numbers
 */
class PCL_CLASS XoShiRo256ss : public XoShiRoBase
{
public:

   /*!
    * Constructs a %XoShiRo256ss pseudo-random number generator.
    *
    * \param seed    64-bit initialization seed. If this parameter is zero, a
    *                unique random seed will be generated automatically. The
    *                default value is zero.
    */
   XoShiRo256ss( uint64 seed = 0 ) noexcept( false )
   {
      Initialize( seed );
   }

   /*!
    * Returns a double precision uniform random deviate in the [0,1) range.
    */
   double operator ()() noexcept
   {
      return UI64ToDouble( UI64() );
   }

   /*!
    * Returns a 64-bit unsigned integer uniform random deviate.
    */
   uint64 UI64() noexcept
   {
      const uint64 result = RotL( m_s[1]*5, 7 ) * 9;
      const uint64 t = m_s[1] << 17;
      m_s[2] ^= m_s[0];
      m_s[3] ^= m_s[1];
      m_s[1] ^= m_s[2];
      m_s[0] ^= m_s[3];
      m_s[2] ^= t;
      m_s[3] = RotL( m_s[3], 45 );
      return result;
   }

   /*!
    * Returns a 32-bit unsigned integer uniform random deviate.
    */
   uint32 UI32() noexcept
   {
      return uint32( UI64() );
   }

   /*!
    * Returns a 64-bit unsigned integer uniform random deviate in the range
    * [0,n-1].
    */
   uint64 UI64N( uint64 n ) noexcept
   {
      return UI64() % n;
   }

   /*!
    * Returns an unsigned integer uniform random deviate in the range [0,n-1].
    */
   uint32 UIN( uint32 n ) noexcept
   {
      return UI64() % n;
   }

   /*!
    * A synonym for UIN().
    */
   uint32 UI32N( uint32 n ) noexcept
   {
      return UIN( n );
   }

   /*!
    * Reinitializes this generator with a new \a seed.
    *
    * If the specified \a seed is zero, a unique, high-quality random seed will
    * be generated automatically by calling RandomSeed64().
    */
   void Initialize( uint64 x )
   {
      if ( x == 0 )
         x = RandomSeed64();
      // Use a SplitMix64 generator to initialize the state space.
      for ( int i = 0; i < 4; ++i )
         m_s[i] = SplitMix64( x );
   }

private:

   uint64 m_s[ 4 ];
};

// ----------------------------------------------------------------------------

/*!
 * \class XoRoShiRo1024ss
 * \brief Implementation of the xoroshiro1024** pseudo-random number generator.
 *
 * Generation of pseudo-random uniform deviates using the xoroshiro1024**
 * generator developed in 2019 by David Blackman and Sebastiano Vigna. This is
 * a fast, top-quality generator with a period of 2^1024-1, passing strong
 * statistical test suites&mdash;actually, it passes all tests we are aware of.
 *
 * Examples of use:
 *
 * \code
 * XoRoShiRo1024ss X; // initialized automatically
 * // ...
 * double x = X();      // x = random uniform deviate in the range [0,1)
 * uint64 y = X.UI64(); // y = 64-bit unsigned integer random uniform deviate
 * uint32 z = X.UI32(); // z = 32-bit unsigned integer random uniform deviate
 * uint32 t = X.UIN( 100 ); // t = integer uniform deviate in the range [0,99]
 * \endcode
 *
 * <b>References</b>
 *
 * David Blackman and Sebastiano Vigna (2019), <em>Scrambled linear
 * pseudorandom number generators</em> (preprint).
 *
 * See also: http://prng.di.unimi.it/
 *
 * \ingroup random_numbers
 */
class PCL_CLASS XoRoShiRo1024ss : public XoShiRoBase
{
public:

   /*!
    * Constructs a %XoRoShiRo1024ss pseudo-random number generator.
    *
    * \param seed    64-bit initialization seed. If this parameter is zero, a
    *                unique random seed will be generated automatically. The
    *                default value is zero.
    */
   XoRoShiRo1024ss( uint64 seed = 0 ) noexcept( false )
   {
      Initialize( seed );
   }

   /*!
    * Returns a double precision uniform random deviate in the [0,1) range.
    */
   double operator ()() noexcept
   {
      return UI64ToDouble( UI64() );
   }

   /*!
    * Returns a 64-bit unsigned integer uniform random deviate.
    */
   uint64 UI64() noexcept
   {
      const int q = m_p;
      const uint64 s0 = m_s[m_p = (m_p + 1) & 15];
      uint64 s15 = m_s[q];
      const uint64 result = RotL( s0*5, 7 ) * 9;
      s15 ^= s0;
      m_s[q] = RotL( s0, 25 ) ^ s15 ^ (s15 << 27);
      m_s[m_p] = RotL( s15, 36 );
      return result;
   }

   /*!
    * Returns a 32-bit unsigned integer uniform random deviate.
    */
   uint32 UI32() noexcept
   {
      return uint32( UI64() );
   }

   /*!
    * Returns a 64-bit unsigned integer uniform random deviate in the range
    * [0,n-1].
    */
   uint64 UI64N( uint64 n ) noexcept
   {
      return UI64() % n;
   }

   /*!
    * Returns an unsigned integer uniform random deviate in the range [0,n-1].
    */
   uint32 UIN( uint32 n ) noexcept
   {
      return UI64() % n;
   }

   /*!
    * A synonym for UIN().
    */
   uint32 UI32N( uint32 n ) noexcept
   {
      return UIN( n );
   }

   /*!
    * Reinitializes this generator with a new \a seed.
    *
    * If the specified \a seed is zero, a unique, high-quality random seed will
    * be generated automatically by calling RandomSeed64().
    */
   void Initialize( uint64 x )
   {
      if ( x == 0 )
         x = RandomSeed64();
      // Use a SplitMix64 generator to initialize the state space.
      for ( int i = 0; i < 16; ++i )
         m_s[i] = SplitMix64( x );
      m_p = 0;
   }

private:

   uint64 m_s[ 16 ];
   int    m_p;
};

// ----------------------------------------------------------------------------

/*!
 * \class NormalRandomDeviates
 * \brief Generation of random normal (Gaussian) deviates.
 * \ingroup random_numbers
 */
template <class RNG>
class PCL_CLASS NormalRandomDeviates
{
public:

   /*!
    * Constructs a %NormalRandomDeviates objects using the specified
    * pseudo-random number generator \a R.
    */
   NormalRandomDeviates( RNG& R ) noexcept( false )
      : m_R( R )
   {
   }

   /*!
    * Returns a random deviate from a Gaussian distribution with zero mean and
    * unit standard deviation.
    */
   double operator ()() noexcept
   {
      /*
       * Marsaglia polar method.
       */
      double x;
      if ( m_first )
      {
         do
         {
            double u1 = m_R();
            double u2 = m_R();
            m_v1 = 2*u1 - 1;
            m_v2 = 2*u2 - 1;
            m_s = m_v1*m_v1 + m_v2*m_v2;
         }
         while ( m_s >= 1 || m_s <= std::numeric_limits<double>::epsilon() );

         x = m_v1 * Sqrt( -2*Ln( m_s )/m_s );
      }
      else
         x = m_v2 * Sqrt( -2*Ln( m_s )/m_s );

      m_first = !m_first;
      return x;
   }

private:

   RNG&   m_R;
   double m_v1 = 0;
   double m_v2 = 0;
   double m_s = 0;
   bool   m_first = true;
};

// ----------------------------------------------------------------------------

/*!
 * \class PoissonRandomDeviates
 * \brief Generation of random Poisson deviates.
 * \ingroup random_numbers
 */
template <class RNG>
class PCL_CLASS PoissonRandomDeviates
{
public:

   /*!
    * Constructs a %PoissonRandomDeviates objects using the specified
    * pseudo-random number generator \a R.
    */
   PoissonRandomDeviates( RNG& R ) noexcept( false )
      : m_R( R )
   {
   }

   /*!
    * Returns a random Poisson deviate for the specified \a value.
    */
   int operator ()( double value ) noexcept
   {
      if ( value < 30 )
      {
         /*
          * Implementation of the algorithm by Donald E. Knuth, 1969.
          *
          * This algorithm is slow (unusable) for large values.
          */
         double p = 1, L = Exp( -value );
         int k = 0;
         do
         {
            ++k;
            p *= m_R();
         }
         while ( p > L );
         return k-1;
      }

      /*
       * Code adapted from 'Random number generation in C++', by John D. Cook:
       *
       * https://www.johndcook.com/blog/cpp_random_number_generation/
       *
       * The algorithm is from "The Computer Generation of Poisson Random
       * Variables" by A. C. Atkinson, Journal of the Royal Statistical
       * Society Series C (Applied Statistics) Vol. 28, No. 1. (1979)
       *
       * This algorithm is slow (unusable) for small values.
       */
      double c = 0.767 - 3.36/value;
      double beta = Const<double>::pi()/Sqrt( 3*value );
      double alpha = beta*value;
      double k = Ln( c ) - value - Ln( beta );
      for ( ;; )
      {
         double u = m_R();
         double x = (alpha - Ln( (1 - u)/u ))/beta;
         int n = int( Floor( x + 0.5 ) );
         if ( n < 0 )
            continue;
         double v = m_R();
         double y = alpha - beta*x;
         double temp = 1 + Exp( y );
         double lhs = y + Ln( v/temp/temp );
         double rhs = k + n*Ln( value ) - LnFactorial( n );
         if ( lhs <= rhs )
            return n;
      }
   }

private:

   RNG& m_R;
};

// ----------------------------------------------------------------------------

/*!
 * \class GammaRandomDeviates
 * \brief Generation of random gamma deviates.
 * \ingroup random_numbers
 */
template <class RNG>
class PCL_CLASS GammaRandomDeviates
{
public:

   /*!
    * Constructs a %GammaRandomDeviates objects using the specified
    * pseudo-random number generator \a R.
    */
   GammaRandomDeviates( RNG& R, double shape = 1, double scale = 1 ) noexcept( false )
      : m_R( R )
      , m_shape( shape )
      , m_scale( scale )
      , m_normal( R )
   {
      if ( m_shape <= 0 )
         throw Error( "GammaRandomDeviates(): The function shape parameter must be > 0." );
      if ( m_scale <= 0 )
         throw Error( "GammaRandomDeviates(): The scale parameter must be > 0." );

      m_d = ((m_shape >= 1) ? m_shape : m_shape + 1) - 1.0/3.0;
      m_c = 1/Sqrt( 9*m_d );
   }

   /*!
    * Returns a random deviate from a Gaussian distribution with zero mean and
    * unit standard deviation.
    */
   double operator ()() noexcept
   {
      /*
       * Code adapted from 'Random number generation in C++', by John D. Cook:
       *    https://www.johndcook.com/blog/cpp_random_number_generation/
       *
       * Implementation based on "A Simple Method for Generating Gamma
       * Variables" by George Marsaglia and Wai Wan Tsang. ACM Transactions on
       * Mathematical Software Vol 26, No 3, September 2000, pages 363-372.
       */
      for ( ;; )
      {
         double x, v;
         do
         {
            x = m_normal();
            v = 1 + m_c*x;
         }
         while ( v <= 0 );
         v = v*v*v;
         double u = m_R();
         double xsquared = x*x;
         if ( u < 1 - 0.0331*xsquared*xsquared || Ln( u ) < 0.5*xsquared + m_d*(1 - v + Ln( v )) )
         {
            double g = m_scale*m_d*v;
            if ( m_shape < 1 )
               g *= Pow( m_R(), 1/m_shape );
            return g;
         }
      }
   }

private:

   RNG&                      m_R;
   double                    m_shape;
   double                    m_scale;
   double                    m_d;
   double                    m_c;
   NormalRandomDeviates<RNG> m_normal;
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_Random_h

// ----------------------------------------------------------------------------
// EOF pcl/Random.h - Released 2020-12-17T15:46:28Z
