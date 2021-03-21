//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/WorldTransformation.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_WorldTransformation_h
#define __PCL_WorldTransformation_h

/// \file pcl/WorldTransformation.h

#include <pcl/Defs.h>

#include <pcl/GridInterpolation.h>
#include <pcl/LinearTransformation.h>
#include <pcl/SurfaceSpline.h>
#include <pcl/WCSKeywords.h>

/*
 * Based on original work contributed by Andrés del Pozo.
 */

namespace pcl
{

// ----------------------------------------------------------------------------

#define __PCL_WCS_DEFAULT_SPLINE_ORDER                       2
#define __PCL_WCS_DEFAULT_SPLINE_SMOOTHNESS                  0.025F
#define __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_ENABLED         true
#define __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_TOLERANCE       0.25F // px
#define __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_REJECT_FRACTION 0.10F
#define __PCL_WCS_MAX_SPLINE_POINTS                          2100

// ----------------------------------------------------------------------------

/*!
 * \class WorldTransformation
 * \brief Abstract base class of world coordinate transformations
 *
 * \ingroup astrometry_support
 */
class PCL_CLASS WorldTransformation
{
public:

   /*!
    * Default constructor.
    */
   WorldTransformation() = default;

   /*!
    * Copy constructor.
    */
   WorldTransformation( const WorldTransformation& ) = default;

   /*!
    * Virtual destructor.
    */
   virtual ~WorldTransformation()
   {
   }

   /*!
    * Returns true iff this transformation is empty (uninitialized, invalid).
    */
   virtual bool IsEmpty() const
   {
      return false;
   }

   /*!
    * Returns a dynamically allocated copy of this object.
    */
   virtual WorldTransformation* Clone() const = 0;

   /*!
    * Transforms from native spherical coordinates to image coordinates.
    *
    * The point \a p contains native spherical coordinates: \a p.x is the
    * native longitude and \a p.y is the native latitude, both expressed in
    * degrees. Returns image coordinates in pixels corresponding to \a p.
    */
   virtual DPoint Direct( const DPoint& p ) const = 0;

   /*!
    * Transforms from image coordinates to native spherical coordinates.
    *
    * The specified point \a p contains image coordinates in pixels. Returns a
    * point \a q where \a q.x is the native longitude and \a q.y is the native
    * latitude, both expressed in degrees, corresponding to \a p.
    */
   virtual DPoint Inverse( const DPoint& p ) const = 0;

   /*!
    * Returns an approximate linear transformation from Image to World
    * coordinates.
    */
   virtual const LinearTransformation& ApproximateLinearTransform() const = 0;
};

// ----------------------------------------------------------------------------

/*!
 * \class LinearWorldTransformation
 * \brief WCS linear world coordinate transformation
 *
 * \ingroup astrometry_support
 */
class PCL_CLASS LinearWorldTransformation : public WorldTransformation
{
public:

   /*!
    * Constructor from a linear transformation. The specified transformation
    * \a transIW must transform from image coordinates to native spherical
    * coordinates.
    */
   LinearWorldTransformation( const LinearTransformation& transIW )
      : m_transWI( transIW.Inverse() )
      , m_transIW( transIW )
   {
   }

   /*!
    * Copy constructor.
    */
   LinearWorldTransformation( const LinearWorldTransformation& ) = default;

   /*!
    * Move constructor.
    */
   LinearWorldTransformation( LinearWorldTransformation&& ) = default;

   /*!
    * Virtual destructor.
    */
   virtual ~LinearWorldTransformation()
   {
   }

   /*!
    * Returns false, since a linear WCS transformation cannot be empty.
    */
   bool IsEmpty() const override
   {
      return false;
   }

   /*!
    */
   WorldTransformation* Clone() const override
   {
      return new LinearWorldTransformation( *this );
   }

   /*!
    */
   DPoint Direct( const DPoint& p ) const override
   {
      return m_transWI.Transform( p );
   }

   /*!
    */
   DPoint Inverse( const DPoint& p ) const override
   {
      return m_transIW.Transform( p );
   }

