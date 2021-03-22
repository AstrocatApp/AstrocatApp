//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/SurfaceSpline.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/SurfaceSpline.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*
 * Add the elements of vector q, each multiplied by k, to the vector z.
 */
template <typename T> inline static
void AddMulVector( int n, T k, const T* __restrict__ q, T* __restrict__ z ) noexcept
{
   PCL_UNROLL( 8 )
   for ( ; n > 0; --n )
      *z++ += k * *q++;
}

// ----------------------------------------------------------------------------

/*
 * Find the index of the largest vector element in absolute value.
 * The return value is in [1,2,...,n], or 0 if n < 1.
 */
template <typename T> inline static
int IndexOfMaxAbsVectorElement( const T* __restrict__ v, int n ) noexcept
{
   if ( n < 1 )
      return 0;

   T maxval = pcl::Abs( *v++ );
   int maxind = 1;
   for ( int i = 2; i <= n; ++i )
   {
      T m = pcl::Abs( *v++ );
      if ( m > maxval )
         maxval = m, maxind = i;
   }
   return maxind;
}

// ----------------------------------------------------------------------------

/*
 * Exchange the elements of two vectors v and w.
 */
template <typename T> inline static
void SwapVectorElements( int n, T* __restrict__ v, T* __restrict__ w ) noexcept
{
   PCL_UNROLL( 8 )
   for ( ; n > 0; --n )
      pcl::Swap( *v++, *w++ );
}

// ----------------------------------------------------------------------------

/*
 * The scalar product v*w = v[0] * w[0] + ... + v[n-1] * w[n-1].
 */
template <typename T> inline static
T DotProduct( const T* __restrict__ v, const T* __restrict__ w, int n ) noexcept
{
   T vdotw = T( 0 );
   PCL_UNROLL( 8 )
   for ( ; n > 0; --n )
      vdotw += *v++ * *w++;
   return vdotw;
}

// ----------------------------------------------------------------------------

/*
 * Computes the factorization of a real symmetric matrix A stored in packed
 * format using the Bunch-Kaufman diagonal pivoting method:
 *
 *    A = U*D*U^T
 *
 * where U is a product of permutation and unit upper triangular matrices, and
 * D is symmetric and block diagonal with 1-by-1 and 2-by-2 diagonal blocks.
 *
 * n        The order of the matrix A.
 *
 * ap       (input/output) Array of (n*(n + 1)/2) elements.
 *          On entry, the upper triangle of the symmetric matrix A, packed
 *          columnwise in a linear array. The j-th column of A is stored in the
 *          array ap as follows:
 *
 *             ap[i + (j - 1)*j/2] = A[i,j] for 0 <= i <= j
 *
 *          On exit, the block diagonal matrix D and the multipliers used to
 *          obtain the factor U, stored as a packed triangular matrix
 *          overwriting A.
 *
 * ipiv     (output) Integer array of n elements. Details of the interchanges
 *          and the block structure of D.
 *          If ipiv[k] > 0, then rows and columns k and ipiv[k] were
 *          interchanged and D(k,k) is a 1-by-1 diagonal block.
 *          If ipiv[k] = ipiv[k-1] < 0, then rows and columns k-1 and -ipiv[k]
 *          were interchanged and D(k-1:k,k-1:k) is a 2-by-2 diagonal block.
 *
 * info     (output) Integer flag/index
 *          = 0: successful exit.
 *          > 0: if info = i, D(i,i) is exactly zero. The factorization has
 *               been completed, but the block diagonal matrix D is exactly
 *               singular, and division by zero will occur if it is used to
 *               solve a system of equations.
 */
