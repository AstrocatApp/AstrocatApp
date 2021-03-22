//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/IntegrationMetadata.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/IntegrationMetadata.h>

namespace pcl
{

// ----------------------------------------------------------------------------

IntegrationMetadata::IntegrationMetadata( const PropertyArray& properties, const FITSKeywordArray& keywords )
{
   /*
    * XISF properties take precedence over FITS keywords.
    */
   for ( auto p : properties )
   {
      try
      {
         if ( p.Id() ==      "Instrument:Camera:Gain" )
            cameraGain = p.Value().ToDouble();
         else if ( p.Id() == "Instrument:Camera:ISOSpeed" )
            cameraISO = p.Value().ToUInt();
         else if ( p.Id() == "Instrument:Camera:Name" )
            instrumentName = p.Value().ToString();
         else if ( p.Id() == "Instrument:Camera:XBinning" )
            xBinning = p.Value().ToUInt();
         else if ( p.Id() == "Instrument:Camera:YBinning" )
            yBinning = p.Value().ToUInt();
         else if ( p.Id() == "Instrument:ExposureTime" )
            expTime = p.Value().ToDouble();
         else if ( p.Id() == "Instrument:Filter:Name" )
            filterName = p.Value().ToString();
         else if ( p.Id() == "Instrument:Sensor:Temperature" )
            sensorTemp = p.Value().ToDouble();
         else if ( p.Id() == "Instrument:Sensor:XPixelSize" )
            xPixSize = p.Value().ToDouble();
         else if ( p.Id() == "Instrument:Sensor:YPixelSize" )
            yPixSize = p.Value().ToDouble();
         else if ( p.Id() == "Instrument:Telescope:Aperture" )
            aperture = p.Value().ToDouble() * 1000;
         else if ( p.Id() == "Instrument:Telescope:CollectingArea" )
            apertureArea = p.Value().ToDouble() * 1000 * 1000;
         else if ( p.Id() == "Instrument:Telescope:FocalLength" )
            focalLength = p.Value().ToDouble() * 1000;
         else if ( p.Id() == "Instrument:Telescope:Name" )
            telescopeName = p.Value().ToString();
         else if ( p.Id() == "Observer:Name" )
            observer = p.Value().ToString();
         else if ( p.Id() == "Observation:Authors" )
            author = p.Value().ToString();
         else if ( p.Id() == "Observation:Object:Name" )
            objectName = p.Value().ToString();
         else if ( p.Id() == "Observation:Center:RA" )
            ra = p.Value().ToDouble();
         else if ( p.Id() == "Observation:Center:Dec" )
            dec = p.Value().ToDouble();
         else if ( p.Id() == "Observation:CelestialReferenceSystem" )
            celCrdSys = p.Value().ToIsoString();
         else if ( p.Id() == "Observation:Equinox" )
            equinox = p.Value().ToDouble();
         else if ( p.Id() == "Observation:Time:Start" )
            startTime = p.Value().ToTimePoint();
         else if ( p.Id() == "Observation:Time:End" )
            endTime = p.Value().ToTimePoint();
         else if ( p.Id() == "Observation:Location:Longitude" )
            longObs = p.Value().ToDouble();
         else if ( p.Id() == "Observation:Location:Latitude" )
            latObs = p.Value().ToDouble();
         else if ( p.Id() == "Observation:Location:Elevation" )
            altObs = p.Value().ToDouble();
         else if ( p.Id() == "PCL:CFASourcePatternName" )
            cfaPatternName = p.Value().ToIsoString();
         else if ( p.Id() == "PCL:CFASourcePattern" )
            cfaPattern = p.Value().ToIsoString();
      }
      catch ( Exception& x )
      {
         Console().CriticalLn( "<end><cbr>*** Error: Parsing " + p.Id() + " image property: " + x.Message() );
      }
      catch ( ... )
      {
         throw;
      }
   }

   /*
    * Primary FITS keywords.
    */
   for ( auto k : keywords )
   {
      try
      {
         IsoString value = k.StripValueDelimiters();

         if ( !author.IsDefined() && k.name == "AUTHOR" )
            author = String( value );
         else if ( !observer.IsDefined() && k.name == "OBSERVER" )
            observer = String( value );
         else if ( !instrumentName.IsDefined() && k.name == "INSTRUME" )
            instrumentName = String( value );
         else if ( !frameType.IsDefined() && k.name == "IMAGETYP" )
            frameType = String( value );
         else if ( !filterName.IsDefined() && k.name == "FILTER" )
            filterName = String( value );
         else if ( !pedestal.IsDefined() && k.name == "PEDESTAL" )
         {
            double p = value.ToDouble();
            if ( p < 0 )
            {
//                Console().WarningLn( "<end><cbr>** Warning: Assuming that negative PEDESTAL keyword value '"
//                                     + value + "\' is intended to be added." );
               p = -p;
            }
            pedestal = p;
         }
         else if ( !expTime.IsDefined() && k.name == "EXPTIME" )
            expTime = Round( value.ToDouble(), 3 ); // 1 ms
         else if ( !sensorTemp.IsDefined() && k.name == "CCD-TEMP" )
            sensorTemp = Round( value.ToDouble(), 1 ); // 0.1 C
         else if ( !xPixSize.IsDefined() && k.name == "XPIXSZ" )
            xPixSize = value.ToDouble();
         else if ( !yPixSize.IsDefined() && k.name == "YPIXSZ" )
            yPixSize = value.ToDouble();
         else if ( !cameraGain.IsDefined() && k.name == "EGAIN" )
            cameraGain = Max( 0.0, value.ToDouble() );
         else if ( !cameraISO.IsDefined() && k.name == "ISOSPEED" )
            cameraISO = unsigned( Max( 1.0, value.ToDouble() ) );
         else if ( !xBinning.IsDefined() && k.name == "XBINNING" )
            xBinning = unsigned( Max( 1.0, value.ToDouble() ) );
         else if ( !yBinning.IsDefined() && k.name == "YBINNING" )
            yBinning = unsigned( Max( 1.0, value.ToDouble() ) );
         else if ( !xOrigin.IsDefined() && k.name == "XORGSUBF" )
            xOrigin = unsigned( Max( 0.0, value.ToDouble() ) );
         else if ( !yOrigin.IsDefined() && k.name == "YORGSUBF" )
            yOrigin = unsigned( Max( 0.0, value.ToDouble() ) );
         else if ( !telescopeName.IsDefined() && k.name == "TELESCOP" )
            telescopeName = String( value );
         else if ( !focalLength.IsDefined() && k.name == "FOCALLEN" )
            focalLength = value.ToDouble();
         else if ( !aperture.IsDefined() && k.name == "APTDIA" )
            aperture = value.ToDouble();
         else if ( !apertureArea.IsDefined() && k.name == "APTAREA" )
            apertureArea = value.ToDouble();
         else if ( !objectName.IsDefined() && k.name == "OBJNAME" )
            objectName = String( value );
         else if ( !startTime.IsDefined() && k.name == "DATE-BEG" )
            startTime = TimePoint( value );
         else if ( !endTime.IsDefined() && k.name == "DATE-END" )
            endTime = TimePoint( value );
         else if ( !ra.IsDefined() && k.name == "RA" )
         {
            double x = value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            // The "RA" keyword must be expressed in hours when it
            // is a complex angular value (HH MM SS.sss), or in
            // degrees when it is a scalar.
            if ( value.Contains( ' ' ) || value.Contains( ':' ) )
               x *= 15;
            if ( x < 0 || x > 360 )
               throw Error( "Right ascension value out of range: '" + value + '\'' );
            // Keep R.A. in (-180,+180], so we can average all values later.
            if ( x > 180 )
               x -= 360;
            ra = x;
         }
         else if ( !dec.IsDefined() && k.name == "DEC" )
         {
            double x = value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            if ( x < -90 || x > 90 )
               throw Error( "Declination value out of range: '" + value + '\'' );
            dec = x;
         }
         else if ( !celCrdSys.IsDefined() && k.name == "RADESYS" )
            celCrdSys = value.Uppercase();
         else if ( !equinox.IsDefined() && k.name == "EQUINOX" )
            equinox = value.ToDouble();
         else if ( !longObs.IsDefined() && k.name == "OBSGEO-L" )
         {
            double x = value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            // Keep longitudes in (-180,+180], so we can average all values later.
            if ( x > 180 )
               x -= 360;
            else if ( x <= -180 )
               x += 360;
            if ( x < -180 || x > 180 )
               throw Error( "Geographic longitude value out of range: '" + value + '\'' );
            longObs = x;
         }
         else if ( !latObs.IsDefined() && k.name == "OBSGEO-B" )
         {
            double x = value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            if ( x < -90 || x > 90 )
               throw Error( "Geographic latitude value out of range: '" + value + '\'' );
            latObs = x;
         }
         else if ( !altObs.IsDefined() && k.name == "OBSGEO-H" )
            altObs = value.ToDouble();
         else if ( !cfaPattern.IsDefined() && k.name == "BAYERPAT" )
            cfaPattern = value.Uppercase();
         else if ( k.name == "XBAYROFF" )
            cfaXOffset = TruncInt( value.ToDouble() );
         else if ( k.name == "YBAYROFF" )
            cfaYOffset = TruncInt( value.ToDouble() );
      }
      catch ( Exception& x )
      {
         Console().CriticalLn( "<end><cbr>*** Error: Parsing " + k.name + " FITS keyword: " + x.Message() );
      }
      catch ( ... )
      {
         throw;
      }
   }

   /*
    * Secondary FITS keywords, supported for compatibility with some
    * applications.
    */
   for ( auto k : keywords )
   {
      try
      {
         IsoString value = k.StripValueDelimiters();

         if ( !frameType.IsDefined() && k.name == "FRAME" )
            frameType = String( value );
         else if ( !filterName.IsDefined() && k.name == "INSFLNAM" )
            filterName = String( value );
         else if ( !expTime.IsDefined() && k.name == "EXPOSURE" )
            expTime = Round( value.ToDouble(), 3 ); // 1 ms
         else if ( !xPixSize.IsDefined() && k.name == "PIXSIZE" )
            xPixSize = yPixSize = value.ToDouble();
         else if ( !xBinning.IsDefined() && k.name == "CCDBINX" )
            xBinning = unsigned( Max( 1.0, value.ToDouble() ) );
         else if ( !yBinning.IsDefined() && k.name == "CCDBINY" )
            yBinning = unsigned( Max( 1.0, value.ToDouble() ) );
         else if ( !xBinning.IsDefined() && k.name == "BINNING" )
            xBinning = yBinning = unsigned( Max( 1.0, value.ToDouble() ) );
         else if ( !objectName.IsDefined() && k.name == "OBJECT" )
            objectName = String( value );
         else if ( !startTime.IsDefined() && k.name == "DATE-OBS" )
            startTime = TimePoint( value );
         else if ( !ra.IsDefined() && k.name == "OBJCTRA" )
         {
            double x = 15 * value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            if ( x < 0 || x > 360 )
               throw Error( "Right ascension value out of range: '" + value + '\'' );
            // Keep R.A. in (-180,+180], so we can average all values later.
            if ( x > 180 )
               x -= 360;
            ra = x;
         }
         else if ( !dec.IsDefined() && k.name == "OBJCTDEC" )
         {
            double x = value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            if ( x < -90 || x > 90 )
               throw Error( "Declination value out of range: '" + value + '\'' );
            dec = x;
         }
         else if ( !longObs.IsDefined() && (k.name == "LONG-OBS" || k.name == "SITELONG") )
         {
            double x = value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            // Keep longitudes in (-180,+180], so we can average all values later.
            if ( x > 180 )
               x -= 360;
            else if ( x <= -180 )
               x += 360;
            if ( x < -180 || x > 180 )
               throw Error( "Geographic longitude value out of range: '" + value + '\'' );
            longObs = x;
         }
         else if ( !latObs.IsDefined() && (k.name == "LAT-OBS" || k.name == "SITELAT") )
         {
            double x = value.SexagesimalToDouble( Array<char>() << ' ' << ':' );
            if ( x < -90 || x > 90 )
               throw Error( "Geographic latitude value out of range: '" + value + '\'' );
            latObs = x;
         }
         else if ( !altObs.IsDefined() && (k.name == "ALT-OBS" || k.name == "SITEELEV") )
         {
            altObs = value.ToDouble();
         }
      }
      catch ( Exception& x )
      {
         Console().CriticalLn( "<end><cbr>*** Error: Parsing " + k.name + " FITS keyword: " + x.Message() );
      }
      catch ( ... )
      {
         throw;
      }
   }

   if ( equinox.IsDefined() )
   {
      if ( celCrdSys.IsDefined() )
         if ( celCrdSys() == "ICRS" || celCrdSys() == "GAPPT" )
            equinox.Undefine();
   }
   else
   {
      if ( ra.IsDefined() )
         if ( dec.IsDefined() )
            if ( celCrdSys.IsDefined() && celCrdSys() != "ICRS" && celCrdSys() != "GAPPT" )
               equinox = 2000.0; // assume FK5 / J2000.0
            else
               celCrdSys = "ICRS";
   }

   if ( !endTime.IsDefined() )
      if ( startTime.IsDefined() )
         if ( expTime.IsDefined() )
            endTime = startTime() + expTime()/86400;

   m_valid = true;
}

// ----------------------------------------------------------------------------

IntegrationMetadata::IntegrationMetadata( const String& serialization )
{
   String data = serialization.Trimmed();
   if ( data.IsEmpty() )
      return;

   try
   {
      StringList items;
      data.Break( items, ItemSeparator );
      for ( const String& item : items )
      {
         StringList tokens;
         item.Break( tokens, TokenSeparator );
         if ( tokens.Length() != 2 )
            throw 0;
         if ( !tokens[1].IsEmpty() )
            if (      tokens[0] == "author" )
               author = tokens[1];
            else if ( tokens[0] == "observer" )
               observer = tokens[1];
            else if ( tokens[0] == "instrumentName" )
               instrumentName = tokens[1];
            else if ( tokens[0] == "frameType" )
               frameType = tokens[1];
            else if ( tokens[0] == "filterName" )
               filterName = tokens[1];
            else if ( tokens[0] == "cfaPatternName" )
               cfaPatternName = IsoString( tokens[1] );
            else if ( tokens[0] == "cfaPattern" )
               cfaPattern = IsoString( tokens[1] );
            else if ( tokens[0] == "cfaXOffset" )
               cfaXOffset = tokens[1].ToInt();
            else if ( tokens[0] == "cfaYOffset" )
               cfaYOffset = tokens[1].ToInt();
            else if ( tokens[0] == "pedestal" )
               pedestal = tokens[1].ToDouble();
            else if ( tokens[0] == "expTime" )
               expTime = tokens[1].ToDouble();
            else if ( tokens[0] == "sensorTemp" )
               sensorTemp = tokens[1].ToDouble();
            else if ( tokens[0] == "xPixSize" )
               xPixSize = tokens[1].ToDouble();
            else if ( tokens[0] == "yPixSize" )
               yPixSize = tokens[1].ToDouble();
            else if ( tokens[0] == "cameraGain" )
               cameraGain = tokens[1].ToDouble();
            else if ( tokens[0] == "cameraISO" )
               cameraISO = tokens[1].ToUInt();
            else if ( tokens[0] == "xBinning" )
               xBinning = tokens[1].ToUInt();
            else if ( tokens[0] == "yBinning" )
               yBinning = tokens[1].ToUInt();
            else if ( tokens[0] == "xOrigin" )
               xOrigin = tokens[1].ToUInt();
            else if ( tokens[0] == "yOrigin" )
               yOrigin = tokens[1].ToUInt();
            else if ( tokens[0] == "telescopeName" )
               telescopeName = tokens[1];
            else if ( tokens[0] == "focalLength" )
               focalLength = tokens[1].ToDouble();
            else if ( tokens[0] == "aperture" )
               aperture = tokens[1].ToDouble();
            else if ( tokens[0] == "apertureArea" )
               apertureArea = tokens[1].ToDouble();
            else if ( tokens[0] == "objectName" )
               objectName = tokens[1];
            else if ( tokens[0] == "startTime" )
               startTime = TimePoint( tokens[1] );
            else if ( tokens[0] == "endTime" )
               endTime = TimePoint( tokens[1] );
            else if ( tokens[0] == "ra" )
               ra = tokens[1].ToDouble();
            else if ( tokens[0] == "dec" )
               dec = tokens[1].ToDouble();
            else if ( tokens[0] == "celCrdSys" )
               celCrdSys = IsoString( tokens[1] );
            else if ( tokens[0] == "equinox" )
               equinox = tokens[1].ToDouble();
            else if ( tokens[0] == "longObs" )
               longObs = tokens[1].ToDouble();
            else if ( tokens[0] == "latObs" )
               latObs = tokens[1].ToDouble();
            else if ( tokens[0] == "altObs" )
               altObs = tokens[1].ToDouble();
            else if ( tokens[0] == "version" )
               version = tokens[1];
      }

      m_valid = true;
   }
   catch ( ... )
   {
      Console().CriticalLn( "<end><cbr>*** Error: Corrupted integration metadata serialization." );
   }
}

// ----------------------------------------------------------------------------

String IntegrationMetadata::Serialize() const
{
   if ( !IsValid() )
      return String();

   return String()   <<                  "version"         << TokenSeparator << __PCL_INTEGRATION_METADATA_VERSION
                     << ItemSeparator << "author"          << TokenSeparator << author.ToString()
                     << ItemSeparator << "observer"        << TokenSeparator << observer.ToString()
                     << ItemSeparator << "instrumentName"  << TokenSeparator << instrumentName.ToString()
                     << ItemSeparator << "frameType"       << TokenSeparator << frameType.ToString()
                     << ItemSeparator << "filterName"      << TokenSeparator << filterName.ToString()
                     << ItemSeparator << "cfaPatternName"  << TokenSeparator << cfaPatternName.ToString()
                     << ItemSeparator << "cfaPattern"      << TokenSeparator << cfaPattern.ToString()
                     << ItemSeparator << "cfaXOffset"      << TokenSeparator << cfaXOffset.ToString()
                     << ItemSeparator << "cfaYOffset"      << TokenSeparator << cfaYOffset.ToString()
                     << ItemSeparator << "pedestal"        << TokenSeparator << pedestal.ToString()
                     << ItemSeparator << "expTime"         << TokenSeparator << expTime.ToString()
                     << ItemSeparator << "sensorTemp"      << TokenSeparator << sensorTemp.ToString()
                     << ItemSeparator << "xPixSize"        << TokenSeparator << xPixSize.ToString()
                     << ItemSeparator << "yPixSize"        << TokenSeparator << yPixSize.ToString()
                     << ItemSeparator << "cameraGain"      << TokenSeparator << cameraGain.ToString()
                     << ItemSeparator << "cameraISO"       << TokenSeparator << cameraISO.ToString()
                     << ItemSeparator << "xBinning"        << TokenSeparator << xBinning.ToString()
                     << ItemSeparator << "yBinning"        << TokenSeparator << yBinning.ToString()
                     << ItemSeparator << "xOrigin"         << TokenSeparator << xOrigin.ToString()
                     << ItemSeparator << "yOrigin"         << TokenSeparator << yOrigin.ToString()
                     << ItemSeparator << "telescopeName"   << TokenSeparator << telescopeName.ToString()
                     << ItemSeparator << "focalLength"     << TokenSeparator << focalLength.ToString()
                     << ItemSeparator << "aperture"        << TokenSeparator << aperture.ToString()
                     << ItemSeparator << "apertureArea"    << TokenSeparator << apertureArea.ToString()
                     << ItemSeparator << "objectName"      << TokenSeparator << objectName.ToString()
                     << ItemSeparator << "startTime"       << TokenSeparator << startTime.ToString()
                     << ItemSeparator << "endTime"         << TokenSeparator << endTime.ToString()
                     << ItemSeparator << "ra"              << TokenSeparator << ra.ToString()
                     << ItemSeparator << "dec"             << TokenSeparator << dec.ToString()
                     << ItemSeparator << "celCrdSys"       << TokenSeparator << celCrdSys.ToString()
                     << ItemSeparator << "equinox"         << TokenSeparator << equinox.ToString()
                     << ItemSeparator << "longObs"         << TokenSeparator << longObs.ToString()
                     << ItemSeparator << "latObs"          << TokenSeparator << latObs.ToString()
                     << ItemSeparator << "altObs"          << TokenSeparator << altObs.ToString();
}

// ----------------------------------------------------------------------------

void IntegrationMetadata::UpdatePropertiesAndKeywords( PropertyArray& properties, FITSKeywordArray& keywords ) const
{
   if ( !IsValid() )
      return;

   if ( author.IsConsistentlyDefined( "Observation:Authors (AUTHOR keyword)" ) )
   {
      properties << Property( "Observation:Authors", author() );
      keywords << FITSHeaderKeyword( "AUTHOR",
                                     author().SingleQuoted(),
                                     "Author of the data" );
   }

   if ( observer.IsConsistentlyDefined( "Observer:Name (OBSERVER keyword)" ) )
   {
      properties << Property( "Observer:Name", observer() );
      keywords << FITSHeaderKeyword( "OBSERVER",
                                     observer().SingleQuoted(),
                                     "Observer who acquired the data" );
   }

   if ( instrumentName.IsConsistentlyDefined( "Instrument:Camera:Name (INSTRUME keyword)" ) )
   {
      properties << Property( "Instrument:Camera:Name", instrumentName() );
      keywords << FITSHeaderKeyword( "INSTRUME",
                                     instrumentName().SingleQuoted(),
                                     "Name of instrument" );
   }

   if ( frameType.IsConsistentlyDefined( "image type (IMAGETYP keyword)" ) )
   {
      keywords << FITSHeaderKeyword( "IMAGETYP",
                                     frameType().SingleQuoted(),
                                     "Type of integrated image" );
   }

   if ( filterName.IsConsistentlyDefined( "Instrument:Filter:Name (FILTER keyword)" ) )
   {
      properties << Property( "Instrument:Filter:Name", filterName() );
      keywords << FITSHeaderKeyword( "FILTER",
                                     filterName().SingleQuoted(),
                                     "Name of filter" );
   }

   if ( cfaPatternName.IsConsistentlyDefined( "PCL:CFASourcePatternName" ) )
      properties << Property( "PCL:CFASourcePatternName", cfaPatternName() );

   if ( cfaPattern.IsConsistentlyDefined( "PCL:CFASourcePattern (BAYERPAT keyword)" ) )
   {
      if ( cfaPattern().Length() == 4 )
      {
         // For Bayer CFA patterns, a PCL:CFASourcePattern XISF property is
         // incompatible with XBAYROFF/YBAYROFF FITS keywords.
         if ( !cfaXOffset.IsDefined() )
            if ( !cfaYOffset.IsDefined() )
               properties << Property( "PCL:CFASourcePattern", cfaPattern() );

         keywords << FITSHeaderKeyword( "BAYERPAT",
                                        cfaPattern().SingleQuoted(),
                                        "Bayer CFA pattern" );
         if ( cfaXOffset.IsConsistentlyDefined( "XBAYROFF keyword" ) )
            keywords << FITSHeaderKeyword( "XBAYROFF",
                                           IsoString( cfaXOffset() ),
                                          "Bayer CFA X-offset" );
         if ( cfaYOffset.IsConsistentlyDefined( "YBAYROFF keyword" ) )
            keywords << FITSHeaderKeyword( "YBAYROFF",
                                           IsoString( cfaYOffset() ),
                                          "Bayer CFA Y-offset" );
      }
      else
      {
         // X-Trans CFA
         properties << Property( "PCL:CFASourcePattern", cfaPattern() );
      }
   }

   if ( pedestal.IsConsistentlyDefined( "PEDESTAL keyword" ) )
      if ( pedestal() > 0 )
         keywords << FITSHeaderKeyword( "PEDESTAL",
                                        IsoString().Format( "%.4f", pedestal() ),
                                        "Value in DN added to enforce positivity" );

   if ( xPixSize.IsConsistentlyDefined( "Instrument:Sensor:XPixelSize (XPIXSZ keyword)" ) )
   {
      properties << Property( "Instrument:Sensor:XPixelSize", Round( xPixSize(), 3 ) );
      keywords << FITSHeaderKeyword( "XPIXSZ",
                                     IsoString().Format( "%.6g", xPixSize() ),
                                     "Pixel size including binning, X-axis (um)" );
   }

   if ( yPixSize.IsConsistentlyDefined( "Instrument:Sensor:YPixelSize (YPIXSZ keyword)" ) )
   {
      properties << Property( "Instrument:Sensor:YPixelSize", Round( yPixSize(), 3 ) );
      keywords << FITSHeaderKeyword( "YPIXSZ",
                                     IsoString().Format( "%.6g", yPixSize() ),
                                     "Pixel size including binning, Y-axis (um)" );
   }

   if ( cameraGain.IsConsistentlyDefined( "Instrument:Camera:Gain (EGAIN keyword)" ) )
   {
      properties << Property( "Instrument:Camera:Gain", Round( cameraGain(), 3 ) );
      keywords << FITSHeaderKeyword( "EGAIN",
                                     IsoString().Format( "%.6g", cameraGain() ),
                                     "Camera gain in electrons per data number" );
   }

   if ( cameraISO.IsConsistentlyDefined( "Instrument:Camera:ISOSpeed (ISOSPEED keyword)" ) )
   {
      properties << Property( "Instrument:Camera:ISOSpeed", cameraISO() );
      keywords << FITSHeaderKeyword( "ISOSPEED",
                                     IsoString( cameraISO() ),
                                     "Camera sensitivity in ISO speed units" );
   }

   if ( xBinning.IsConsistentlyDefined( "Instrument:Camera:XBinning (XBINNING keyword)" ) )
   {
      properties << Property( "Instrument:Camera:XBinning", xBinning() );
      keywords << FITSHeaderKeyword( "XBINNING",
                                     IsoString( xBinning() ),
                                     "Pixel binning factor, X-axis" );
   }

   if ( yBinning.IsConsistentlyDefined( "Instrument:Camera:YBinning (YBINNING keyword)" ) )
   {
      properties << Property( "Instrument:Camera:YBinning", yBinning() );
      keywords << FITSHeaderKeyword( "YBINNING",
                                     IsoString( yBinning() ),
                                     "Pixel binning factor, Y-axis" );
   }

   if ( xOrigin.IsConsistentlyDefined( "subframe x-origin (XORGSUBF keyword)" ) )
      keywords << FITSHeaderKeyword( "XORGSUBF",
                                     IsoString( xOrigin() ),
                                     "Subframe origin, X-axis (px)" );

   if ( yOrigin.IsConsistentlyDefined( "subframe y-origin (YORGSUBF keyword)" ) )
      keywords << FITSHeaderKeyword( "YORGSUBF",
                                     IsoString( yOrigin() ),
                                     "Subframe origin, Y-axis (px)" );

   if ( telescopeName.IsConsistentlyDefined( "Instrument:Telescope:Name (TELESCOP keyword)" ) )
   {
      properties << Property( "Instrument:Telescope:Name", telescopeName() );
      keywords << FITSHeaderKeyword( "TELESCOP",
                                     telescopeName().SingleQuoted(),
                                     "Name of telescope" );
   }

   if ( focalLength.IsConsistentlyDefined( "Instrument:Telescope:FocalLength (FOCALLEN keyword)" ) )
   {
      properties << Property( "Instrument:Telescope:FocalLength", focalLength()/1000 );
      keywords << FITSHeaderKeyword( "FOCALLEN",
                                     IsoString().Format( "%.8g", focalLength() ),
                                     "Effective focal length (mm)" );
   }

   if ( aperture.IsConsistentlyDefined( "Instrument:Telescope:Aperture (APTDIA keyword)" ) )
   {
      properties << Property( "Instrument:Telescope:Aperture", aperture()/1000 );
      keywords << FITSHeaderKeyword( "APTDIA",
                                     IsoString().Format( "%.8g", aperture() ),
                                     "Effective aperture diameter (mm)" );
   }

   if ( apertureArea.IsConsistentlyDefined( "Instrument:Telescope:CollectingArea (APTAREA keyword)" ) )
   {
      properties << Property( "Instrument:Telescope:CollectingArea", apertureArea()/1000/1000 );
      keywords << FITSHeaderKeyword( "APTAREA",
                                     IsoString().Format( "%.8g", apertureArea() ),
                                     "Effective aperture area (mm**2)" );
   }

   if ( objectName.IsConsistentlyDefined( "Observation:Object:Name (OBJECT keyword)" ) )
   {
      properties << Property( "Observation:Object:Name", objectName() );
      keywords << FITSHeaderKeyword( "OBJECT",
                                     objectName().SingleQuoted(),
                                     "Name of observed object" );
   }

   if ( ra.IsConsistentlyDefined( "Observation:Center:RA (RA / OBJCTRA keyword)" ) )
   {
      double ra = this->ra();
      if ( ra < 0 )
         ra += 360;
      properties << Property( "Observation:Center:RA", ra );
      keywords << FITSHeaderKeyword( "RA",
                                     IsoString().Format( "%.16g", ra ),
                                     "Right ascension of the center of the image (deg)" );
      SexagesimalConversionOptions options( 3/*items*/, 3/*precision*/, false/*sign*/, 0/*width*/, ' '/*separator*/ );
      keywords << FITSHeaderKeyword( "OBJCTRA",
                                     IsoString::ToSexagesimal( ra/15, options ).SingleQuoted(),
                                     "Right ascension (hours) (compatibility)" );
   }

   if ( dec.IsConsistentlyDefined( "Observation:Center:Dec (DEC / OBJCTDEC keyword)" ) )
   {
      properties << Property( "Observation:Center:Dec", dec() );
      keywords << FITSHeaderKeyword( "DEC",
                                     IsoString().Format( "%.16g", dec() ),
                                     "Declination of the center of the image (deg)" );
      SexagesimalConversionOptions options( 3/*items*/, 2/*precision*/, true/*sign*/, 0/*width*/, ' '/*separator*/ );
      keywords << FITSHeaderKeyword( "OBJCTDEC",
                                     IsoString::ToSexagesimal( dec(), options ).SingleQuoted(),
                                     "Declination (deg) (compatibility)" );
   }

   if ( celCrdSys.IsConsistentlyDefined( "Observation:CelestialReferenceSystem (RADESYS keyword)" ) )
   {
      properties << Property( "Observation:CelestialReferenceSystem", celCrdSys() );
      keywords << FITSHeaderKeyword( "RADESYS",
                                     celCrdSys(),
                                     "Reference system of celestial coordinates" );
   }

   if ( equinox.IsConsistentlyDefined( "Observation:Equinox (EQUINOX keyword)" ) )
   {
      properties << Property( "Observation:Equinox", equinox() );
      keywords << FITSHeaderKeyword( "EQUINOX",
                                     IsoString( equinox() ),
                                     "Epoch of the mean equator and equinox (years)" );
   }

   if ( startTime.IsConsistentlyDefined( "Observation:Time:Start (DATE-OBS / DATE-BEG keywords)" ) )
   {
      properties << Property( "Observation:Time:Start", startTime() );
      keywords << FITSHeaderKeyword( "DATE-OBS",
                                     startTime().ToIsoString( 3/*timeItems*/, 3/*precision*/, 0/*tz*/, false/*timeZone*/ ).SingleQuoted(),
                                     "Date/time of start of observation (UTC)" );
   }

   if ( endTime.IsConsistentlyDefined( "Observation:Time:End (DATE-END / DATE-OBS + EXPTIME keywords)" ) )
   {
      properties << Property( "Observation:Time:End", endTime() );
      keywords << FITSHeaderKeyword( "DATE-END",
                                     endTime().ToIsoString( 3/*timeItems*/, 3/*precision*/, 0/*tz*/, false/*timeZone*/ ).SingleQuoted(),
                                     "Date/time of end of observation (UTC)" );
   }

   if ( longObs.IsConsistentlyDefined( "Observation:Location:Longitude (OBSGEO-L / LONG-OBS keyword)" ) )
   {
      properties << Property( "Observation:Location:Longitude", Round( longObs(), 6 ) );
      keywords << FITSHeaderKeyword( "OBSGEO-L",
                                     IsoString().Format( "%.10g", longObs() ),
                                     "Geodetic longitude of observation location (deg)" );
      keywords << FITSHeaderKeyword( "LONG-OBS",
                                     IsoString().Format( "%.10g", longObs() ),
                                     "Geodetic longitude (deg) (compatibility)" );
   }

   if ( latObs.IsConsistentlyDefined( "Observation:Location:Latitude (OBSGEO-B / LAT-OBS keyword)" ) )
   {
      properties << Property( "Observation:Location:Latitude", Round( latObs(), 6 ) );
      keywords << FITSHeaderKeyword( "OBSGEO-B",
                                     IsoString().Format( "%.10g", latObs() ),
                                     "Geodetic latitude of observation location (deg)" );
      keywords << FITSHeaderKeyword( "LAT-OBS",
                                     IsoString().Format( "%.10g", latObs() ),
                                     "Geodetic latitude (deg) (compatibility)" );
   }

   if ( altObs.IsConsistentlyDefined( "Observation:Location:Elevation (OBSGEO-H / ALT-OBS keyword)" ) )
   {
      properties << Property( "Observation:Location:Elevation", RoundInt( altObs() ) );
      keywords << FITSHeaderKeyword( "OBSGEO-H",
                                     IsoString().Format( "%.0f", altObs() ),
                                     "Geodetic height of observation location (m)" );
      keywords << FITSHeaderKeyword( "ALT-OBS",
                                     IsoString().Format( "%.0f", altObs() ),
                                     "Geodetic height (m) (compatibility)" );
   }
}

// ----------------------------------------------------------------------------

IntegrationMetadata IntegrationMetadata::Summary( const Array<IntegrationMetadata>& items )
{
   IntegrationMetadata summary;
   ConsistentlyDefined<TimePoint> startTime, endTime;
   int raTotal = 0, decTotal = 0, longObsTotal = 0, latObsTotal = 0, altObsTotal = 0;
   for ( const IntegrationMetadata& metadata : items )
      if ( metadata.IsValid() )
      {
         if ( summary.IsValid() )
         {
            summary.author           = metadata.author;
            summary.observer         = metadata.observer;
            summary.instrumentName   = metadata.instrumentName;
            summary.frameType        = metadata.frameType;
            summary.filterName       = metadata.filterName;
            summary.cfaPatternName   = metadata.cfaPatternName;
            summary.cfaPattern       = metadata.cfaPattern;
            summary.cfaXOffset       = metadata.cfaXOffset;
            summary.cfaYOffset       = metadata.cfaYOffset;
            summary.pedestal         = metadata.pedestal;
            summary.expTime         += metadata.expTime;
            summary.sensorTemp       = metadata.sensorTemp;
            summary.xPixSize         = metadata.xPixSize;
            summary.yPixSize         = metadata.yPixSize;
            summary.cameraGain       = metadata.cameraGain;
            summary.cameraISO        = metadata.cameraISO;
            summary.xBinning         = metadata.xBinning;
            summary.yBinning         = metadata.yBinning;
            summary.xOrigin          = metadata.xOrigin;
            summary.yOrigin          = metadata.yOrigin;
            summary.telescopeName    = metadata.telescopeName;
            summary.focalLength      = metadata.focalLength;
            summary.aperture         = metadata.aperture;
            summary.apertureArea     = metadata.apertureArea;
            summary.objectName       = metadata.objectName;
            summary.celCrdSys        = metadata.celCrdSys;
            summary.equinox          = metadata.equinox;

            if ( metadata.ra.IsDefined() )
               summary.ra += metadata.ra;

            if ( metadata.dec.IsDefined() )
               summary.dec += metadata.dec;

            if ( metadata.longObs.IsDefined() )
               summary.longObs += metadata.longObs;

            if ( metadata.latObs.IsDefined() )
               summary.latObs += metadata.latObs;

            if ( metadata.altObs.IsDefined() )
               summary.altObs += metadata.altObs;
         }
         else
            summary = metadata;

         if ( metadata.ra.IsDefined() )
            ++raTotal;

         if ( metadata.dec.IsDefined() )
            ++decTotal;

         if ( metadata.longObs.IsDefined() )
            ++longObsTotal;

         if ( metadata.latObs.IsDefined() )
            ++latObsTotal;

         if ( metadata.altObs.IsDefined() )
            ++altObsTotal;

         if ( metadata.startTime.IsDefined() && metadata.endTime.IsDefined() )
         {
            if ( startTime.IsDefined() )
            {
               if ( metadata.startTime() < startTime() )
                  startTime.ForceValue( metadata.startTime() );
            }
            else
               startTime = metadata.startTime();

            if ( endTime.IsDefined() )
            {
               if ( metadata.endTime() > endTime() )
                  endTime.ForceValue( metadata.endTime() );
            }
            else
               endTime = metadata.endTime();
         }
         else
         {
            startTime.SetInconsistent();
            endTime.SetInconsistent();
         }
      }
      else
      {
         if ( summary.IsValid() )
         {
            Console().WarningLn( "<end><cbr>** Warning: Corrupted or invalid integration metadata item(s)." );
            return IntegrationMetadata();
         }
      }

   if ( summary.IsValid() )
   {
      if ( summary.ra.IsDefined() )
         summary.ra.ForceValue( summary.ra() / raTotal );
      if ( summary.dec.IsDefined() )
         summary.dec.ForceValue( summary.dec() / decTotal );
      if ( summary.longObs.IsDefined() )
         summary.longObs.ForceValue( summary.longObs() / longObsTotal );
      if ( summary.latObs.IsDefined() )
         summary.latObs.ForceValue( summary.latObs() / latObsTotal );
      if ( summary.altObs.IsDefined() )
         summary.altObs.ForceValue( summary.altObs() / altObsTotal );
      if ( startTime.IsDefined() && endTime.IsDefined() )
         if ( startTime.IsConsistent() && endTime.IsConsistent() )
         {
            summary.startTime = startTime();
            summary.endTime = endTime();
         }
         else
         {
            summary.startTime.SetInconsistent();
            summary.endTime.SetInconsistent();
         }
   }

   return summary;
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/IntegrationMetadata.cpp - Released 2020-12-17T15:46:35Z