   /*!
    * Returns a reference to the internal linear transformation (from image to
    * native spherical coordinates).
    */
   const LinearTransformation& ApproximateLinearTransform() const override
   {
      return m_transIW;
   }

private:

   LinearTransformation m_transWI; // world -> image
   LinearTransformation m_transIW; // image -> world
};

// ----------------------------------------------------------------------------

/*!
 * \class SplineWorldTransformation
 * \brief Surface spline world coordinate transformation
 *
 * %SplineWorldTransformation implements a world coordinate transform based on
 * 2-D interpolating/approximating surface splines (also known as <em>thin
 * plates</em>). These mathematical modeling devices allow for accurate
 * representations of coordinate systems subject to arbitrary local
 * distortions, which are impossible to achieve with linear transformations.
 *
 * The underlying implementation of this class uses the SurfaceSpline,
 * PointSurfaceSpline, SurfaceSimplifier and PointGridInterpolation classes.
 *
 * \ingroup astrometry_support
 */
class PCL_CLASS SplineWorldTransformation : public WorldTransformation
{
public:

   /*!
    * Constructs a 2-D spline based world coordinate transformation.
    *
    * \param controlPointsW   Array of world control points. Each point in this
    *                         array must contain spherical coordinates in the
    *                         native world coordinate system of the astrometric
    *                         solution, expressed in degrees. For a given
    *                         point p in this array, p.x is the native
    *                         longitude coordinate and p.y is the latitude.
    *
    * \param controlPointsI   Array of image control points. Each point in this
    *                         array must contain the coordinates on the X and Y
    *                         axes of the image plane corresponding to the same
    *                         point in the \a controlPointsW array. In other
    *                         words, there must be a one-to-one correspondence
    *                         between world and image control points.
    *
    * \param smoothness       When this parameter is greater than zero,
    *                         approximating splines will be generated instead
    *                         of interpolating splines. The higher this value,
    *                         the closest will be the 2-D approximating surface
    *                         to the reference plane of the image.
    *                         Approximating surface splines are robust to
    *                         outlier control points and hence recommended in
    *                         virtually all cases. The value of this parameter
    *                         should be relatively small if \a enableSimplifier
    *                         is true, since the surface simplification
    *                         algorithm already performs robust rejection of
    *                         outliers. The default value of 0.025 is normally
    *                         quite appropriate.
    *
    * \param weights          When the \a smoothness parameter is greater than
    *                         zero and this vector is not empty, it must define
    *                         a positive weight greater than zero for each
    *                         point defined by the \a controlPointsW and
    *                         \a controlPointsI arrays. If \a smoothness is
    *                         zero or negative, this parameter will be ignored.
    *                         See the PointSurfaceSpline::Initialize() member
    *                         function for detailed information on 2-D spline
    *                         node weights. If \a enableSimplifier is true,
    *                         control point weights are not used and hence the
    *                         value of this parameter will be ignored.
    *
    * \param order            Derivative order of continuity. The default value
    *                         is 2, which is also the recommended value. Higher
    *                         orders may improve adaptability to complex field
    *                         distortions only in some special cases, but at
    *                         the risk of leading to ill-conditioned linear
    *                         systems, and hence to errors while building the
    *                         surface splines.
    *
    * \param enableSimplifier If true, a surface simplification algorithm will
    *                         be applied to the lists of control points for
    *                         surface spline generation. The use of surface
    *                         simplification greatly improves efficiency of
    *                         surface splines by removing all redundant points
    *                         and keeping only the control points required to
    *                         define the coordinate transformations accurately.
    *                         In addition, the applied surface simplification
    *                         algorithm implements robust PCA fitting and
    *                         outlier rejection techniques that improve the
    *                         generated interpolation devices in terms of
    *                         resilience to noise and invalid data in the
    *                         underlying astrometric solution. This option is
    *                         enabled by default. When this parameter is true,
    *                         the \a weights parameter is ignored.
    *
    * \param simplifierTolerance       Tolerance of the surface simplification
    *                         algorithm in pixels. The default value is 0.25
    *                         pixels.
    *
    * \param simplifierRejectFraction  Fraction of rejected control points for
    *                         simplification of surface subregions. The default
    *                         value is 0.10.
    *
    * Thanks to the implemented surface simplification algorithms, we can work
    * with very large sets of control points to generate astrometric solutions
    * able to model strong and complex arbitrary field distortions accurately
    * and efficiently. Surface simplification concentrates control points just
    * where they are necessary to represent distortions, leaving undistorted
    * regions of the image covered by a minimal grid.
    *
    * Newly constructed instances are guaranteed to be valid (in the structural
    * and numerical senses; note that this does not necessarily imply that the
    * underlying astrometric solution is valid). In the event of invalid input
    * data or spline initialization problems, this constructor will throw an
    * Error exception.
    *
    * After surface spline initialization, an approximate linear transformation
    * will also be calculated automatically.
    */
   SplineWorldTransformation( const Array<DPoint>& controlPointsW,
                              const Array<DPoint>& controlPointsI,
                              float smoothness = __PCL_WCS_DEFAULT_SPLINE_SMOOTHNESS,
                              const FVector& weights = FVector(),
                              int order = __PCL_WCS_DEFAULT_SPLINE_ORDER,
                              bool enableSimplifier = __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_ENABLED,
                              float simplifierTolerance = __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_TOLERANCE,
                              float simplifierRejectFraction = __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_REJECT_FRACTION )
      : m_controlPointsW( controlPointsW )
      , m_controlPointsI( controlPointsI )
      , m_order( order )
      , m_smoothness( smoothness )
      , m_weights( weights )
      , m_enableSimplifier( enableSimplifier )
      , m_simplifierTolerance( simplifierTolerance )
      , m_simplifierRejectFraction( simplifierRejectFraction )
   {
      InitializeSplines();
      CalculateLinearApproximation();
   }