template <typename T> static
PCL_HOT_FUNCTION int Factorize( T* __restrict__ ap, int n, int* __restrict__ pvt )
{
   // Constant used to choose a pivot block size.
   const T ALPHA = (1 + Sqrt( 17.0 ))/8;

   --ap; --pvt;

   // The return value: > 0 is the index of a singular pivot block.
   int info = 0;

   for ( int k = n, ik = (n*(n - 1))/2, im = 0, kstep; k != 0; k -= kstep )
   {
      if ( k <= 1 )
      {
         pvt[1] = 1;
         if ( ap[1] == 0 )
            info = 1;
         break;
      }

      /*
       * The following statements check which elimination to use. Afterwards
       * kstep contains the size of the pivot block, and swap indicates whether
       * swaps are being used.
       */
      int kk = ik + k;
      T absakk = pcl::Abs( ap[kk] );

      // Find largest non diagonal element in column k
      int imax = IndexOfMaxAbsVectorElement( ap+ik+1, k-1 );

      // Largest nonzero, non-diagonal element in column k.
      T colmax = pcl::Abs( ap[ik + imax] );

      bool swap;
      if ( absakk >= ALPHA*colmax )
         kstep = 1, swap = false;
      else
      {
         // Find the largest nonzero, non-diagonal element in row imax.
         T rowmax = 0;
         im = (imax*(imax - 1))/2;
         for ( int j = imax+1, imj = im + 2*imax; j <= k; imj += j, ++j )
         {
            T m = pcl::Abs( ap[imj] );
            if ( m > rowmax )
               rowmax = m;
         }
         if ( imax != 1 )
         {
            T m = pcl::Abs( ap[IndexOfMaxAbsVectorElement( ap+im+1, imax-1 ) + im] );
            if ( m > rowmax )
               rowmax = m;
         }

         if ( pcl::Abs( ap[imax + im] ) >= ALPHA*rowmax )
            kstep = 1, swap = true;
         else if ( absakk >= ALPHA*colmax*(colmax/rowmax) )
            kstep = 1, swap = false;
         else
            kstep = 2, swap = (imax != k-1);
      }

      if ( absakk == 0 && colmax == 0 )
      {
         if ( info == 0 )
            info = k; // report a singular pivot block
         pvt[k] = k;  // start next loop
      }
      else if ( kstep == 1 )                 // 1x1 pivot block?
      {
         if ( swap )
         {
            SwapVectorElements( imax, ap+im+1, ap+ik+1 );
            for ( int jj = imax, imj = ik + imax; jj <= k; ++jj )
            {
               int j = k + imax - jj;
               pcl::Swap( ap[ik+j], ap[imj] );
               imj -= j-1;
            }
         }

         for ( int jj = 1, ij = ik - (k - 1); jj < k; ++jj )
         {
            int j = k - jj;
            T amulk = -ap[ik+j] / ap[kk];
            AddMulVector( j, amulk, ap+ik+1, ap+ij+1 );
            ap[ik+j] = amulk;
            ij -= j-1;
         }

         pvt[k] = swap ? imax : k;           // set pivot index
      }
      else                                   // 2x2 pivot block?
      {
         int km1k = ik +  k-1;  // index of  A[k-1][k] in ap
         int ikm1 = ik - (k-1); // leading index (-1) of the (k-1)st column of A

         if ( swap )
         {
            SwapVectorElements( imax, ap+im+1, ap+ikm1+1 );

            for ( int jj = imax, imj = ikm1 + imax; jj < k; ++jj )
            {
               int j = k - 1 + imax - jj;
               pcl::Swap( ap[ikm1+j], ap[imj] );
               imj -= j-1;
            }

            pcl::Swap( ap[km1k], ap[ik+imax] );
         }

         if ( k != 2 )                       // elimination
         {
            T ak    = ap[kk]/ap[km1k];
            T akm1  = ap[ik]/ap[km1k];
            T denom = 1 - ak*akm1;
            for ( int jj = 1, ij = ik - (k - 1) - (k - 2); jj < k-1; ++jj )
            {
               int j       = k - 1 - jj;
               int jk      = ik + j;
               int jkm1    = ikm1 + j;
               T   bk      = ap[jk]/ap[km1k];
               T   bkm1    = ap[jkm1]/ap[km1k];
               T   amulk   = (akm1*bk - bkm1)/denom;
               T   amulkm1 = (ak*bkm1 - bk)/denom;

               AddMulVector( j, amulk, ap+ik+1, ap+ij+1 );
               AddMulVector( j, amulkm1, ap+ikm1+1, ap+ij+1 );

               ap[jk] = amulk;
               ap[jkm1] = amulkm1;

               ij -= j-1;
            }
         }

         pvt[k] = swap ? (-imax) : (1 - k);  // set pivot indices
         pvt[k-1] = pvt[k];
      }

      ik -= k-1;
      if ( kstep == 2 )
         ik -= k-2;
   }

   return info;
}

