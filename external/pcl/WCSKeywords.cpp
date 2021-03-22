//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/WCSKeywords.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/LinearTransformation.h>
#include <pcl/TimePoint.h>
#include <pcl/WCSKeywords.h>

/*
 * Based on original work contributed by Andrés del Pozo.
 */

namespace pcl
{

// ----------------------------------------------------------------------------

void WCSKeywords::Read( const PropertyArray& properties, const FITSKeywordArray& keywords )
{
   Optional<double> expTime; // only if Observation:Time:End is not available

   /*
    * XISF properties take precedence over FITS keywords.
    *
    * ### TODO: When defined by the XISF standard, include all properties in
    *           the WCS namespace.
    */
   for ( const Property& property : properties )
   {
      if ( property.Id() == "Observation:Center:RA" )
         objctra = property.Value().ToDouble();
      else if ( property.Id() == "Observation:Center:Dec" )
         objctdec = property.Value().ToDouble();
      else if ( property.Id() == "Observation:CelestialReferenceSystem" )
         radesys = property.Value().ToIsoString();
      else if ( property.Id() == "Observation:Equinox" )
         equinox = property.Value().ToDouble();
      else if ( property.Id() == "Observation:Time:Start" )
         dateobs = property.Value().ToTimePoint().JD();
      else if ( property.Id() == "Observation:Time:End" )
         dateend = property.Value().ToTimePoint().JD();
      else if ( property.Id() == "Observation:Location:Longitude" )
         longobs = property.Value().ToDouble();
      else if ( property.Id() == "Observation:Location:Latitude" )
         latobs = property.Value().ToDouble();
      else if ( property.Id() == "Observation:Location:Elevation" )
         altobs = property.Value().ToDouble();
      else if ( property.Id() == "Instrument:Telescope:FocalLength" )
         focallen = property.Value().ToDouble() * 1000;
      else if ( property.Id() == "Instrument:Sensor:XPixelSize" )
         xpixsz = property.Value().ToDouble();
      else if ( property.Id() == "Instrument:ExposureTime" )
         expTime = property.Value().ToDouble();
   }

   /*
    * Standard WCS FITS keywords.
    */
   for ( const FITSHeaderKeyword& keyword : keywords )
   {
      IsoString svalue = keyword.StripValueDelimiters();
      double nvalue;
      if ( keyword.name == "CTYPE1" )
      {
         ctype1 = svalue;
      }
      else if ( keyword.name == "CTYPE2" )
      {
         ctype2 = svalue;
      }
      else if ( keyword.name == "CRVAL1" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            crval1 = nvalue;
      }
      else if ( keyword.name == "CRVAL2" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            crval2 = nvalue;
      }
      else if ( keyword.name == "CRPIX1" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            crpix1 = nvalue;
      }
      else if ( keyword.name == "CRPIX2" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            crpix2 = nvalue;
      }
      else if ( keyword.name == "CD1_1" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            cd1_1 = nvalue;
      }
      else if ( keyword.name == "CD1_2" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            cd1_2 = nvalue;
      }
      else if ( keyword.name == "CD2_1" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            cd2_1 = nvalue;
      }
      else if ( keyword.name == "CD2_2" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            cd2_2 = nvalue;
      }
      else if ( keyword.name == "CDELT1" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            cdelt1 = nvalue;
      }
      else if ( keyword.name == "CDELT2" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            cdelt2 = nvalue;
      }
      else if ( keyword.name == "CROTA1" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            crota1 = nvalue;
      }
      else if ( keyword.name == "CROTA2" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            crota2 = nvalue;
      }
      else if ( keyword.name == "PV1_1" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            pv1_1 = nvalue;
      }
      else if ( keyword.name == "PV1_2" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            pv1_2 = nvalue;
      }
      else if ( keyword.name == "PV1_3" || keyword.name == "LONPOLE" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            lonpole = nvalue;
      }
      else if ( keyword.name == "PV1_4" || keyword.name == "LATPOLE" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            latpole = nvalue;
      }
      else if ( keyword.name == "REFSPLIN" || keyword.name == "REFSPLINE" )
      {
         // N.B. Be compatible with 9-char keyword "REFSPLINE" written by old
         // versions of the ImageSolver script.
         refSpline = svalue;
      }
   }

   /*
    * Primary optional FITS keywords.
    */
   for ( const FITSHeaderKeyword& keyword : keywords )
   {
      IsoString svalue = keyword.StripValueDelimiters();
      double nvalue;
      if ( !objctra.IsDefined() && keyword.name == "RA" )
      {
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
            if ( nvalue >= 0 )
            {
               /*
                * The RA keyword value can be either a complex angular
                * representation in hours (hh mm ss.sss) or a scalar in degrees
                * ([+|-]ddd.dddddd).
                */
               if ( svalue.Contains( ' ' ) || svalue.Contains( ':' ) )
                  nvalue *= 15;
               if ( nvalue <= 360 )
               {
                  if ( nvalue == 360 )
                     nvalue = 0;
                  objctra = nvalue;
               }
            }
      }
      else if ( !objctdec.IsDefined() && keyword.name == "DEC" )
      {
         /*
          * The DEC keyword value can be either a complex angular
          * representation in degrees ([+|-]dd mm ss.sss) or a scalar
          * ([+|-]ddd.dddddd), also in degrees.
          */
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
            if ( nvalue >= -90 )
               if ( nvalue <= +90 )
                  objctdec = nvalue;
      }
      else if ( radesys.IsEmpty() && keyword.name == "RADESYS" )
         radesys = svalue.Uppercase();
      else if ( !equinox.IsDefined() && keyword.name == "EQUINOX" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            equinox = nvalue;
      }
      else if ( !dateobs.IsDefined() && keyword.name == "DATE-BEG" )
      {
         TimePoint t;
         if ( TimePoint::TryFromString( t, svalue ) )
            dateobs = t.JD();
      }
      else if ( !dateend.IsDefined() && keyword.name == "DATE-END" )
      {
         TimePoint t;
         if ( TimePoint::TryFromString( t, svalue ) )
            dateend = t.JD();
      }
      else if ( !longobs.IsDefined() && keyword.name == "OBSGEO-L" )
      {
         /*
          * The OBSGEO-L keyword value can be either a complex angular
          * representation in degrees ([+|-]ddd mm ss.sss) or a scalar in
          * degrees ([+|-]ddd.dddddd), positive East.
          */
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
         {
            if ( nvalue > 180 )
               nvalue -= 360;
            else if ( nvalue <= -180 )
               nvalue += 360;
            if ( nvalue >= -180 )
               if ( nvalue <= 180 )
                  longobs = nvalue;
         }
      }
      else if ( !latobs.IsDefined() && keyword.name == "OBSGEO-B" )
      {
         /*
          * The OBSGEO-B keyword value can be either a complex angular
          * representation in degrees ([+|-]dd mm ss.sss) or a scalar in
          * degrees ([+|-]dd.dddddd), positive North.
          */
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
            if ( nvalue >= -90 )
               if ( nvalue <= +90 )
                  latobs = nvalue;
      }
      else if ( !altobs.IsDefined() && keyword.name == "OBSGEO-H" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            altobs = nvalue;
      }
      else if ( !focallen.IsDefined() && keyword.name == "FOCALLEN" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            focallen = nvalue;
      }
      else if ( !xpixsz.IsDefined() && keyword.name == "XPIXSZ" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            xpixsz = nvalue;
      }
      else if ( !expTime.IsDefined() && keyword.name == "EXPTIME" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            expTime = nvalue;
      }
   }

   /*
    * Secondary optional FITS keywords, supported for compatibility with some
    * applications.
    */
   for ( const FITSHeaderKeyword& keyword : keywords )
   {
      IsoString svalue = keyword.StripValueDelimiters();
      double nvalue;
      if ( !objctra.IsDefined() && keyword.name == "OBJCTRA" )
      {
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
            if ( nvalue >= 0 )
            {
               nvalue *= 15;
               if ( nvalue <= 360 )
               {
                  if ( nvalue == 360 )
                     nvalue = 0;
                  objctra = nvalue;
               }
            }
      }
      else if ( !objctdec.IsDefined() && keyword.name == "OBJCTDEC" )
      {
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
            if ( nvalue >= -90 )
               if ( nvalue <= +90 )
                  objctdec = nvalue;
      }
      else if ( !dateobs.IsDefined() && keyword.name == "DATE-OBS" )
      {
         TimePoint t;
         if ( TimePoint::TryFromString( t, svalue ) )
            dateobs = t.JD();
      }
      else if ( !longobs.IsDefined() && (keyword.name == "LONG-OBS" || keyword.name == "SITELONG") )
      {
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
         {
            if ( nvalue > 180 )
               nvalue -= 360;
            else if ( nvalue <= -180 )
               nvalue += 360;
            if ( nvalue >= -180 )
               if ( nvalue <= 180 )
                  longobs = nvalue;
         }
      }
      else if ( !latobs.IsDefined() && (keyword.name == "LAT-OBS" || keyword.name == "SITELAT") )
      {
         if ( svalue.TrySexagesimalToDouble( nvalue, Array<char>() << ' ' << ':' ) )
            if ( nvalue >= -90 )
               if ( nvalue <= +90 )
                  latobs = nvalue;
      }
      else if ( !altobs.IsDefined() && (keyword.name == "ALT-OBS" || keyword.name == "SITEELEV") )
      {
         if ( svalue.TryToDouble( nvalue ) )
            altobs = nvalue;
      }
      else if ( !xpixsz.IsDefined() && keyword.name == "PIXSIZE" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            xpixsz = nvalue;
      }
      else if ( !expTime.IsDefined() && keyword.name == "EXPOSURE" )
      {
         if ( svalue.TryToDouble( nvalue ) )
            expTime = nvalue;
      }
   }

   /*
    * If Observation:Time:End is not available, try to approximate it from the
    * observation start time and exposure time in seconds.
    */
   if ( !dateend.IsDefined() )
      if ( dateobs.IsDefined() )
         if ( expTime.IsDefined() )
            dateend = dateobs() + expTime()/86400;

   // For mental sanity, ensure start_time <= end_time.
   if ( dateobs.IsDefined() )
      if ( dateend.IsDefined() )
         if ( dateend() < dateobs() )
            Swap( dateobs, dateend );
}

// ----------------------------------------------------------------------------

bool WCSKeywords::ExtractWorldTransformation( LinearTransformation& transIW, int imageHeight )
{
   /*
    * Transform pixel coordinates in FITS convention to World coordinates.
    */
   LinearTransformation transFW;
   if ( cd1_1.IsDefined() && cd1_2.IsDefined() && cd2_1.IsDefined() && cd2_2.IsDefined() )
   {
      transFW = LinearTransformation( cd1_1(), cd1_2(), -cd1_1()*crpix1() - cd1_2()*crpix2(),
                                      cd2_1(), cd2_2(), -cd2_1()*crpix1() - cd2_2()*crpix2() );
   }
   else if ( cdelt1.IsDefined() && cdelt2.IsDefined() )
   {
      if ( !crota2.IsDefined() )
         crota2 = 0;
      double sinr, cosr;
      SinCos( Rad( crota2() ), sinr, cosr );
      double cd1_1 =  cdelt1()*cosr;
      double cd1_2 = -cdelt2()*sinr;
      double cd2_1 =  cdelt1()*sinr;
      double cd2_2 =  cdelt2()*cosr;
      transFW = LinearTransformation( cd1_1, cd1_2, -cd1_1*crpix1() - cd1_2*crpix2(),
                                      cd2_1, cd2_2, -cd2_1*crpix1() - cd2_2*crpix2() );
   }
   else
      return false;

   /*
    * Transforms pixel coordinates between FITS and PixInsight conventions.
    */
   LinearTransformation ref_F_I( 1,  0,              -0.5,
                                 0, -1, imageHeight + 0.5 );
   transIW = transFW.Multiply( ref_F_I.Inverse() );
   return true;
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/WCSKeywords.cpp - Released 2020-12-17T15:46:35Z