   /*!
    * Constructs a %SplineWorldTransformation instance by deserializing the
    * specified raw \a data.
    *
    * An approximate linear transformation will be calculated automatically.
    */
   SplineWorldTransformation( const ByteArray& data )
   {
      Deserialize( data );
      InitializeSplines();
      CalculateLinearApproximation();
   }

   /*!
    * Constructs a %SplineWorldTransformation instance by deserializing the
    * specified raw \a data, with a prescribed approximate linear
    * transformation \a linearTransIW.
    */
   SplineWorldTransformation( const ByteArray& data, const LinearTransformation& linearTransIW )
   {
      Deserialize( data );
      InitializeSplines();
      m_linearIW = linearTransIW;
   }

   /*!
    * Copy constructor.
    */
   SplineWorldTransformation( const SplineWorldTransformation& ) = default;

   /*!
    * Move constructor.
    */
   SplineWorldTransformation( SplineWorldTransformation&& ) = default;

   /*!
    * Virtual destructor.
    */
   virtual ~SplineWorldTransformation()
   {
   }

   /*!
    * Copy assignment operator. Returns a reference to this object.
    */
   SplineWorldTransformation& operator =( const SplineWorldTransformation& ) = default;

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   SplineWorldTransformation& operator =( SplineWorldTransformation&& ) = default;

   /*!
    * Returns true iff this object has no working data.
    *
    * With the restrictions imposed by class constructors, this can only happen
    * if this object is an xvalue after move construction or assignment.
    */
   bool IsEmpty() const override
   {
      return m_controlPointsW.IsEmpty() || m_controlPointsI.IsEmpty();
   }

   /*!
    * Returns a dynamically allocated copy of this object.
    */
   WorldTransformation* Clone() const override
   {
      return new SplineWorldTransformation( *this );
   }