// ----------------------------------------------------------------------------

/*
 * Solves a system of linear equations A*x = b with a real symmetric matrix A
 * stored in packed format using the factorization
 *
 *    A = U*D*U^T
 *
 * ap       Array with (n*(n + 1)/2) elements.
 *          The block diagonal matrix D and the multipliers used to obtain the
 *          factor U as computed by Factorize(), stored as a packed triangular
 *          matrix.
 *
 * n        Size of matrix A.
 *
 * pvt      Integer array with n elements.
 *          Details of the interchanges and the block structure of D as
 *          determined by Factorize().
 *
 * b        (input/output) Array with (ldb*nrhs) elements.
 *          On entry, the right hand side vector b.
 *          On exit, the solution vector x.
 */
template <typename T> static
PCL_HOT_FUNCTION void Solve( const T* __restrict__ ap, int n, const int* __restrict__ pvt, T* __restrict__ b )
{
   --ap; --pvt; --b;

   for ( int ik = (n*(n - 1))/2, k = n; k != 0; )
   {
      // index of diagonal element A[k][k] in ap
      int kk = ik + k;

      if ( pvt[k] >= 0 )                     // 1x1 pivot block?
      {
         if ( k != 1 )
         {
            int pvtind = pvt[k];
            if ( pvtind != k )
               Swap( b[k], b[pvtind] );
            AddMulVector( k-1, b[k], ap+ik+1, b+1 );
         }

         b[k] /= ap[kk];
         --k;
         ik -= k;
      }
      else                                   // 2x2 pivot block?
      {
         if ( k != 2 )
         {
            int pvtind = Abs( pvt[k] );
            if ( pvtind != k-1 )
               Swap( b[k-1], b[pvtind] );

            AddMulVector( k-2, b[k],   ap+ik+1,     b+1 );
            AddMulVector( k-2, b[k-1], ap+ik+1-k+1, b+1 );
         }

         T akm1k  = ap[ik + k - 1];
         T ak     = ap[kk]/akm1k;
         T akm1   = ap[ik]/akm1k;
         T bk     = b[k]/akm1k;
         T bkm1   = b[k-1]/akm1k;
         T denom  = ak*akm1 - 1;
         b[k]   = (akm1*bk - bkm1)/denom;
         b[k-1] = (ak*bkm1 - bk)/denom;
         k -= 2;
         ik -= k+k + 1;
      }
   }

   for ( int ik = 0, k = 1; k <= n; )
      if ( pvt[k] >= 0 )                     // 1x1 pivot block?
      {
         if ( k != 1 )
         {
            b[k] += DotProduct( ap+ik+1, b+1, k-1 );
            int pvtind = pvt[k];
            if ( pvtind != k )
               Swap( b[k], b[pvtind] );
         }

         ik += k;
         ++k;
      }
      else                                   // 2x2 pivot block?
      {
         if ( k != 1 )
         {
            b[k]   += DotProduct( ap+ik+1,   b+1, k-1 );
            b[k+1] += DotProduct( ap+ik+k+1, b+1, k-1 );
            int pvtind = Abs( pvt[k] );
            if ( pvtind != k )
               Swap( b[k], b[pvtind] );
         }

         ik += k+k + 1;
         k += 2;
      }
}

// ----------------------------------------------------------------------------

/*
 * Compute the kernel part G of the system matrix, and write it to the working
 * matrix in packed form.
 *
 * Input
 * =====
 * n     Number of nodes.
 * m     Derivative order.
 * x,y   Vectors of node coordinates.
 *
 * r     Regularization parameter:
 *       For r <= 0  we compute an interpolating spline.
 *       for r > 0   we compute an approximating spline. The larger r, the
 *                   closer the spline gets to the fitting plane.
 *
 * w     For r > 0 only: Optional vector of positive node weights. Nodes with
 *       larger weights are given more prominence in the approximation. For
 *       r <= 0, or if nullptr is specified, this parameter is ignored.
 *
 * Output
 * ======
 * a     Vector with packed matrix that contains the kernel and polynomial
 *       parts, G and P respectively, of the system matrix. The kernel part G
 *       begins at index 0.
 */
