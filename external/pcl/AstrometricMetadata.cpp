//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/AstrometricMetadata.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/AstrometricMetadata.h>
#include <pcl/ImageWindow.h>
#include <pcl/ProjectionFactory.h>
#include <pcl/Version.h>
#include <pcl/XISF.h>

/*
 * Based on original work contributed by Andrés del Pozo.
 */

namespace pcl
{

// ----------------------------------------------------------------------------

AstrometricMetadata::AstrometricMetadata( ProjectionBase* projection,
                                          WorldTransformation* worldTransformation, int width, int height )
   : m_projection( projection )
   , m_transformWI( worldTransformation )
   , m_width( width )
   , m_height( height )
{
   LinearTransformation linearTransIW = m_transformWI->ApproximateLinearTransform();
   double resx = Sqrt( linearTransIW.A00() * linearTransIW.A00() + linearTransIW.A01() * linearTransIW.A01() );
   double resy = Sqrt( linearTransIW.A10() * linearTransIW.A10() + linearTransIW.A11() * linearTransIW.A11() );
   m_resolution = (resx + resy)/2;
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::Build( const PropertyArray& properties, const FITSKeywordArray& keywords,
                                 const ByteArray& controlPoints, int width, int height )
{
   m_description.Destroy();

   WCSKeywords wcs( properties, keywords );

   m_pixelSize = wcs.xpixsz;

   if ( wcs.dateobs.IsDefined() )
      m_obsStartTime = TimePoint( wcs.dateobs() );
   else
      m_obsStartTime = Optional<TimePoint>();

   if ( wcs.dateend.IsDefined() )
      m_obsEndTime = TimePoint( wcs.dateend() );
   else
      m_obsEndTime = Optional<TimePoint>();

   m_geoLongitude = wcs.longobs;
   m_geoLatitude = wcs.latobs;
   m_geoHeight = wcs.altobs;

   m_width = width;
   m_height = height;
   m_resolution = 0;

   if ( wcs.ctype1.StartsWith( "RA--" ) &&
        wcs.ctype2.StartsWith( "DEC-" ) &&
        wcs.crpix1.IsDefined() && wcs.crpix2.IsDefined() && wcs.crval1.IsDefined() && wcs.crval2.IsDefined() )
   {
      m_projection = ProjectionFactory::Create( wcs );

      LinearTransformation linearTransIW;
      if ( wcs.ExtractWorldTransformation( linearTransIW, m_height ) )
      {
         if ( controlPoints.IsEmpty() )
            m_transformWI = new LinearWorldTransformation( linearTransIW );
         else
            m_transformWI = new SplineWorldTransformation( controlPoints, linearTransIW );

         double resx = Sqrt( linearTransIW.A00()*linearTransIW.A00() + linearTransIW.A01()*linearTransIW.A01() );
         double resy = Sqrt( linearTransIW.A10()*linearTransIW.A10() + linearTransIW.A11()*linearTransIW.A11() );
         m_resolution = (resx + resy)/2;
         if ( m_pixelSize.IsDefined() )
            if ( m_pixelSize() > 0 )
               m_focalLength = FocalFromResolution( m_resolution );
      }
   }

   if ( m_transformWI.IsNull() )
   {
      if ( wcs.focallen.IsDefined() )
         if ( wcs.focallen() > 0 )
            m_focalLength = wcs.focallen();
      if ( m_focalLength.IsDefined() )
         if ( m_focalLength() > 0 )
            if ( m_pixelSize.IsDefined() )
               if ( m_pixelSize() > 0 )
                  m_resolution = ResolutionFromFocal( m_focalLength() );
   }
}

// ----------------------------------------------------------------------------

AstrometricMetadata::AstrometricMetadata( const ImageWindow& window )
{
   int width, height;
   View view = window.MainView();
   view.GetSize( width, height );

   ByteArray controlPoints;
   Variant v = view.PropertyValue( "Transformation_ImageToProjection" );
   if ( v.IsValid() )
      controlPoints = v.ToByteArray();

   Build( view.Properties(), window.Keywords(), controlPoints, width, height );
}

// ----------------------------------------------------------------------------

AstrometricMetadata::AstrometricMetadata( XISFReader& reader )
{
   ImageInfo info = reader.ImageInfo();

   ByteArray controlPoints;
   Variant v = reader.ReadImageProperty( "Transformation_ImageToProjection" );
   if ( v.IsValid() )
      controlPoints = v.ToByteArray();

   Build( reader.ReadImageProperties(), reader.ReadFITSKeywords(), controlPoints, info.width, info.height );
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::Write( ImageWindow& window, bool notify ) const
{
   if ( !IsValid() )
      throw Error( "AstrometricMetadata::Write(): Invalid or uninitialized metadata." );

   View view = window.MainView();
   if( view.Width() != m_width || view.Height() != m_height )
      throw Error( "AstrometricMetadata::Write(): Metadata not compatible with the dimensions of the image." );

   FITSKeywordArray keywords = window.Keywords();
   UpdateBasicKeywords( keywords );
   UpdateWCSKeywords( keywords );
   window.SetKeywords( keywords );

   if ( m_focalLength.IsDefined() && m_focalLength() > 0 )
      view.SetStorablePropertyValue( "Instrument:Telescope:FocalLength", Round( m_focalLength()/1000, 6 ), notify );

   if ( m_pixelSize.IsDefined() && m_pixelSize() > 0 )
   {
      view.SetStorablePropertyValue( "Instrument:Sensor:XPixelSize", Round( m_pixelSize(), 3 ), notify );
      view.SetStorablePropertyValue( "Instrument:Sensor:YPixelSize", Round( m_pixelSize(), 3 ), notify );
   }

   if ( m_obsStartTime.IsDefined() )
      view.SetStorablePropertyValue( "Observation:Time:Start", m_obsStartTime(), notify );

   if ( m_obsEndTime.IsDefined() )
      view.SetStorablePropertyValue( "Observation:Time:End", m_obsEndTime(), notify );

   if ( m_geoLongitude.IsDefined() && m_geoLatitude.IsDefined() )
   {
      view.SetStorablePropertyValue( "Observation:Location:Longitude", Round( m_geoLongitude(), 6 ), notify );
      view.SetStorablePropertyValue( "Observation:Location:Latitude", Round( m_geoLatitude(), 6 ), notify );
      if ( m_geoHeight.IsDefined() )
         view.SetStorablePropertyValue( "Observation:Location:Elevation", RoundInt( m_geoHeight() ), notify );
   }

   DPoint pRD;
   if ( ImageToCelestial( pRD, DPoint( m_width/2.0, m_height/2.0 ) ) )
   {
      view.SetStorablePropertyValue( "Observation:Center:RA", pRD.x, notify );
      view.SetStorablePropertyValue( "Observation:Center:Dec", pRD.y, notify );
      view.SetStorablePropertyValue( "Observation:CelestialReferenceSystem", "ICRS", notify );
      view.SetStorablePropertyValue( "Observation:Equinox", 2000.0, notify );
      // The default reference point is the geometric center of the image.
      view.DeleteProperty( "Observation:Center:X", notify );
      view.DeleteProperty( "Observation:Center:Y", notify );
   }

   const SplineWorldTransformation* splineTransform = dynamic_cast<const SplineWorldTransformation*>( m_transformWI.Pointer() );
   if ( splineTransform != nullptr )
   {
      ByteArray data;
      splineTransform->Serialize( data );
      view.SetStorablePropertyValue( "Transformation_ImageToProjection", data, notify );
   }
   else
      view.DeleteProperty( "Transformation_ImageToProjection", notify );
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::Write( XISFWriter& writer ) const
{
   if ( !IsValid() )
      throw Error( "AstrometricMetadata::Write(): Invalid or uninitialized metadata." );

   FITSKeywordArray keywords = writer.FITSKeywords();
   UpdateBasicKeywords( keywords );
   UpdateWCSKeywords( keywords );
   writer.WriteFITSKeywords( keywords );

   if ( m_focalLength.IsDefined() && m_focalLength() > 0 )
      writer.WriteImageProperty( "Instrument:Telescope:FocalLength", Round( m_focalLength()/1000, 6 ) );

   if ( m_pixelSize.IsDefined() && m_pixelSize() > 0 )
   {
      writer.WriteImageProperty( "Instrument:Sensor:XPixelSize", Round( m_pixelSize(), 3 ) );
      writer.WriteImageProperty( "Instrument:Sensor:YPixelSize", Round( m_pixelSize(), 3 ) );
   }

   if ( m_obsStartTime.IsDefined() )
      writer.WriteImageProperty( "Observation:Time:Start", m_obsStartTime() );

   if ( m_obsEndTime.IsDefined() )
      writer.WriteImageProperty( "Observation:Time:End", m_obsEndTime() );

   if ( m_geoLongitude.IsDefined() && m_geoLatitude.IsDefined() )
   {
      writer.WriteImageProperty( "Observation:Location:Longitude", Round( m_geoLongitude(), 6 ) );
      writer.WriteImageProperty( "Observation:Location:Latitude", Round( m_geoLatitude(), 6 ) );
      if ( m_geoHeight.IsDefined() )
         writer.WriteImageProperty( "Observation:Location:Elevation", RoundInt( m_geoHeight() ) );
   }

   DPoint pRD;
   if ( ImageToCelestial( pRD, DPoint( m_width/2.0, m_height/2.0 ) ) )
   {
      writer.WriteImageProperty( "Observation:Center:RA", pRD.x );
      writer.WriteImageProperty( "Observation:Center:Dec", pRD.y );
      writer.WriteImageProperty( "Observation:CelestialReferenceSystem", "ICRS" );
      writer.WriteImageProperty( "Observation:Equinox", 2000.0 );
      // The default reference point is the geometric center of the image.
      writer.RemoveImageProperty( "Observation:Center:X" );
      writer.RemoveImageProperty( "Observation:Center:Y" );
   }

   const SplineWorldTransformation* splineTransform = dynamic_cast<const SplineWorldTransformation*>( m_transformWI.Pointer() );
   if ( splineTransform != nullptr )
   {
      ByteArray data;
      splineTransform->Serialize( data );
      writer.WriteImageProperty( "Transformation_ImageToProjection", data );
   }
   else
      writer.RemoveImageProperty( "Transformation_ImageToProjection" );
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::Verify( DPoint& centerErrors,
                                  DPoint& topLeftErrors, DPoint& topRightErrors,
                                  DPoint& bottomLeftErrors, DPoint& bottomRightErrors ) const
{
   try
   {
      if ( !IsValid() )
         throw Error( "Invalid or uninitialized metadata." );

      /*
       * Performs full cycle transformations (image > celestial > image) and
       * reports the resulting absolute differences in pixels.
       */
      Array<DPoint> pI;
      pI << DPoint( m_width/2.0, m_height/2.0 )
         << DPoint( 0,           0 )
         << DPoint( m_width,     0 )
         << DPoint( 0,           m_height )
         << DPoint( m_width,     m_height );
      Array<DPoint> pIr;
      for ( int i = 0; i < 5; ++i )
      {
         DPoint pRD;
         if ( !ImageToCelestial( pRD, pI[i] ) )
            throw Error( String().Format(
               "Failed to perform ImageToCelestial() coordinate transformation, step %d.", i+1 ) );
         DPoint pIi;
         if ( !CelestialToImage( pIi, pRD ) )
            throw Error( String().Format(
               "Failed to perform CelestialToImage() coordinate transformation, step %d.", i+1 ) );
         pIr << DPoint( pI[i].x - pIi.x, pI[i].y - pIi.y );
      }
      centerErrors = pIr[0];
      topLeftErrors = pIr[1];
      topRightErrors = pIr[2];
      bottomLeftErrors = pIr[3];
      bottomRightErrors = pIr[4];
   }
   catch ( const Exception& x )
   {
      throw Error( "AstrometricMetadata::Verify(): " + x.Message() );
   }
   catch ( ... )
   {
      throw;
   }
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::Validate( double tolerance ) const
{
   DPoint e0, dontcare;
   Verify( e0, dontcare, dontcare, dontcare, dontcare );
   if ( Abs( e0.x ) > tolerance || Abs( e0.y ) > tolerance )
      throw Error( "AstrometricMetadata::Validate(): Inconsistent coordinate transformation results." );
}

// ----------------------------------------------------------------------------

double AstrometricMetadata::Rotation( bool& flipped ) const
{
   if ( m_transformWI.IsNull() )
      throw Error( "Invalid call to AstrometricMetadata::Rotation(): No world transformation defined." );

   LinearTransformation linearTransIW = m_transformWI->ApproximateLinearTransform();
   double det = linearTransIW.A01() * linearTransIW.A10() - linearTransIW.A00() * linearTransIW.A11();
   double rotation = Deg( ArcTan( linearTransIW.A00() + linearTransIW.A01(), linearTransIW.A10() + linearTransIW.A11() ) ) + 135;
   if ( det > 0 )
      rotation = -90 - rotation;
   if ( rotation <= -180 )
      rotation += 360;
   if ( rotation > 180 )
      rotation -= 360;
   flipped = det > 0;
   return rotation;
}

// ----------------------------------------------------------------------------

String AstrometricMetadata::Summary() const
{
   if ( !IsValid() )
      throw Error( "Invalid call to AstrometricMetadata::Summary(): No astrometric solution." );

   UpdateDescription();

   String summary;
   summary    << "Referentiation matrix (world[ra,dec] = matrix * image[x,y]):" << '\n'
              << m_description->referenceMatrix << '\n'
              << "WCS transformation ....... " << m_description->wcsTransformationType << '\n';
   if ( !m_description->controlPoints.IsEmpty() )
      summary << "Control points ........... " << m_description->controlPoints << '\n';
   if ( !m_description->splineLengths.IsEmpty() )
      summary << "Spline lengths ........... " << m_description->splineLengths << '\n';
   summary    << "Projection ............... " << m_description->projectionName << '\n'
              << "Projection origin ........ " << m_description->projectionOrigin << '\n'
              << "Resolution ............... " << m_description->resolution << '\n'
              << "Rotation ................. " << m_description->rotation << '\n';

   if ( !m_description->observationStartTime.IsEmpty() )
      summary << "Observation start time ... " << m_description->observationStartTime << '\n';
   if ( !m_description->observationEndTime.IsEmpty() )
      summary << "Observation end time ..... " << m_description->observationEndTime << '\n';

   if ( !m_description->observerLocation.IsEmpty() )
      summary << "Geodetic coordinates ..... " << m_description->observerLocation << '\n';

   if ( !m_description->focalDistance.IsEmpty() )
      summary << "Focal distance ........... " << m_description->focalDistance << '\n';

   if ( !m_description->pixelSize.IsEmpty() )
      summary << "Pixel size ............... " << m_description->pixelSize << '\n';

   summary    << "Field of view ............ " << m_description->fieldOfView << '\n'
              << "Image center ............. " << m_description->centerCoordinates << '\n'
              << "Image bounds:" << '\n'
              << "   top-left .............. " << m_description->topLeftCoordinates << '\n'
              << "   top-right ............. " << m_description->topRightCoordinates << '\n'
              << "   bottom-left ........... " << m_description->bottomLeftCoordinates << '\n'
              << "   bottom-right .......... " << m_description->bottomRightCoordinates << '\n';

   return summary;
}

// ----------------------------------------------------------------------------

static void ModifyKeyword( FITSKeywordArray& keywords, const IsoString& name, const IsoString& value, const IsoString& comment )
{
   for ( FITSHeaderKeyword& keyword : keywords )
      if ( keyword.name == name )
      {
         keyword.value = value;
         keyword.comment = comment;
         return;
      }

   keywords << FITSHeaderKeyword( name, value, comment );
}

static void ModifySignatureKeyword( FITSKeywordArray& keywords )
{
   FITSHeaderKeyword signature( "COMMENT", IsoString(), "Astrometric solution by " + PixInsightVersion::AsString() );
   for ( FITSHeaderKeyword& keyword : keywords )
      if ( keyword.name == "COMMENT" )
         if ( keyword.comment.StartsWith( "Astrometric solution by" ) )
         {
            keyword = signature;
            return;
         }
   keywords << signature;
}

static void RemoveKeyword( FITSKeywordArray& keywords, const IsoString& name )
{
   keywords.Remove( FITSHeaderKeyword( name ), []( const FITSHeaderKeyword& k1, const FITSHeaderKeyword& k2 )
                                               { return k1.name == k2.name; } );
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::UpdateBasicKeywords( FITSKeywordArray& keywords ) const
{
   ModifySignatureKeyword( keywords );

   if ( m_focalLength.IsDefined() && m_focalLength() > 0 )
      ModifyKeyword( keywords, "FOCALLEN", IsoString().Format( "%.8g", m_focalLength() ), "Focal length (mm)" );
   else
      RemoveKeyword( keywords, "FOCALLEN" );

   if ( m_pixelSize.IsDefined() && m_pixelSize() > 0 )
   {
      ModifyKeyword( keywords, "XPIXSZ", IsoString().Format( "%.6g", m_pixelSize() ), "Pixel size including binning, X-axis (um)" );
      ModifyKeyword( keywords, "YPIXSZ", IsoString().Format( "%.6g", m_pixelSize() ), "Pixel size including binning, Y-axis (um)" );
      RemoveKeyword( keywords, "PIXSIZE" );
   }

   ModifyKeyword( keywords, "TIMESYS", "UTC", "Time scale: Universal Time, Coordinated" );

   if ( m_obsStartTime.IsDefined() )
   {
      ModifyKeyword( keywords, "DATE-OBS",
            m_obsStartTime().ToIsoString( 3/*timeItems*/, 3/*precision*/, 0/*tz*/, false/*timeZone*/ ).SingleQuoted(),
            "Start date/time of observation (UTC)" );
      RemoveKeyword( keywords, "DATE-BEG" );
   }

   if ( m_obsEndTime.IsDefined() )
      ModifyKeyword( keywords, "DATE-END",
            m_obsEndTime().ToIsoString( 3/*timeItems*/, 3/*precision*/, 0/*tz*/, false/*timeZone*/ ).SingleQuoted(),
            "End date/time of observation (UTC)" );

   if ( m_geoLongitude.IsDefined() && m_geoLatitude.IsDefined() )
   {
      ModifyKeyword( keywords, "OBSGEO-L", IsoString().Format( "%.10g", m_geoLongitude() ), "Geodetic longitude of observation location (deg)" );
      ModifyKeyword( keywords, "LONG-OBS", IsoString().Format( "%.10g", m_geoLongitude() ), "Geodetic longitude (deg) (compatibility)" );
      RemoveKeyword( keywords, "SITELONG" );

      ModifyKeyword( keywords, "OBSGEO-B", IsoString().Format( "%.10g", m_geoLatitude() ), "Geodetic latitude of observation location (deg)" );
      ModifyKeyword( keywords, "LAT-OBS", IsoString().Format( "%.10g", m_geoLatitude() ), "Geodetic latitude (deg) (compatibility)" );
      RemoveKeyword( keywords, "SITELAT" );

      if ( m_geoHeight.IsDefined() )
      {
         ModifyKeyword( keywords, "OBSGEO-H", IsoString().Format( "%.0f", m_geoHeight() ), "Geodetic height of observation location (m)" );
         ModifyKeyword( keywords, "ALT-OBS", IsoString().Format( "%.0f", m_geoHeight() ), "Geodetic height (m) (compatibility)" );
         RemoveKeyword( keywords, "SITEELEV" );
      }
   }

   DPoint pRD;
   if ( ImageToCelestial( pRD, DPoint( m_width/2.0, m_height/2.0 ) ) )
   {
      ModifyKeyword( keywords, "RA",
            IsoString().Format( "%.16g", pRD.x ),
            "Right ascension of the center of the image (deg)" );
      ModifyKeyword( keywords, "OBJCTRA",
            IsoString::ToSexagesimal( pRD.x/15, RAConversionOptions( 3/*precision*/, 0/*width*/ ) ).SingleQuoted(),
            "Right ascension (hours) (compatibility)" );

      ModifyKeyword( keywords, "DEC",
            IsoString().Format( "%.16g", pRD.y ),
            "Declination of the center of the image (deg)" );
      ModifyKeyword( keywords, "OBJCTDEC",
            IsoString::ToSexagesimal( pRD.y, DecConversionOptions( 2/*precision*/, 0/*width*/ ) ).SingleQuoted(),
            "Declination (deg) (compatibility)" );
   }
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::UpdateWCSKeywords( FITSKeywordArray& keywords ) const
{
   RemoveKeyword( keywords, "RADESYS" );
   RemoveKeyword( keywords, "EQUINOX" );
   RemoveKeyword( keywords, "EPOCH" );
   RemoveKeyword( keywords, "CTYPE1" );
   RemoveKeyword( keywords, "CTYPE2" );
   RemoveKeyword( keywords, "CRPIX1" );
   RemoveKeyword( keywords, "CRPIX2" );
   RemoveKeyword( keywords, "CRVAL1" );
   RemoveKeyword( keywords, "CRVAL2" );
   RemoveKeyword( keywords, "PV1_1" );
   RemoveKeyword( keywords, "PV1_2" );
   RemoveKeyword( keywords, "PV1_3" );
   RemoveKeyword( keywords, "PV1_4" );
   RemoveKeyword( keywords, "LONPOLE" );
   RemoveKeyword( keywords, "LATPOLE" );
   RemoveKeyword( keywords, "CD1_1" );
   RemoveKeyword( keywords, "CD1_2" );
   RemoveKeyword( keywords, "CD2_1" );
   RemoveKeyword( keywords, "CD2_2" );
   RemoveKeyword( keywords, "PC1_1" );
   RemoveKeyword( keywords, "PC1_2" );
   RemoveKeyword( keywords, "PC2_1" );
   RemoveKeyword( keywords, "PC2_2" );
   RemoveKeyword( keywords, "REFSPLIN" );
   RemoveKeyword( keywords, "REFSPLINE" ); // N.B. 9-char keyword name written by old versions, not FITS-compliant.
   RemoveKeyword( keywords, "CDELT1" );
   RemoveKeyword( keywords, "CDELT2" );
   RemoveKeyword( keywords, "CROTA1" );
   RemoveKeyword( keywords, "CROTA2" );

   if ( IsValid() )
   {
      WCSKeywords wcs = ComputeWCSKeywords();

      if ( wcs.radesys.IsEmpty() )
      {
         if ( wcs.equinox.IsDefined() )
         {
            keywords << FITSHeaderKeyword( "RADESYS", (wcs.equinox() >= 1984.0) ? "FK5" : "FK4", "Reference system of celestial coordinates" );
            keywords << FITSHeaderKeyword( "EQUINOX", IsoString( wcs.equinox() ), "Epoch of the mean equator and equinox (years)" );
         }
         else
            keywords << FITSHeaderKeyword( "RADESYS", "ICRS", "Coordinates referred to ICRS / J2000.0" );
      }
      else
      {
         if ( wcs.radesys == "ICRS" )
            keywords << FITSHeaderKeyword( "RADESYS", "ICRS", "Coordinates referred to ICRS / J2000.0" );
         else if ( wcs.radesys == "GAPPT" )
            keywords << FITSHeaderKeyword( "RADESYS", "GAPPT", "Geocentric apparent coordinates / J2000.0" );
         else
         {
            keywords << FITSHeaderKeyword( "RADESYS", wcs.radesys, "Reference system of celestial coordinates" );
            if ( wcs.equinox.IsDefined() )
               keywords << FITSHeaderKeyword( "EQUINOX", IsoString( wcs.equinox() ), "Epoch of the mean equator and equinox (years)" );
         }
      }

      keywords << FITSHeaderKeyword( "CTYPE1", wcs.ctype1, "Axis1 projection: " + m_projection->Name() )
               << FITSHeaderKeyword( "CTYPE2", wcs.ctype2, "Axis2 projection: " + m_projection->Name() )
               << FITSHeaderKeyword( "CRPIX1", IsoString().Format( "%.16g", wcs.crpix1() ), "Axis1 reference pixel" )
               << FITSHeaderKeyword( "CRPIX2", IsoString().Format( "%.16g", wcs.crpix2() ), "Axis2 reference pixel" );

      if ( wcs.crval1.IsDefined() )
         keywords << FITSHeaderKeyword( "CRVAL1", IsoString().Format( "%.16g", wcs.crval1() ), "Axis1 reference value" );
      if ( wcs.crval2.IsDefined() )
         keywords << FITSHeaderKeyword( "CRVAL2", IsoString().Format( "%.16g", wcs.crval2() ), "Axis2 reference value" );
      if ( wcs.pv1_1.IsDefined() )
         keywords << FITSHeaderKeyword( "PV1_1", IsoString().Format( "%.16g", wcs.pv1_1() ), "Native longitude of the reference point (deg)" );
      if ( wcs.pv1_2.IsDefined() )
         keywords << FITSHeaderKeyword( "PV1_2", IsoString().Format( "%.16g", wcs.pv1_2() ), "Native latitude of the reference point (deg)" );
      if ( wcs.lonpole.IsDefined() )
         keywords << FITSHeaderKeyword( "LONPOLE", IsoString().Format( "%.16g", wcs.lonpole() ), "Longitude of the celestial pole (deg)" );
      if ( wcs.latpole.IsDefined() )
         keywords << FITSHeaderKeyword( "LATPOLE", IsoString().Format( "%.16g", wcs.latpole() ), "Latitude of the celestial pole (deg)" );

      keywords << FITSHeaderKeyword( "CD1_1", IsoString().Format( "%.16g", wcs.cd1_1() ), "Scale matrix (1,1)" )
               << FITSHeaderKeyword( "CD1_2", IsoString().Format( "%.16g", wcs.cd1_2() ), "Scale matrix (1,2)" )
               << FITSHeaderKeyword( "CD2_1", IsoString().Format( "%.16g", wcs.cd2_1() ), "Scale matrix (2,1)" )
               << FITSHeaderKeyword( "CD2_2", IsoString().Format( "%.16g", wcs.cd2_2() ), "Scale matrix (2,2)" );

      if ( HasSplineWorldTransformation() )
         keywords << FITSHeaderKeyword( "REFSPLIN", "T", "Thin plate spline astrometric solution available" );

      // AIPS keywords (CDELT1, CDELT2, CROTA1, CROTA2)
      keywords << FITSHeaderKeyword( "CDELT1", IsoString().Format( "%.16g", wcs.cdelt1() ), "Axis1 scale" )
               << FITSHeaderKeyword( "CDELT2", IsoString().Format( "%.16g", wcs.cdelt2() ), "Axis2 scale" )
               << FITSHeaderKeyword( "CROTA1", IsoString().Format( "%.16g", wcs.crota1() ), "Axis1 rotation angle (deg)" )
               << FITSHeaderKeyword( "CROTA2", IsoString().Format( "%.16g", wcs.crota2() ), "Axis2 rotation angle (deg)" );
   }
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::RemoveKeywords( FITSKeywordArray& keywords, bool removeCenterKeywords, bool removeScaleKeywords )
{
   if ( removeCenterKeywords )
   {
      RemoveKeyword( keywords, "RA" );
      RemoveKeyword( keywords, "OBJCTRA" );
      RemoveKeyword( keywords, "DEC" );
      RemoveKeyword( keywords, "OBJCTDEC" );
      RemoveKeyword( keywords, "RADESYS" );
      RemoveKeyword( keywords, "EQUINOX" );
      RemoveKeyword( keywords, "EPOCH" );
   }

   if ( removeScaleKeywords )
   {
      RemoveKeyword( keywords, "FOCALLEN" );
      RemoveKeyword( keywords, "XPIXSZ" );
      RemoveKeyword( keywords, "YPIXSZ" );
      RemoveKeyword( keywords, "PIXSIZE" );
   }

   RemoveKeyword( keywords, "CTYPE1" );
   RemoveKeyword( keywords, "CTYPE2" );
   RemoveKeyword( keywords, "CRVAL1" );
   RemoveKeyword( keywords, "CRVAL2" );
   RemoveKeyword( keywords, "CRPIX1" );
   RemoveKeyword( keywords, "CRPIX2" );
   RemoveKeyword( keywords, "CD1_1" );
   RemoveKeyword( keywords, "CD1_2" );
   RemoveKeyword( keywords, "CD2_1" );
   RemoveKeyword( keywords, "CD2_2" );
   RemoveKeyword( keywords, "PC1_1" );
   RemoveKeyword( keywords, "PC1_2" );
   RemoveKeyword( keywords, "PC2_1" );
   RemoveKeyword( keywords, "PC2_2" );
   RemoveKeyword( keywords, "CDELT1" );
   RemoveKeyword( keywords, "CDELT2" );
   RemoveKeyword( keywords, "CROTA1" );
   RemoveKeyword( keywords, "CROTA2" );
   RemoveKeyword( keywords, "PV1_1" );
   RemoveKeyword( keywords, "PV1_2" );
   RemoveKeyword( keywords, "PV1_3" );
   RemoveKeyword( keywords, "PV1_4" );
   RemoveKeyword( keywords, "LONPOLE" );
   RemoveKeyword( keywords, "LATPOLE" );
   RemoveKeyword( keywords, "REFSPLIN" );
   RemoveKeyword( keywords, "REFSPLINE" ); // N.B. 9-char keyword name written by old versions, not FITS-compliant.
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::RescalePixelSizeKeywords( FITSKeywordArray& keywords, double scalingFactor )
{
   for ( FITSHeaderKeyword& keyword : keywords )
      if ( keyword.name == "XPIXSZ" || keyword.name == "YPIXSZ" || keyword.name == "PIXSIZE" )
      {
         double size;
         if ( keyword.StripValueDelimiters().TryToDouble( size ) )
            keyword.value = IsoString().Format( "%.6g", size*scalingFactor );
      }
}

// ----------------------------------------------------------------------------

static void ModifyProperty( PropertyArray& properties, const IsoString& id, const Variant& value )
{
   PropertyArray::iterator i = properties.Search( id );
   if ( i != properties.End() )
      i->SetValue( value );
   else
      properties << Property( id, value );
}

static void RemoveProperty( PropertyArray& properties, const IsoString& id )
{
   properties.Remove( Property( id ) );
}

void AstrometricMetadata::UpdateProperties( PropertyArray& properties ) const
{
   if ( IsValid() )
   {
      if ( m_focalLength.IsDefined() && m_focalLength() > 0 )
         ModifyProperty( properties, "Instrument:Telescope:FocalLength", Round( m_focalLength()/1000, 6 ) );
      else
         RemoveProperty( properties, "Instrument:Telescope:FocalLength" );

      if ( m_pixelSize.IsDefined() && m_pixelSize() > 0 )
      {
         ModifyProperty( properties, "Instrument:Sensor:XPixelSize", Round( m_pixelSize(), 3 ) );
         ModifyProperty( properties, "Instrument:Sensor:YPixelSize", Round( m_pixelSize(), 3 ) );
      }

      if ( m_obsStartTime.IsDefined() )
         ModifyProperty( properties, "Observation:Time:Start", m_obsStartTime() );

      if ( m_obsEndTime.IsDefined() )
         ModifyProperty( properties, "Observation:Time:End", m_obsEndTime() );

      if ( m_geoLongitude.IsDefined() && m_geoLatitude.IsDefined() )
      {
         ModifyProperty( properties, "Observation:Location:Longitude", Round( m_geoLongitude(), 6 ) );
         ModifyProperty( properties, "Observation:Location:Latitude", Round( m_geoLatitude(), 6 ) );
         if ( m_geoHeight.IsDefined() )
            ModifyProperty( properties, "Observation:Location:Elevation", RoundInt( m_geoHeight() ) );
      }

      DPoint pRD;
      if ( ImageToCelestial( pRD, DPoint( m_width/2.0, m_height/2.0 ) ) )
      {
         ModifyProperty( properties, "Observation:Center:RA", pRD.x );
         ModifyProperty( properties, "Observation:Center:Dec", pRD.y );
         ModifyProperty( properties, "Observation:CelestialReferenceSystem", "ICRS" );
         ModifyProperty( properties, "Observation:Equinox", 2000.0 );
         // The default reference point is the geometric center of the image.
         RemoveProperty( properties, "Observation:Center:X" );
         RemoveProperty( properties, "Observation:Center:Y" );
      }

      const SplineWorldTransformation* splineTransform = dynamic_cast<const SplineWorldTransformation*>( m_transformWI.Pointer() );
      if ( splineTransform != nullptr )
      {
         ByteArray data;
         splineTransform->Serialize( data );
         ModifyProperty( properties, "Transformation_ImageToProjection", data );
      }
      else
         RemoveProperty( properties, "Transformation_ImageToProjection" );
   }
   else
      RemoveProperty( properties, "Transformation_ImageToProjection" );
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::RemoveProperties( PropertyArray& properties, bool removeCenterProperties, bool removeScaleProperties )
{
   if ( removeCenterProperties )
   {
      RemoveProperty( properties, "Observation:Center:RA" );
      RemoveProperty( properties, "Observation:Center:Dec" );
      RemoveProperty( properties, "Observation:Center:X" );
      RemoveProperty( properties, "Observation:Center:Y" );
      RemoveProperty( properties, "Observation:CelestialReferenceSystem" );
      RemoveProperty( properties, "Observation:Equinox" );
   }

   if ( removeScaleProperties )
   {
      RemoveProperty( properties, "Instrument:Telescope:FocalLength" );
      RemoveProperty( properties, "Instrument:Sensor:XPixelSize" );
      RemoveProperty( properties, "Instrument:Sensor:YPixelSize" );
   }

   RemoveProperty( properties, "Transformation_ImageToProjection" );
}

void AstrometricMetadata::RemoveProperties( ImageWindow& window, bool removeCenterProperties, bool removeScaleProperties )
{
   View view = window.MainView();

   if ( removeCenterProperties )
   {
      view.DeletePropertyIfExists( "Observation:Center:RA" );
      view.DeletePropertyIfExists( "Observation:Center:Dec" );
      view.DeletePropertyIfExists( "Observation:Center:X" );
      view.DeletePropertyIfExists( "Observation:Center:Y" );
      view.DeletePropertyIfExists( "Observation:CelestialReferenceSystem" );
      view.DeletePropertyIfExists( "Observation:Equinox" );
   }

   if ( removeScaleProperties )
   {
      view.DeletePropertyIfExists( "Instrument:Telescope:FocalLength" );
      view.DeletePropertyIfExists( "Instrument:Sensor:XPixelSize" );
      view.DeletePropertyIfExists( "Instrument:Sensor:YPixelSize" );
   }

   view.DeletePropertyIfExists( "Transformation_ImageToProjection" );
}

// ----------------------------------------------------------------------------

void AstrometricMetadata::RescalePixelSizeProperties( PropertyArray& properties, double scalingFactor )
{
   PropertyArray::iterator i = properties.Search( IsoString( "Instrument:Sensor:XPixelSize" ) );
   if ( i != properties.End() )
      i->SetValue( i->Value().ToDouble()*scalingFactor );
   i = properties.Search( IsoString( "Instrument:Sensor:YPixelSize" ) );
   if ( i != properties.End() )
      i->SetValue( i->Value().ToDouble()*scalingFactor );
}

void AstrometricMetadata::RescalePixelSizeProperties( ImageWindow& window, double scalingFactor )
{
   View view = window.MainView();
   if ( view.HasProperty( "Instrument:Sensor:XPixelSize" ) )
      view.SetPropertyValue( "Instrument:Sensor:XPixelSize",
                             view.PropertyValue( "Instrument:Sensor:XPixelSize" ).ToDouble()*scalingFactor );
   if ( view.HasProperty( "Instrument:Sensor:YPixelSize" ) )
      view.SetPropertyValue( "Instrument:Sensor:YPixelSize",
                             view.PropertyValue( "Instrument:Sensor:YPixelSize" ).ToDouble()*scalingFactor );
}

// ----------------------------------------------------------------------------

WCSKeywords AstrometricMetadata::ComputeWCSKeywords() const
{
   if ( !IsValid() )
      throw Error( "AstrometricMetadata::ComputeWCSKeywords(): Invalid or uninitialized metadata" );

   LinearTransformation trans_F_I( 1,  0, -0.5,
                                   0, -1,  m_height+0.5 );
   LinearTransformation trans_I_W = m_transformWI->ApproximateLinearTransform();
   LinearTransformation trans_F_W = trans_I_W.Multiply( trans_F_I );

   WCSKeywords wcs;
   m_projection->GetWCS( wcs );

   wcs.cd1_1 = trans_F_W.A00();
   wcs.cd1_2 = trans_F_W.A01();
   wcs.cd2_1 = trans_F_W.A10();
   wcs.cd2_2 = trans_F_W.A11();

   DPoint orgF = trans_F_W.TransformInverse( DPoint( 0 ) );
   wcs.crpix1 = orgF.x;
   wcs.crpix2 = orgF.y;

   /*
    * CDELT1, CDELT2 and CROTA2 are computed using the formulas in section 6.2
    * of http://fits.gsfc.nasa.gov/fits_wcs.html "Representations of celestial
    * coordinates in FITS".
    */
   double rot1, rot2;
   if ( wcs.cd2_1() > 0 )
      rot1 = ArcTan( wcs.cd2_1(), wcs.cd1_1() );
   else if ( wcs.cd2_1() < 0 )
      rot1 = ArcTan( -wcs.cd2_1(), -wcs.cd1_1() );
   else
      rot1 = 0;

   if ( wcs.cd1_2() > 0 )
      rot2 = ArcTan( wcs.cd1_2(), -wcs.cd2_2() );
   else if ( wcs.cd1_2() < 0 )
      rot2 = ArcTan( -wcs.cd1_2(), wcs.cd2_2() );
   else
      rot2 = 0;

   double rot = (rot1 + rot2)/2;
   rot2 = rot1 = rot;
   double sinrot, cosrot;
   SinCos( rot, sinrot, cosrot );
   if ( Abs( cosrot ) > Abs( sinrot ) )
   {
      wcs.cdelt1 =  wcs.cd1_1()/cosrot;
      wcs.cdelt2 =  wcs.cd2_2()/cosrot;
   }
   else
   {
      wcs.cdelt1 =  wcs.cd2_1()/sinrot;
      wcs.cdelt2 = -wcs.cd1_2()/sinrot;
   }

   wcs.crota1 = Deg( rot1 );
   wcs.crota2 = Deg( rot2 );

   return wcs;
}

// ----------------------------------------------------------------------------

static String FieldString( double field )
{
   int sign, s1, s2; double s3;
   IsoString::ToSexagesimal( field ).ParseSexagesimal( sign, s1, s2, s3 );
   if ( s1 > 0 )
      return String().Format( "%dd %d' %.1f\"", s1, s2, s3 );
   if ( s2 > 0 )
      return String().Format( "%d' %.1f\"", s2, s3 );
   return String().Format( "%.2f\"", s3 );
}

static String CelestialToString( const DPoint& pRD )
{
   double ra = pRD.x;
   if ( ra < 0 )
      ra += 360;
   return String()
      << "RA: "
      << String().ToSexagesimal( ra/15, RAConversionOptions( 3/*precision*/ ) )
      << "  Dec: "
      << String().ToSexagesimal( pRD.y, DecConversionOptions( 2/*precision*/ ) );
}

static String ImageToCelestialToString( const AstrometricMetadata* A, const DPoint& pI )
{
   DPoint pRD;
   if ( A->ImageToCelestial( pRD, pI ) )
      return CelestialToString( pRD );
   return "------";
}

static String ReprojectionErrorsToString( const DPoint& e )
{
   return String().Format( "  ex: %+9.6f px  ey: %+9.6f px", e.x, e.y );
}

void AstrometricMetadata::UpdateDescription() const
{
   if ( m_description.IsNull() )
      if ( IsValid() )
      {
         LinearTransformation linearTransIW = m_transformWI->ApproximateLinearTransform();
         DPoint projOrgPx = linearTransIW.TransformInverse( DPoint( 0 ) );
         DPoint projOrgRD = m_projection->ProjectionOrigin();
         bool flipped;
         double rotation = Rotation( flipped );

         const SplineWorldTransformation* S = dynamic_cast<const SplineWorldTransformation*>( m_transformWI.Pointer() );

         m_description = new DescriptionItems;

         m_description->referenceMatrix = linearTransIW.ToString();
         if ( S != nullptr )
         {
            m_description->wcsTransformationType = "Thin plate spline";
            m_description->controlPoints = String( S->NumberOfControlPoints() );
            int xWI, yWI, xIW, yIW;
            S->GetSplineLengths( xWI, yWI, xIW, yIW );
            m_description->splineLengths = String().Format( "l:%d b:%d X:%d Y:%d", xWI, yWI, xIW, yIW );
         }
         else
            m_description->wcsTransformationType = "Linear";

         m_description->projectionName = m_projection->Name();
         m_description->projectionOrigin = String().Format( "[%.6f %.6f] px", projOrgPx.x, projOrgPx.y )
                                             << " -> [" << CelestialToString( projOrgRD ) << ']';
         m_description->resolution = String().Format( "%.3f arcsec/px", m_resolution*3600 );
         m_description->rotation = String().Format( "%.3f deg", rotation ) << (flipped ? " (flipped)" : "");

         if ( m_obsStartTime.IsDefined() )
            m_description->observationStartTime = m_obsStartTime().ToString( "%Y-%M-%D %h:%m:%s0 UTC" );

         if ( m_obsEndTime.IsDefined() )
            m_description->observationEndTime = m_obsEndTime().ToString( "%Y-%M-%D %h:%m:%s0 UTC" );

         if ( m_geoLongitude.IsDefined() && m_geoLatitude.IsDefined() )
         {
            m_description->observerLocation =
                  String().ToSexagesimal( Abs( m_geoLongitude() ),
                                          SexagesimalConversionOptions( 3/*items*/,
                                                                        0/*precision*/,
                                                                        false/*sign*/,
                                                                        3/*width*/,
                                                                        ' '/*separator*/ ) )
                  << ' ' << ((m_geoLongitude() < 0) ? 'W' : 'E')
                  << "  "
                  << String().ToSexagesimal( Abs( m_geoLatitude() ),
                                          SexagesimalConversionOptions( 3/*items*/,
                                                                        0/*precision*/,
                                                                        false/*sign*/,
                                                                        2/*width*/,
                                                                        ' '/*separator*/ ) )
                  << ' ' << ((m_geoLatitude() < 0) ? 'S' : 'N');

            if ( m_geoHeight.IsDefined() )
               m_description->observerLocation << String().Format( "  %.0f m", m_geoHeight() );
         }

         if ( m_pixelSize.IsDefined() )
            if ( m_pixelSize() > 0 )
               if ( m_focalLength.IsDefined() )
               {
                  m_description->focalDistance = String().Format( "%.2f mm", m_focalLength() );
                  m_description->pixelSize = String().Format( "%.2f um", m_pixelSize() );
               }

         m_description->fieldOfView = FieldString( m_width*m_resolution ) << " x " << FieldString( m_height*m_resolution );
         m_description->centerCoordinates = ImageToCelestialToString( this, DPoint( m_width/2.0, m_height/2.0 ) );
         m_description->topLeftCoordinates = ImageToCelestialToString( this, DPoint( 0, 0 ) );
         m_description->topRightCoordinates = ImageToCelestialToString( this, DPoint( m_width, 0 ) );
         m_description->bottomLeftCoordinates = ImageToCelestialToString( this, DPoint( 0, m_height ) );
         m_description->bottomRightCoordinates = ImageToCelestialToString( this, DPoint( m_width, m_height ) );

         if ( S != nullptr )
         {
            DPoint e0, e1, e2, e3, e4;
            Verify( e0, e1, e2, e3, e4 );
            m_description->centerCoordinates << ReprojectionErrorsToString( e0 );
            m_description->topLeftCoordinates << ReprojectionErrorsToString( e1 );
            m_description->topRightCoordinates << ReprojectionErrorsToString( e2 );
            m_description->bottomLeftCoordinates << ReprojectionErrorsToString( e3 );
            m_description->bottomRightCoordinates << ReprojectionErrorsToString( e4 );
         }
      }
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/AstrometricMetadata.cpp - Released 2020-12-17T15:46:35Z