   /*!
    * Transforms from native spherical coordinates to image coordinates.
    *
    * The point \a p contains native spherical coordinates: \a p.x is the
    * native longitude and \a p.y is the native latitude, both expressed in
    * degrees. Returns image coordinates in pixels corresponding to \a p.
    *
    * If grid interpolations have been initialized for this object, and the
    * specified point \a p is included in the reference rectangle used for grid
    * initialization, they will be used transparently by this member function.
    * Otherwise the surface splines will be evaluated directly, which can be
    * much slower, depending on the number of control points defined by the
    * astrometric solution.
    *
    * \sa Inverse(), InitializeGridInterpolations()
    */
   DPoint Direct( const DPoint& p ) const override
   {
      if ( m_gridWI.IsValid() )
         if ( m_gridWI.ReferenceRect().IncludesFast( p ) )
            return m_gridWI( p );
      return m_splineWI( p );
   }

   /*!
    * Transforms from image coordinates to native spherical coordinates.
    *
    * The specified point \a p contains image coordinates in pixels. Returns a
    * point \a q where \a q.x is the native longitude and \a q.y is the native
    * latitude, both expressed in degrees, corresponding to \a p.
    *
    * If grid interpolations have been initialized for this object, and the
    * specified point \a p is included in the reference rectangle used for grid
    * initialization, they will be used transparently by this member function.
    * Otherwise the surface splines will be evaluated directly, which can be
    * much slower, depending on the number of control points defined by the
    * astrometric solution.
    *
    * \sa Direct(), InitializeGridInterpolations()
    */
   DPoint Inverse( const DPoint& p ) const override
   {
      if ( m_gridIW.IsValid() )
         if ( m_gridIW.ReferenceRect().IncludesFast( p ) )
            return m_gridIW( p );
      return m_splineIW( p );
   }

   /*!
    * Returns an approximate linear transformation from image to native
    * spherical coordinates, computed from the internal point surface splines.
    */
   const LinearTransformation& ApproximateLinearTransform() const override
   {
      return m_linearIW;
   }

   /*!
    * Initializes the internal grid interpolation devices for the specified
    * reference rectangular region \a rect and grid distance \a delta in
    * pixels.
    *
    * A grid distance of 24 pixels is applied by default. This is normally more
    * than sufficient to yield accurate coordinate readouts, even for strongly
    * distorted images.
    *
    * See GridInterpolation::Initialize() for detailed information on spline
    * grid interpolation and its working parameters.
    *
    * \note This member function will show no console messages. If some
    * feedback must be provided to the user during the potentially long
    * operation, it must be given before calling this function.
    */
   void InitializeGridInterpolations( const Rect& rect, int delta = 24 )
   {
      m_gridWI.Initialize( rect, delta, m_splineWI, false/*verbose*/ );
      m_gridIW.Initialize( rect, delta, m_splineIW, false/*verbose*/ );
   }

   /*!
    * Returns true if the internal grid interpolation devices have been
    * initialized. See InitializeGridInterpolations() for information on grid
    * interpolation.
    */
   bool HasGridInterpolations() const
   {
      return m_gridWI.IsValid() && m_gridIW.IsValid();
   }

   /*!
    * Serializes this %SplineWorldTransformation instance in raw binary format
    * and stores the result in the specified \a data array.
    */
   void Serialize( ByteArray& data ) const;

   /*!
    * Returns the number of control points employed to generate the surface
    * splines used for coordinate transformations.
    *
    * The value returned by this function is the length of the original list of
    * control points specified upon object construction or deserialization; the
    * actual sets of data points being used internally by surface splines are
    * usually smaller because surface simplification is enabled by default.
    *
    * \sa NativeControlPoints(), ImageControlPoints(), GetSplineLengths()
    */
   int NumberOfControlPoints() const
   {
      return int( m_controlPointsW.Length() );
   }

   /*!
    * Returns a reference to the list of control points in native spherical
    * coordinates.
    *
    * For each point \e p in the returned array, \e p.x is the native longitude
    * and \e p.y is the native latitude, both expressed in degrees.
    *
    * For each point \a p in the returned array, there is a corresponding point
    * \a q in the array of image control points returned by the
    * \a ImageControlPoints() member function, whose \e q.x and \e q.y
    * components are the image coordinates in pixels corresponding to \e p.
    */
   const Array<DPoint>& NativeControlPoints() const
   {
      return m_controlPointsW;
   }