template <typename T> static
PCL_HOT_FUNCTION void KernelPart( int n, int m, const T* __restrict__ x, const T* __restrict__ y,
                                  float r, const float* __restrict__ w, T* __restrict__ a )
{
   --x; --y; --a;
   if ( r > 0 )
      if ( w != nullptr )
         --w;

   for ( int l = 1, i = 1; i <= n; ++i, ++l )
   {
      for ( int k = 1; k < i; ++k, ++l )
      {
         double dx = x[k] - x[i];
         double dy = y[k] - y[i];
         double r2 = dx*dx + dy*dy;
         if ( likely( r2 != 0 ) )
         {
            double E = pcl::Ln( r2 );
            for ( int j = m; --j > 0; )
               E *= r2;
            a[l] = E;
         }
         else
            a[l] = 0;
      }

      // Main diagonal.
      a[l] = (r > 0) ? ((w == nullptr) ? r : r/w[i]) : 0;
   }
}

// ----------------------------------------------------------------------------

/*
 * Build auxiliary arrays for generation of all two-dimensional monomials up to
 * degree m. The number of existing such monomials is:
 *
 *    n = (m + 1)*(m + 2)/2
 *
 * For example, for m = 1, n = 3:
 *
 *    1, x, y
 *
 * For m = 2, n = 6:
 *
 *    1, x, y, xy, x^2, y^2
 *
 * Input
 * =====
 * i           Index of the last computed monomial.
 * idx[1...i]  Vector of powers of x in the monomials indexed from 1 to i.
 * idy[1...i]  Vector of powers of y in the monomials indexed from 1 to i.
 *
 * Output
 * ======
 * idx[i+1]    The power of x for the next monomial.
 * idy[i+1]    The power of y for the next monomial.
 * ixy         Index of the monomials that must be multiplied by either x or y
 *             (as indicated by the return value of this function) to obtain
 *             the next monomial at index i+1.
 *
 * Returns:
 * 1 if the next monomial must be multiplied by x: Monom[i+1] = Monom[ixy]*x
 * 0 if the next monomial must be multiplied by y: Monom[i+1] = Monom[ixy]*y
 */
inline static
int NextXYMonomial( int i, int* __restrict__ idx, int* __restrict__ idy, int& ixy )
{
   int n = idx[i] + idy[i];
   if ( idx[i] == 0 )
   {
      idx[i+1] = n + 1;
      idy[i+1] = 0;
      for ( int j = 1; j <= i; ++j )
         if ( idx[j] == n )
         {
            ixy = j;
            break;
         }
      return 1;
   }
   idx[i+1] = idx[i] - 1;
   idy[i+1] = idy[i] + 1;
   for ( int j = 1; j <= i; ++j )
      if ( idx[j] == idx[i] - 1 )
         if ( idy[j] == idy[i] )
         {
            ixy = j;
            break;
         }
   return 0;
}

// ----------------------------------------------------------------------------

/*
 * Compute the polynomial part P of the system matrix, and write it to the
 * working matrix in condensed form.
 *
 * Input
 * =====
 * n     Number of nodes.
 * m     Derivative order.
 * x,y   Vectors of node coordinates.
 *
 * Output
 * ======
 * a     Vector with condensed matrix that contains the kernel and polynomial
 *       parts, G and P respectively, of the system matrix. The polynomial part
 *       P begins at index (n*(n + 1))/2.
 */
template <typename T> static
PCL_HOT_FUNCTION void PolynomialPart( int n, int m, const T* __restrict__ x, const T* __restrict__ y, T* __restrict__ a )
{
   int mm12 = m*(m + 1)/2;

   IVector idx0( mm12 ); // [1...m*(m+1)/2] vector of powers of X
   IVector idy0( mm12 ); // [1...m*(m+1)/2] vector of powers of Y
   int* idx = *idx0 - 1;
   int* idy = *idy0 - 1;
   --x; --y; --a;

   // Skip the kernel function part G in the condensed matrix.
   a += (n*(n + 1))/2;

   // Set up the first monomial (= 1).
   T* __restrict__ a1 = a;
   PCL_IVDEP
   for ( int j = 0; j < n; ++j )
      *++a1 = 1;

   idx[1] = idy[1] = 0;
   a[n+1] = 0;

   // Compute the monomials  2,...,m(m + 1)/2
   for ( int kli = n + 1, i = 2; i <= mm12; kli += n+i, ++i )
   {
      // Find index of monomial that is multiplied by x or y.
      int ixy = 0;
      const T* __restrict__ xy = NextXYMonomial( i-1, idx, idy, ixy ) ? x : y;
      for ( int j = 1, kl = (ixy + n+n)*(ixy - 1)/2 + 1, klj = kli + 1; j <= n; ++j, ++kl, ++klj )
         a[klj] = a[kl] * xy[j];

      // Set rest of matrix to zero.
      a1 = a + kli + n;
      PCL_IVDEP
      for ( int j = 0; j < i; ++j )
         *++a1 = 0;
   }
}

// ----------------------------------------------------------------------------

/*
 * Surface spline generation. Build and solve a linear system of the form:
 *
 *   (G    P) (cv1)   (z)
 *   (      ) (   ) = ( )
 *   (P^T  0) (cv2)   (0)
 *
 * where G is an n*n matrix of kernel components, P is an n*m matrix of
 * polynomial components, cv1 are [0,...,n-1] surface spline coefficients, cv2
 * are [n,...,n + (m*(m + 1))/2 - 1] surface spline coefficients, and z is the
 * vector of n functional values.
 */
template <typename T> static
void GenerateSpline( T* __restrict__ cv,
                     const T* __restrict__ x, const T* __restrict__ y, const T* __restrict__ z,
                     int n, int m, float r, const float* __restrict__ w )
{
   // Size of the system matrix.
   int nm = n + (m*(m + 1))/2;

   // Vector for storage of a symmetric matrix in column packed form.
   GenericVector<T> A( nm*(nm + 1)/2 );

   // Pivot vector for matrix factorization.
   IVector pvt( nm );

   // Put the polynomial part P into the upper right corner.
   PolynomialPart( n, m, x, y, *A );

   // The kernel part G goes to the upper left corner.
   KernelPart( n, m, x, y, r, w, *A );

   /*
    * Compute matrix factorization: A = U*D*U^T
    *
    * N.B.: Factorize() is the bottleneck of this task. Example of execution
    * times measured for 1933 interpolation nodes:
    *
    * PolynomialPart(): 15.700 us
    * KernelPart():     51.536 ms
    * Factorize():     736.969 ms
    * Solve():           1.986 ms
    */
   if ( Factorize( *A, nm, *pvt ) != 0 )
      throw Error( "SurfaceSpline::Generate(): Singular matrix." );

   /*
    * Initialize the right-hand side vector.
    */
   for ( int i = 0; i < n; ++i )
      cv[i] = z[i];
   for ( int i = n; i < nm; ++i )
      cv[i] = 0;

   /*
    * Solve the linear system.
    */
   Solve( *A, nm, *pvt, cv );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void SurfaceSplineBase::Generate( float* __restrict__ cv,
                                  const float* __restrict__ x, const float* __restrict__ y, const float* __restrict__ z,
                                  int n, int m, float r, const float* __restrict__ w )
{
   GenerateSpline( cv, x, y, z, n, m, r, w );
}

void SurfaceSplineBase::Generate( double* __restrict__ cv,
                                  const double* __restrict__ x, const double* __restrict__ y, const double* __restrict__ z,
                                  int n, int m, float r, const float* __restrict__ w )
{
   GenerateSpline( cv, x, y, z, n, m, r, w );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/SurfaceSpline.cpp - Released 2020-12-17T15:46:35Z