   /*!
    * Returns a reference to the list of control points in image coordinates.
    *
    * For each point \e p in the returned array, \e p.x and \e p.y are the
    * components of a control point in image coordinates, expressed in pixels.
    *
    * For each point \a p in the returned array, there is a corresponding point
    * \a q in the array of native control points returned by the
    * \a NativeControlPoints() member function, whose \e q.x and \e q.y
    * components are the spherical native coordinates in degrees corresponding
    * to \e p.
    */
   const Array<DPoint>& ImageControlPoints() const
   {
      return m_controlPointsI;
   }

   /*!
    * Provides the number of data points in the internal surface splines used
    * for coordinate transformations.
    *
    * \param[out] xWI   Number of spline points used for world-to-image
    *                   coordinate transformations on the native longitude
    *                   direction.
    *
    * \param[out] yWI   Number of spline points used for world-to-image
    *                   coordinate transformations on the native latitude
    *                   direction.
    *
    * \param[out] xIW   Number of spline points used for image-to-world
    *                   coordinate transformations on the X-axis direction.
    *
    * \param[out] yIW   Number of spline points used for image-to-world
    *                   coordinate transformations on the Y-axis direction.
    */
   void GetSplineLengths( int& xWI, int& yWI, int& xIW, int& yIW ) const
   {
      xWI = m_splineWI.SplineX().Length();
      yWI = m_splineWI.SplineY().Length();
      xIW = m_splineIW.SplineX().Length();
      yIW = m_splineIW.SplineY().Length();
   }

   /*!
    * Returns true iff the lists of transformation control points were
    * truncated before generation of surface splines. If surface simplification
    * is enabled, truncation may happen, when necessary, after simplification.
    * If no simplification is used, truncation may happen directly on the
    * original lists of control points.
    *
    * A maximum of 2100 control points is imposed in the current implementation
    * to prevent excessive execution times for surface spline generation, which
    * grow approximately with O(n^3) complexity.
    */
   bool TruncatedControlPoints() const
   {
      return m_truncated;
   }

   /*!
    * Returns true iff the surface simplification algorithm has been enabled
    * for generation of surface splines. See the class constructor for more
    * information.
    */
   bool IsSimplifierEnabled() const
   {
      return m_enableSimplifier;
   }

   /*!
    * Returns the tolerance in pixels of the surface simplifier used for
    * generation of surface splines. See the class constructor for more
    * information.
    */
   float SimplifierTolerance() const
   {
      return m_simplifierTolerance;
   }

   /*!
    * Returns the outlier rejection fraction of the surface simplifier used for
    * generation of surface splines. See the class constructor for more
    * information.
    */
   float SimplifierRejectFraction() const
   {
      return m_simplifierRejectFraction;
   }

private:

   Array<DPoint>              m_controlPointsW;
   Array<DPoint>              m_controlPointsI;
   int                        m_order = __PCL_WCS_DEFAULT_SPLINE_ORDER;
   float                      m_smoothness = __PCL_WCS_DEFAULT_SPLINE_SMOOTHNESS;
   FVector                    m_weights;
   bool                       m_enableSimplifier = __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_ENABLED;
   float                      m_simplifierTolerance = __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_TOLERANCE;
   float                      m_simplifierRejectFraction = __PCL_WCS_SURFACE_SIMPLIFIER_DEFAULT_REJECT_FRACTION;
   bool                       m_truncated = false;
   PointSurfaceSpline<DPoint> m_splineWI;
   PointSurfaceSpline<DPoint> m_splineIW;
   PointGridInterpolation     m_gridWI;
   PointGridInterpolation     m_gridIW;
   LinearTransformation       m_linearIW;

   void Deserialize( const ByteArray& );
   void InitializeSplines();
   void CalculateLinearApproximation();
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_WorldTransformation_h

// ----------------------------------------------------------------------------
// EOF pcl/WorldTransformation.h - Released 2020-12-17T15:46:29Z
