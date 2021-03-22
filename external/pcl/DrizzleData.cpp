//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/DrizzleData.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/AutoPointer.h>
#include <pcl/Compression.h>
#include <pcl/Console.h>
#include <pcl/DrizzleData.h>
#include <pcl/XML.h>

#include <errno.h>

namespace pcl
{

// ----------------------------------------------------------------------------

void DrizzleData::Clear()
{
   m_sourceFilePath = m_cfaSourceFilePath = m_cfaSourcePattern = m_alignTargetFilePath = String();
   m_referenceWidth = m_referenceHeight = -1;
   m_alignmentOrigin = 0.5;
   m_H = Matrix();
   m_S.Clear();
   m_Sinv.Clear();
   m_LP1.Clear();
   m_LD2.Clear();
   m_LP2.Clear();
   m_LD1.Clear();
   m_LW.Clear();
   m_Sx = m_Sy = m_Sxinv = m_Syinv = spline();
   ClearIntegrationData();
}

void DrizzleData::ClearIntegrationData()
{
   m_metadata.Clear();
   m_pedestal = 0;
   m_location = m_referenceLocation = m_scale = m_unitScale = m_weight = m_unitWeight = Vector();
   m_adaptiveCoordinates.Clear();
   m_adaptiveLocation.Clear();
   m_adaptiveScaleLow.Clear();
   m_adaptiveScaleHigh.Clear();
   m_adaptiveZeroOffsetLow.Clear();
   m_adaptiveZeroOffsetHigh.Clear();
   m_rejectionLowCount = m_rejectionHighCount = UI64Vector();
   m_rejectionMap.FreeData();
   m_rejectLowData = m_rejectHighData = rejection_data();
}

// ----------------------------------------------------------------------------

XMLDocument* DrizzleData::Serialize() const
{
   // Validate image registration data
   if ( m_sourceFilePath.IsEmpty() ||
        m_referenceWidth < 1 || m_referenceHeight < 1 ||
       !m_H.IsEmpty() && (m_H.Rows() != 3 || m_H.Columns() != 3) ||
        m_H.IsEmpty() && !m_S.IsValid() )
      throw Error( "Invalid or insufficient image registration data." );

   // Validate image integration data
   if ( m_location.Length() != m_referenceLocation.Length() ||
       !m_scale.IsEmpty() && m_location.Length() != m_scale.Length() ||
       !m_weight.IsEmpty() && m_location.Length() != m_weight.Length() ||
       !m_rejectionMap.IsEmpty() && m_location.Length() != m_rejectionMap.NumberOfChannels() )
      throw Error( "Invalid or insufficient image integration data." );

   AutoPointer<XMLDocument> xml = new XMLDocument;
   xml->SetXML( "1.0", "UTF-8" );
   *xml << new XMLComment( "\nPixInsight XML Drizzle Data Format - XDRZ version 1.0"
                           "\nCreated with PixInsight software - http://pixinsight.com/"
                           "\n" );

   XMLElement* root = new XMLElement( "xdrz", XMLAttributeList()
      << XMLAttribute( "version", "1.0" )
      << XMLAttribute( "xmlns", "http://www.pixinsight.com/xdrz" )
      << XMLAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" )
      << XMLAttribute( "xsi:schemaLocation", "http://www.pixinsight.com/xdrz http://pixinsight.com/xdrz/xdrz-1.0.xsd" ) );

   xml->SetRootElement( root );

   *(new XMLElement( *root, "CreationTime" )) << new XMLText( TimePoint::Now().ToString() );

   *(new XMLElement( *root, "SourceImage" )) << new XMLText( m_sourceFilePath );

   if ( !m_cfaSourceFilePath.IsEmpty() )
      *(new XMLElement( *root, "CFASourceImage", XMLAttributeList()
            << (m_cfaSourcePattern.IsEmpty() ? XMLAttribute() : XMLAttribute( "pattern", m_cfaSourcePattern )) )
       ) << new XMLText( m_cfaSourceFilePath );

   if ( !m_alignTargetFilePath.IsEmpty() )
      *(new XMLElement( *root, "AlignmentTargetImage" )) << new XMLText( m_alignTargetFilePath );

   new XMLElement( *root, "ReferenceGeometry", XMLAttributeList()
      << XMLAttribute( "width", String( m_referenceWidth ) )
      << XMLAttribute( "height", String( m_referenceHeight ) )
      << (m_location.IsEmpty() ? XMLAttribute() : XMLAttribute( "numberOfChannels", String( m_location.Length() ) )) );

   new XMLElement( *root, "AlignmentOrigin", XMLAttributeList()
                                 << XMLAttribute( "x", String( m_alignmentOrigin.x ) )
                                 << XMLAttribute( "y", String( m_alignmentOrigin.y ) ) );

   if ( !m_H.IsEmpty() )
      *(new XMLElement( *root, "AlignmentMatrix" )) << new XMLText( String().ToCommaSeparated( m_H ) );

   if ( m_S.IsValid() )
   {
      SerializeSpline( new XMLElement( *root, "AlignmentSplineX" ), m_S.m_Sx );
      SerializeSpline( new XMLElement( *root, "AlignmentSplineY" ), m_S.m_Sy );

      if ( m_Sinv.IsValid() )
      {
         SerializeSpline( new XMLElement( *root, "AlignmentInverseSplineX" ), m_Sinv.m_Sx );
         SerializeSpline( new XMLElement( *root, "AlignmentInverseSplineY" ), m_Sinv.m_Sy );
      }

      if ( !m_LP1.IsEmpty() && !m_LD2.IsEmpty() )
      {
         XMLElement* element = new XMLElement( *root, "LocalDistortionModel", XMLAttributeList()
                                 << XMLAttribute( "order", String( m_localDistortionOrder ) )
                                 << XMLAttribute( "regularization", String( m_localDistortionRegularization ) )
                                 << XMLAttribute( "extrapolation", String( m_localDistortionExtrapolation ) ) );
         SerializePoints( new XMLElement( *element, "ReferencePoints" ), m_LP1 );
         SerializePoints( new XMLElement( *element, "TargetDisplacements" ), m_LD2 );
         if ( !m_LW.IsEmpty() )
            SerializeDistortionWeights( new XMLElement( *element, "PointWeights" ), m_LW );
         if ( !m_LP2.IsEmpty() && !m_LD1.IsEmpty() )
         {
            SerializePoints( new XMLElement( *element, "TargetPoints" ), m_LP2 );
            SerializePoints( new XMLElement( *element, "ReferenceDisplacements" ), m_LD1 );
         }
      }
   }

   if ( !m_metadata.IsEmpty() )
      *(new XMLElement( *root, "Metadata", XMLAttributeList()
                           << XMLAttribute( "encoding", "Base64" ) )) << new XMLText( IsoString::ToBase64( m_metadata ) );

   if ( m_pedestal > 0 )
      *(new XMLElement( *root, "Pedestal" )) << new XMLText( String( m_pedestal ) );

   if ( !m_location.IsEmpty() )
   {
      *(new XMLElement( *root, "LocationEstimates" )) << new XMLText( String().ToCommaSeparated( m_location ) );
      *(new XMLElement( *root, "ReferenceLocation" )) << new XMLText( String().ToCommaSeparated( m_referenceLocation ) );
      if ( !m_scale.IsEmpty() )
         *(new XMLElement( *root, "ScaleFactors" )) << new XMLText( String().ToCommaSeparated( m_scale ) );
      if ( !m_weight.IsEmpty() )
         *(new XMLElement( *root, "Weights" )) << new XMLText( String().ToCommaSeparated( m_weight ) );
      if ( !m_rejectionMap.IsEmpty() )
         SerializeRejectionMap( new XMLElement( *root, "RejectionMap" ) );
   }

   if ( HasAdaptiveNormalizationData() )
   {
      XMLElement* element = new XMLElement( *root, "AdaptiveNormalization" );
      Array<double> x, y;
      for ( const DPoint& p : m_adaptiveCoordinates )
      {
         x << p.x;
         y << p.y;
      }
      *(new XMLElement( *element, "XCoordinates" )) << new XMLText( String().ToCommaSeparated( x ) );
      *(new XMLElement( *element, "YCoordinates" )) << new XMLText( String().ToCommaSeparated( y ) );
      StringList list;
      for ( const Vector& v : m_adaptiveLocation )
         list << String().ToCommaSeparated( v );
      *(new XMLElement( *element, "LocationEstimates" )) << new XMLText( String().ToSeparated( list, ';' ) );
      list.Clear();
      for ( const Vector& v : m_adaptiveScaleLow )
         list << String().ToCommaSeparated( v );
      *(new XMLElement( *element, "LowScaleFactors" )) << new XMLText( String().ToSeparated( list, ';' ) );
      list.Clear();
      for ( const Vector& v : m_adaptiveScaleHigh )
         list << String().ToCommaSeparated( v );
      *(new XMLElement( *element, "HighScaleFactors" )) << new XMLText( String().ToSeparated( list, ';' ) );
      list.Clear();
      for ( const Vector& v : m_adaptiveZeroOffsetLow )
         list << String().ToCommaSeparated( v );
      *(new XMLElement( *element, "LowZeroOffsetCoefficients" )) << new XMLText( String().ToSeparated( list, ';' ) );
      list.Clear();
      for ( const Vector& v : m_adaptiveZeroOffsetHigh )
         list << String().ToCommaSeparated( v );
      *(new XMLElement( *element, "HighZeroOffsetCoefficients" )) << new XMLText( String().ToSeparated( list, ';' ) );
   }

   return xml.Release();
}

// ----------------------------------------------------------------------------

void DrizzleData::SerializeToFile( const String& path ) const
{
   AutoPointer<XMLDocument> xml = Serialize();
   xml->EnableAutoFormatting();
   xml->SetIndentSize( 3 );
   xml->SerializeToFile( path );
}

// ----------------------------------------------------------------------------

static void WarnOnUnexpectedChildNode( const XMLNode& node, const String& parsingWhatElement )
{
   if ( !node.IsComment() )
   {
      XMLParseError e( node,
            "Parsing " + parsingWhatElement + " element",
            "Ignoring unexpected XML child node of " + XMLNodeType::AsString( node.NodeType() ) + " type." );
      Console().WarningLn( "<end><cbr>** Warning: " + e.Message() );
   }
}

static void WarnOnUnknownChildElement( const XMLElement& element, const String& parsingWhatElement )
{
   XMLParseError e( element,
         "Parsing " + parsingWhatElement + " element",
         "Skipping unknown \'" + element.Name() + "\' child element." );
   Console().WarningLn( "<end><cbr>** Warning: " + e.Message() );
}

// ----------------------------------------------------------------------------

static bool TryToInt( int& value, IsoString::const_iterator p )
{
   IsoString::iterator endptr = nullptr;
   errno = 0;
   long val = ::strtol( p, &endptr, 0 );
   if ( errno == 0 && (endptr == nullptr || *endptr == '\0') )
   {
      value = int( val );
      return true;
   }
   return false;
}

static bool TryToDouble( double& value, IsoString::const_iterator p )
{
   IsoString::iterator endptr = nullptr;
   errno = 0;
   double val = ::strtod( p, &endptr );
   if ( errno == 0 && (endptr == nullptr || *endptr == '\0') )
   {
      value = val;
      return true;
   }
   return false;
}

static Vector ParseListOfRealValues( IsoString& text, size_type start, size_type end, size_type minCount = 0, size_type maxCount = ~size_type( 0 ) )
{
   Array<double> v;
   for ( size_type i = start, j; i < end; ++i )
   {
      for ( j = i; j < end; ++j )
         if ( text[j] == ',' )
            break;
      text[j] = '\0';
      double x;
      if ( !TryToDouble( x, text.At( i ) ) )
         throw Error( "Parsing real numeric list: Invalid floating point numeric literal \'" + IsoString( text.At( i ) ) + "\'" );
      if ( v.Length() == maxCount )
         throw Error( "Parsing real numeric list: Too many items." );
      v << x;
      i = j;
   }
   if ( v.Length() < minCount )
      throw Error( "Parsing real numeric list: Too few items." );
   return Vector( v.Begin(), int( v.Length() ) );
}

static Vector ParseListOfRealValues( const XMLElement& element, size_type minCount = 0, size_type maxCount = ~size_type( 0 ) )
{
   IsoString text = IsoString( element.Text().Trimmed() );
   return ParseListOfRealValues( text, 0, text.Length(), minCount, maxCount );
}

static MultiVector ParseListsOfRealValues( const XMLElement& element, size_type minCount = 0, size_type maxCount = ~size_type( 0 ) )
{
   MultiVector m;
   IsoString text = IsoString( element.Text().Trimmed() );
   IsoStringList lists;
   text.Break( lists, ';', true/*trim*/ );
   for ( IsoString& list : lists )
      if ( !list.IsEmpty() )
         m << ParseListOfRealValues( list, 0, list.Length(), minCount, maxCount );
   return m;
}

static IVector ParseListOfIntegerValues( IsoString& text, size_type start, size_type end, size_type minCount = 0, size_type maxCount = ~size_type( 0 ) )
{
   Array<int> v;
   for ( size_type i = start, j; i < end; ++i )
   {
      for ( j = i; j < end; ++j )
         if ( text[j] == ',' )
            break;
      text[j] = '\0';
      int x;
      if ( !TryToInt( x, text.At( i ) ) )
         throw Error( "Parsing integer numeric list: Invalid integer numeric literal \'" + IsoString( text.At( i ) ) + "\' at offset " + IsoString( start ) );
      if ( v.Length() == maxCount )
         throw Error( "Parsing integer numeric list: Too many items." );
      v << x;
      i = j;
   }
   if ( v.Length() < minCount )
      throw Error( "Parsing integer numeric list: Too few items." );
   return IVector( v.Begin(), int( v.Length() ) );
}

static double ParseRealValue( const IsoString& s, size_type start, size_type end )
{
   double x;
   if ( !s.Substring( start, end-start ).TryToDouble( x ) )
      throw Error( "Invalid floating point numeric literal \'" + s + "\' at offset " + String( start ) );
   return x;
}

static int ParseIntegerValue( const IsoString& s, size_type start, size_type end )
{
   int x;
   if ( !s.Substring( start, end-start ).TryToInt( x ) )
      throw Error( "Invalid integer numeric literal \'" + s + "\' at offset " + IsoString( start ) );
   return x;
}

// ----------------------------------------------------------------------------

template <typename T>
static GenericVector<T> ParseBase64EncodedVector( const XMLElement& element, size_type minCount = 0, size_type maxCount = ~size_type( 0 ) )
{
   ByteArray data = IsoString( element.Text().Trimmed() ).FromBase64();
   if ( data.IsEmpty() )
      throw Error( "Missing encoded vector data in " + element.Name() + " element." );
   if ( data.Size() % sizeof( T ) != 0 )
      throw Error( "Invalid size of encoded vector data in " + element.Name() + " element." );
   size_type n = data.Size()/sizeof( T );
   if ( n < minCount )
      throw Error( "Too few vector components in " + element.Name() + " element." );
   if ( n > maxCount )
      throw Error( "Too many vector components in " + element.Name() + " element." );
   return GenericVector<T>( reinterpret_cast<const T*>( data.Begin() ), int( n ) );
}

// ----------------------------------------------------------------------------

void DrizzleData::Parse( const String& filePath, bool ignoreIntegrationData )
{
   IsoString text = File::ReadTextFile( filePath );
   for ( auto ch : text )
   {
      if ( ch == '<' )
      {
         XMLDocument xml;
         xml.SetParserOption( XMLParserOption::IgnoreComments );
         xml.SetParserOption( XMLParserOption::IgnoreUnknownElements );
         xml.Parse( text.UTF8ToUTF16() );
         Parse( xml, ignoreIntegrationData );
         return;
      }

      if ( !IsoCharTraits::IsSpace( ch ) )
      {
         Clear();
         PlainTextDecoder( this, ignoreIntegrationData ).Decode( text );

         // Build rejection map from rejection coordinate lists.
         if ( !m_rejectHighData.IsEmpty() || !m_rejectLowData.IsEmpty() )
         {
            m_rejectionMap.AllocateData( m_referenceWidth, m_referenceHeight, NumberOfChannels() );
            m_rejectionMap.Zero();

            if ( !m_rejectHighData.IsEmpty() )
            {
               for ( int c = 0; c < NumberOfChannels(); ++c )
                  for ( const Point& p : m_rejectHighData[c] )
                     m_rejectionMap( p, c ) = uint8( 1 );
               m_rejectHighData.Clear();
            }

            if ( !m_rejectLowData.IsEmpty() )
            {
               for ( int c = 0; c < NumberOfChannels(); ++c )
                  for ( const Point& p : m_rejectLowData[c] )
                     m_rejectionMap( p, c ) |= uint8( 2 );
               m_rejectLowData.Clear();
            }
         }

         return;
      }
   }

   throw Error( "Empty drizzle data file." );
}

// ----------------------------------------------------------------------------

void DrizzleData::Parse( const XMLDocument& xml, bool ignoreIntegrationData )
{
   if ( xml.RootElement() == nullptr )
      throw Error( "The XML document has no root element." );
   if ( xml.RootElement()->Name() != "xdrz" || xml.RootElement()->AttributeValue( "version" ) != "1.0" )
      throw Error( "Not an XDRZ version 1.0 document." );
   Parse( *xml.RootElement(), ignoreIntegrationData );
}

// ----------------------------------------------------------------------------

void DrizzleData::Parse( const XMLElement& root, bool ignoreIntegrationData )
{
   Clear();

   for ( const XMLNode& node : root )
   {
      if ( !node.IsElement() )
      {
         WarnOnUnexpectedChildNode( node, "xdrz root" );
         continue;
      }

      const XMLElement& element = static_cast<const XMLElement&>( node );

      try
      {
         if ( element.Name() == "SourceImage" )
         {
            m_sourceFilePath = element.Text().Trimmed();
            if ( m_sourceFilePath.IsEmpty() )
               throw Error( "Empty source file path definition." );
         }
         else if ( element.Name() == "CFASourceImage" )
         {
            // optional
            m_cfaSourceFilePath = element.Text().Trimmed();
            m_cfaSourcePattern = element.AttributeValue( "pattern" );
         }
         else if ( element.Name() == "AlignmentTargetImage" )
         {
            // optional
            m_alignTargetFilePath = element.Text().Trimmed();
         }
         else if ( element.Name() == "ReferenceGeometry" )
         {
            String width = element.AttributeValue( "width" );
            String height = element.AttributeValue( "height" );
            if ( width.IsEmpty() || height.IsEmpty() )
               throw Error( "Missing reference dimension attribute(s)." );
            m_referenceWidth = width.ToInt();
            m_referenceHeight = height.ToInt();
            if ( m_referenceWidth < 1 || m_referenceHeight < 1 )
               throw Error( "Invalid reference dimension(s)." );
         }
         else if ( element.Name() == "AlignmentOrigin" )
         {
            String x = element.AttributeValue( "x" );
            String y = element.AttributeValue( "y" );
            if ( x.IsEmpty() || y.IsEmpty() )
               throw Error( "Missing alignment origin attribute(s)." );
            m_alignmentOrigin.x = x.ToDouble();
            m_alignmentOrigin.y = y.ToDouble();
         }
         else if ( element.Name() == "AlignmentMatrix" )
         {
            Vector v = ParseListOfRealValues( element, 9, 9 );
            m_H = Matrix( v.Begin(), 3, 3 );
         }
         else if ( element.Name() == "AlignmentSplineX" )
         {
            ParseSpline( m_Sx, element );
         }
         else if ( element.Name() == "AlignmentSplineY" )
         {
            ParseSpline( m_Sy, element );
         }
         else if ( element.Name() == "AlignmentInverseSplineX" )
         {
            ParseSpline( m_Sxinv, element );
         }
         else if ( element.Name() == "AlignmentInverseSplineY" )
         {
            ParseSpline( m_Syinv, element );
         }
         else if ( element.Name() == "LocalDistortionModel" )
         {
            String s = element.AttributeValue( "order" );
            if ( !s.IsEmpty() )
            {
               m_localDistortionOrder = s.ToInt();
               if ( m_localDistortionOrder < 2 || m_localDistortionOrder > 6 )
                  throw Error( "Invalid local distortion derivative order."  );
            }

            s = element.AttributeValue( "regularization" );
            if ( !s.IsEmpty() )
            {
               m_localDistortionRegularization = s.ToFloat();
               if ( m_localDistortionRegularization < 0 )
                  throw Error( "Invalid local distortion regularization factor."  );
            }

            s = element.AttributeValue( "extrapolation" );
            if ( !s.IsEmpty() )
               m_localDistortionExtrapolation = s.ToBool();

            for ( const XMLNode& node : element )
            {
               if ( !node.IsElement() )
               {
                  WarnOnUnexpectedChildNode( node, "LocalDistortionModel" );
                  continue;
               }
               const XMLElement& element = static_cast<const XMLElement&>( node );
               if ( element.Name() == "ReferencePoints" )
                  m_LP1 = ParsePoints( element );
               else if ( element.Name() == "TargetDisplacements" )
                  m_LD2 = ParsePoints( element );
               else if ( element.Name() == "PointWeights" )
                  m_LW = ParseDistortionWeights( element );
               else if ( element.Name() == "TargetPoints" )
                  m_LP2 = ParsePoints( element );
               else if ( element.Name() == "ReferenceDisplacements" )
                  m_LD1 = ParsePoints( element );
               else
                  WarnOnUnknownChildElement( element, "LocalDistortionModel" );
            }

            if ( m_LP1.IsEmpty() || m_LD2.IsEmpty() )
               throw Error( "Missing or incomplete local distortion model data." );
            if ( m_LP1.Length() < 3 || m_LD2.Length() < 3 || !m_LP2.IsEmpty() && (m_LP2.Length() < 3 || m_LD1.Length() < 3) )
               throw Error( "Insufficient local distortion point data."  );
            if ( !m_LW.IsEmpty() && m_LW.Length() < m_LP1.Length() )
               throw Error( "Insufficient local distortion weight data."  );
            if ( m_LP1.Length() != m_LD2.Length() || m_LP2.Length() != m_LD1.Length() )
               throw Error( "Incongruent local distortion data."  );
         }
         else if ( element.Name() == "Metadata" )
         {
            if ( !ignoreIntegrationData )
            {
               String encoding = element.AttributeValue( "encoding" );
               if ( encoding.IsEmpty() )
                  m_metadata = element.Text().Trimmed();
               else
               {
                  if ( encoding.CaseFolded() != "base64" )
                     throw Error( "Invalid metadata encoding attribute value: Expected Base64, got '" + encoding + "'." );
                  ByteArray data = IsoString( element.Text().Trimmed() ).FromBase64();
                  m_metadata = String( reinterpret_cast<const char16_type*>( data.Begin() ),
                                       reinterpret_cast<const char16_type*>( data.End() ) );
               }
            }
         }
         else if ( element.Name() == "Pedestal" )
         {
            if ( !ignoreIntegrationData )
            {
               String pedestal = element.Text().Trimmed();
               m_pedestal = pedestal.ToDouble();
               if ( m_pedestal < 0 || m_pedestal >= 1 )
                  throw Error( "Pedestal value out of range: '" + pedestal + "'." );
            }
         }
         else if ( element.Name() == "LocationEstimates" )
         {
            if ( !ignoreIntegrationData )
               m_location = ParseListOfRealValues( element, 1 );
         }
         else if ( element.Name() == "ReferenceLocation" )
         {
            if ( !ignoreIntegrationData )
               m_referenceLocation = ParseListOfRealValues( element, 1 );
         }
         else if ( element.Name() == "ScaleFactors" )
         {
            if ( !ignoreIntegrationData )
               m_scale = ParseListOfRealValues( element, 1 );
         }
         else if ( element.Name() == "Weights" )
         {
            if ( !ignoreIntegrationData )
               m_weight = ParseListOfRealValues( element, 1 );
         }
         else if ( element.Name() == "RejectionMap" )
         {
            if ( !ignoreIntegrationData )
               ParseRejectionMap( element );
         }
         else if ( element.Name() == "AdaptiveNormalization" )
         {
            if ( !ignoreIntegrationData )
            {
               DVector x, y;
               for ( const XMLNode& node : element )
               {
                  if ( !node.IsElement() )
                  {
                     WarnOnUnexpectedChildNode( node, "AdaptiveNormalization" );
                     continue;
                  }
                  const XMLElement& element = static_cast<const XMLElement&>( node );
                  if ( element.Name() == "XCoordinates" )
                     x = ParseListOfRealValues( element, 1 );
                  else if ( element.Name() == "YCoordinates" )
                     y = ParseListOfRealValues( element, 1 );
                  else if ( element.Name() == "LocationEstimates" )
                     m_adaptiveLocation = ParseListsOfRealValues( element, 1 );
                  else if ( element.Name() == "LowScaleFactors" )
                     m_adaptiveScaleLow = ParseListsOfRealValues( element, 1 );
                  else if ( element.Name() == "HighScaleFactors" )
                     m_adaptiveScaleHigh = ParseListsOfRealValues( element, 1 );
                  else if ( element.Name() == "LowZeroOffsetCoefficients" )
                     m_adaptiveZeroOffsetLow = ParseListsOfRealValues( element, 1 );
                  else if ( element.Name() == "HighZeroOffsetCoefficients" )
                     m_adaptiveZeroOffsetHigh = ParseListsOfRealValues( element, 1 );
                  else
                     WarnOnUnknownChildElement( element, "AdaptiveNormalization" );
               }

               if ( x.Length() < 3 || x.Length() != y.Length() )
                  throw Error( "Missing or incongruent adaptive normalization coordinates." );
               if ( m_adaptiveLocation.IsEmpty() )
                  throw Error( "Missing adaptive normalization location estimates." );
               if ( m_adaptiveScaleLow.IsEmpty() )
                  throw Error( "Missing low adaptive normalization scale factors." );
               if ( m_adaptiveScaleHigh.IsEmpty() )
                  throw Error( "Missing high adaptive normalization scale factors." );
               if ( m_adaptiveZeroOffsetLow.IsEmpty() )
                  throw Error( "Missing low adaptive normalization zero offset coefficients." );
               if ( m_adaptiveZeroOffsetHigh.IsEmpty() )
                  throw Error( "Missing high adaptive normalization zero offset coefficients." );

               if (  m_adaptiveLocation.Length() != m_adaptiveScaleLow.Length()
                  || m_adaptiveLocation.Length() != m_adaptiveScaleHigh.Length()
                  || m_adaptiveLocation.Length() != m_adaptiveZeroOffsetHigh.Length()
                  || m_adaptiveLocation.Length() != m_adaptiveZeroOffsetHigh.Length() )
                  throw Error( "Incongruent adaptive normalization data." );
               for ( size_type i = 0; i < m_adaptiveLocation.Length(); ++i )
                  if (  m_adaptiveLocation[i].Length() != x.Length()
                     || m_adaptiveScaleLow[i].Length() != x.Length()
                     || m_adaptiveScaleHigh[i].Length() != x.Length()
                     || m_adaptiveZeroOffsetLow[i].Length() != x.Length()
                     || m_adaptiveZeroOffsetHigh[i].Length() != x.Length() )
                     throw Error( "Invalid adaptive normalization vector lengths." );

               for ( int i = 0; i < x.Length(); ++i )
                  m_adaptiveCoordinates << DPoint( x[i], y[i] );
            }
         }
         else if ( element.Name() == "CreationTime" )
         {
            m_creationTime = TimePoint( element.Text().Trimmed() );
         }
         else
         {
            WarnOnUnknownChildElement( element, "xdrz root" );
         }
      }
      catch ( Exception& x )
      {
         try
         {
            throw XMLParseError( element, "Parsing " + element.Name() + " element", x.Message() );
         }
         catch ( Exception& x )
         {
            x.Show();
         }
      }
      catch ( ... )
      {
         throw;
      }
   }

   if ( m_sourceFilePath.IsEmpty() )
      throw Error( "Missing required SourceImage element." );

   if ( m_referenceWidth < 1 || m_referenceHeight < 1 )
      throw Error( "Missing required ReferenceGeometry element." );

   if ( m_H.IsEmpty() && !m_Sx.IsValid() )
      throw Error( "Missing required AlignmentMatrix or AlignmentSplineX/AlignmentSplineY element(s)." );

   if ( m_Sx.IsValid() != m_Sy.IsValid() )
      throw Error( "Missing required AlignmentSplineX/AlignmentSplineY element." );

   if ( m_Sxinv.IsValid() != m_Syinv.IsValid() )
      throw Error( "Missing required inverse AlignmentSplineX/AlignmentSplineY element." );

   if ( m_Sxinv.IsValid() && !m_Sx.IsValid() )
      throw Error( "Missing required AlignmentSplineX and AlignmentSplineY elements." );

   if ( !ignoreIntegrationData )
   {
      if ( m_location.IsEmpty() )
         throw Error( "Missing required LocationEstimates element." );

      if ( m_referenceLocation.IsEmpty() )
         throw Error( "Missing required ReferenceLocation element." );

      if ( m_location.Length() != m_referenceLocation.Length() )
         throw Error( "Incongruent reference location vector definition." );

      if ( !m_scale.IsEmpty() )
         if ( m_location.Length() != m_scale.Length() )
            throw Error( "Incongruent scale factors vector definition." );

      if ( !m_weight.IsEmpty() )
         if ( m_location.Length() != m_weight.Length() )
            throw Error( "Incongruent image weights vector definition." );

      if ( !m_rejectionMap.IsEmpty() )
      {
         if ( m_location.Length() != m_rejectionMap.NumberOfChannels() )
            throw Error( "Incongruent pixel rejection map definition." );
         m_rejectionHighCount = UI64Vector( uint64( 0 ), m_location.Length() );
         m_rejectionLowCount = UI64Vector( uint64( 0 ), m_location.Length() );
         for ( UInt8Image::const_pixel_iterator i( m_rejectionMap ); i; ++i )
            for ( int j = 0; j < m_location.Length(); ++j )
            {
               if ( i[j] & 1 )
                  ++m_rejectionHighCount[j];
               if ( i[j] & 2 )
                  ++m_rejectionLowCount[j];
            }
      }

      if ( !m_adaptiveLocation.IsEmpty() )
         if ( int( m_adaptiveLocation.Length() ) != m_location.Length() )
            throw Error( "Incongruent adaptive normalization data." );
   }

   if ( m_Sx.IsValid() )
   {
      m_S.m_Sx = m_Sx;
      m_S.m_Sy = m_Sy;
      m_Sx.Clear();
      m_Sy.Clear();

      if ( m_Sxinv.IsValid() )
      {
         m_Sinv.m_Sx = m_Sxinv;
         m_Sinv.m_Sy = m_Syinv;
         m_Sxinv.Clear();
         m_Syinv.Clear();
      }
   }
}

// ----------------------------------------------------------------------------

void DrizzleData::ParseRejectionMap( const XMLElement& root )
{
   String s = root.AttributeValue( "width" );
   if ( s.IsEmpty() )
      throw Error( "Missing rejection map width attribute." );
   int width = s.ToInt();
   if ( width < 1 )
      throw Error( "Invalid rejection map width attribute value '" + s + '\'' );

   s = root.AttributeValue( "height" );
   if ( s.IsEmpty() )
      throw Error( "Missing rejection map height attribute." );
   int height = s.ToInt();
   if ( height < 1 )
      throw Error( "Invalid rejection map height attribute value '" + s + '\'' );

   s = root.AttributeValue( "numberOfChannels" );
   if ( s.IsEmpty() )
      throw Error( "Missing rejection map numberOfChannels attribute." );
   int numberOfChannels = s.ToInt();
   if ( numberOfChannels < 1 )
      throw Error( "Invalid rejection map numberOfChannels attribute value '" + s + '\'' );

   m_rejectionMap.AllocateData( width, height, numberOfChannels );

   int channel = 0;

   for ( const XMLNode& node : root )
   {
      if ( !node.IsElement() )
      {
         WarnOnUnexpectedChildNode( node, "RejectionMap" );
         continue;
      }

      const XMLElement& element = static_cast<const XMLElement&>( node );

      if ( element.Name() == "ChannelData" )
      {
         if ( channel == numberOfChannels )
            throw Error( "Unexpected ChannelData child element - all rejection map channels are already defined." );

         ByteArray channelData = ParseMaybeCompressedData( element );
         if ( channelData.Size() != m_rejectionMap.ChannelSize() )
            throw Error( "Parsing xdrz RejectionMap ChannelData element: Invalid channel data size: "
               "Expected " + String( m_rejectionMap.ChannelSize() ) + " bytes, "
               "got " + String( channelData.Size() ) + " bytes." );

         ::memcpy( m_rejectionMap[channel], channelData.Begin(), channelData.Size() );

         ++channel;
      }
      else
      {
         WarnOnUnknownChildElement( element, "RejectionMap" );
      }
   }

   if ( channel < numberOfChannels )
      throw Error( "Missing rejection map channel data." );
}

// ----------------------------------------------------------------------------

void DrizzleData::SerializeRejectionMap( XMLElement* root ) const
{
   root->SetAttribute( "width", String( m_rejectionMap.Width() ) );
   root->SetAttribute( "height", String( m_rejectionMap.Height() ) );
   root->SetAttribute( "numberOfChannels", String( m_rejectionMap.NumberOfChannels() ) );

   for ( int c = 0; c < m_rejectionMap.NumberOfChannels(); ++c )
      SerializeMaybeCompressedData( new XMLElement( *root, "ChannelData" ),
                                    m_rejectionMap[c], m_rejectionMap.ChannelSize() );
}

// ----------------------------------------------------------------------------

void DrizzleData::ParseSpline( DrizzleData::spline& S, const XMLElement& root )
{
   // Scaling factor for normalization of node coordinates
   String s = root.AttributeValue( "scalingFactor" );
   if ( s.IsEmpty() )
      throw Error( "Missing surface spline scalingFactor attribute." );
   S.m_r0 = s.ToDouble();
   if ( S.m_r0 <= 0 )
      throw Error( "Invalid surface spline scaling factor '" + s + '\'' );

   // Zero offset for normalization of X node coordinates
   s = root.AttributeValue( "zeroOffsetX" );
   if ( s.IsEmpty() )
      throw Error( "Missing surface spline zeroOffsetX attribute." );
   S.m_x0 = s.ToDouble();

   // Zero offset for normalization of Y node coordinates
   s = root.AttributeValue( "zeroOffsetY" );
   if ( s.IsEmpty() )
      throw Error( "Missing surface spline zeroOffsetY attribute." );
   S.m_y0 = s.ToDouble();

   // Derivative order > 0
   s = root.AttributeValue( "order" );
   if ( s.IsEmpty() )
      throw Error( "Missing surface spline order attribute." );
   S.m_order = s.ToInt();
   if ( S.m_order < 1 )
      throw Error( "Invalid surface spline derivative order '" + s + '\'' );

   // Smoothing factor, or interpolating 2-D spline if m_smoothing == 0
   s = root.AttributeValue( "smoothing" );
   if ( !s.IsEmpty() )
   {
      S.m_smoothing = s.ToFloat();
      if ( S.m_smoothing < 0 )
         throw Error( "Invalid surface spline smoothing factor '" + s + '\'' );
   }
   else
      S.m_smoothing = 0;

   S.m_x.Clear();
   S.m_y.Clear();
   S.m_weights.Clear();
   S.m_spline.Clear();

   for ( const XMLNode& node : root )
   {
      if ( !node.IsElement() )
      {
         WarnOnUnexpectedChildNode( node, "AlignmentSplineX/AlignmentSplineY" );
         continue;
      }

      const XMLElement& element = static_cast<const XMLElement&>( node );

      if ( element.Name() == "NodeXCoordinates" )
         S.m_x = ParseBase64EncodedVector<vector_spline::spline::scalar>( element, 3 );
      else if ( element.Name() == "NodeYCoordinates" )
         S.m_y = ParseBase64EncodedVector<vector_spline::spline::scalar>( element, 3 );
      else if ( element.Name() == "Coefficients" )
         S.m_spline = ParseBase64EncodedVector<vector_spline::spline::scalar>( element, 3 );
      else if ( element.Name() == "NodeWeights" )
         S.m_weights = ParseBase64EncodedVector<FVector::scalar>( element, 3 );
      else
         WarnOnUnknownChildElement( element, "AlignmentSplineX/AlignmentSplineY" );
   }

   if ( S.m_x.Length() < 3 )
      throw Error( "Missing surface spline NodeXCoordinates child element." );
   if ( S.m_y.Length() < 3 )
      throw Error( "Missing surface spline NodeYCoordinates child element." );
   if ( S.m_spline.Length() < 3 )
      throw Error( "Missing surface spline Coefficients child element." );

   if ( S.m_x.Length() != S.m_y.Length() ||
       !S.m_weights.IsEmpty() && S.m_weights.Length() != S.m_x.Length() ||
        S.m_spline.Length() != S.m_x.Length() + ((S.m_order*(S.m_order + 1)) >> 1) )
   {
      throw Error( "Invalid surface spline definition." );
   }
}

// ----------------------------------------------------------------------------

void DrizzleData::SerializeSpline( XMLElement* root, const DrizzleData::spline& S )
{
   root->SetAttribute( "scalingFactor", String( S.m_r0 ) );
   root->SetAttribute( "zeroOffsetX", String( S.m_x0 ) );
   root->SetAttribute( "zeroOffsetY", String( S.m_y0 ) );
   root->SetAttribute( "order", String( S.m_order ) );
   *(new XMLElement( *root, "NodeXCoordinates" )) << new XMLText( IsoString::ToBase64( S.m_x ) );
   *(new XMLElement( *root, "NodeYCoordinates" )) << new XMLText( IsoString::ToBase64( S.m_y ) );
   *(new XMLElement( *root, "Coefficients" ))     << new XMLText( IsoString::ToBase64( S.m_spline ) );
   if ( S.m_smoothing > 0 )
   {
      root->SetAttribute( "smoothing", String( S.m_smoothing ) );
      if ( !S.m_weights.IsEmpty() )
         (*new XMLElement( *root, "NodeWeights" )) << new XMLText( IsoString::ToBase64( S.m_weights ) );
   }
}

// ----------------------------------------------------------------------------

DrizzleData::point_list DrizzleData::ParsePoints( const XMLElement& root )
{
   ByteArray pointData = ParseMaybeCompressedData( root );
   if ( pointData.Size() % sizeof( point_list::item_type ) )
      throw Error( "Parsing points list from " + root.Name() + " element: Invalid data length." );

   point_list D( pointData.Size()/sizeof( point_list::item_type ) );
   ::memcpy( D.Begin(), pointData.Begin(), pointData.Size() );
   return D;
}

// ----------------------------------------------------------------------------

void DrizzleData::SerializePoints( XMLElement* root, const point_list& points ) const
{
   SerializeMaybeCompressedData( root, points.Begin(), points.Size(), sizeof( point_list::item_type::component ) );
}

// ----------------------------------------------------------------------------

DrizzleData::weight_vector DrizzleData::ParseDistortionWeights( const XMLElement& root )
{
   ByteArray weightData = ParseMaybeCompressedData( root );
   if ( weightData.Size() % sizeof( weight_vector::item_type ) )
      throw Error( "Parsing distortion weights vector from " + root.Name() + " element: Invalid data length." );

   weight_vector W( weightData.Size()/sizeof( weight_vector::item_type ) );
   ::memcpy( W.Begin(), weightData.Begin(), weightData.Size() );
   return W;
}

// ----------------------------------------------------------------------------

void DrizzleData::SerializeDistortionWeights( XMLElement* root, const weight_vector& weights ) const
{
   SerializeMaybeCompressedData( root, weights.Begin(), weights.Size(), sizeof( weight_vector::item_type ) );
}

// ----------------------------------------------------------------------------

ByteArray DrizzleData::ParseMaybeCompressedData( const XMLElement& root )
{
   String algorithmName = root.AttributeValue( "compression" ).CaseFolded();
   if ( algorithmName.IsEmpty() )
      return IsoString( root.Text().Trimmed() ).FromBase64();

   AutoPointer<Compression> compression;
   if ( algorithmName == "lz4" || algorithmName == "lz4+sh" )
      compression = new LZ4Compression;
   else if ( algorithmName == "lz4hc" || algorithmName == "lz4hc+sh" )
      compression = new LZ4HCCompression;
   else if ( algorithmName == "zlib" || algorithmName == "zlib+sh" )
      compression = new ZLibCompression;
   else
      throw Error( "Unknown or unsupported compression codec '" + algorithmName + '\'' );

   if ( algorithmName.EndsWith( "+sh" ) )
   {
      String itemSize = root.AttributeValue( "itemSize" ).CaseFolded();
      if ( !itemSize.IsEmpty() )
      {
         compression->SetItemSize( itemSize.ToUInt() );
         compression->EnableByteShuffling();
      }
   }

   Compression::subblock_list subblocks;

   for ( const XMLNode& node : root )
   {
      if ( !node.IsElement() )
      {
         WarnOnUnexpectedChildNode( node, root.Name() );
         continue;
      }

      const XMLElement& element = static_cast<const XMLElement&>( node );

      if ( element.Name() == "Subblock" )
      {
         Compression::Subblock subblock;
         String size = element.AttributeValue( "uncompressedSize" );
         if ( size.IsEmpty() )
            throw Error( "Missing subblock uncompressedSize attribute." );
         subblock.uncompressedSize = size.ToUInt64();
         subblock.compressedData = IsoString( element.Text().Trimmed() ).FromBase64();
         subblocks << subblock;
      }
      else
      {
         WarnOnUnknownChildElement( root, root.Name() );
      }
   }

   if ( subblocks.IsEmpty() )
      throw Error( "Parsing xdrz " + root.Name() + " element: Missing Subblock child element(s)." );

   return compression->Uncompress( subblocks );
}

// ----------------------------------------------------------------------------

void DrizzleData::SerializeMaybeCompressedData( XMLElement* root, const void* data, size_type size, size_type itemSize ) const
{
   if ( m_compressionEnabled )
   {
      LZ4Compression compression;
      if ( itemSize > 1 )
      {
         compression.SetItemSize( itemSize );
         compression.EnableByteShuffling();
      }
      Compression::subblock_list subblocks = compression.Compress( data, size );
      if ( !subblocks.IsEmpty() )
      {
         root->SetAttribute( "compression", compression.AlgorithmName().CaseFolded() + ((itemSize > 1) ? "+sh" : "") );
         if ( itemSize > 1 )
            root->SetAttribute( "itemSize", String( itemSize ) );
         for ( const Compression::Subblock& subblock : subblocks )
         {
            XMLElement* subblockElement = new XMLElement( *root, "Subblock" );
            subblockElement->SetAttribute( "uncompressedSize", String( subblock.uncompressedSize ) );
            *subblockElement << new XMLText( IsoString::ToBase64( subblock.compressedData ) );
         }
         return;
      }
   }

   *root << new XMLText( IsoString::ToBase64( data, size ) );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

/*
 * Compatibility with the old .drz plain text format.
 */

void DrizzleData::PlainTextDecoder::Decode( IsoString& s, size_type start, size_type end )
{
   if ( end <= start )
      end = s.Length();
   IsoString itemId;
   size_type block = 0;
   size_type blockStart = 0;
   for ( size_type i = start; i < end; ++i )
      switch ( s[i] )
      {
      case '{':
         if ( block++ == 0 )
         {
            blockStart = i;
            itemId.Trim();
            if ( itemId.IsEmpty() )
               throw Error( "At offset=" + String( i ) + ": Missing item identifier." );
         }
         break;
      case '}':
         if ( block == 0 )
            throw Error( "At offset=" + String( i ) + ": Unexpected block termination." );
         if ( --block == 0 )
         {
            ProcessBlock( s, itemId, blockStart+1, i );
            itemId.Clear();
         }
         break;
      default:
         if ( block == 0 )
            itemId << s[i];
         break;
      }

   if ( block > 0 )
      throw Error( "At offset=" + String( blockStart ) + ": Unterminated block." );
   if ( !itemId.IsEmpty() )
      throw Error( "Uncompleted item definition \'" + itemId + '\'' );
}

// ----------------------------------------------------------------------------

void DrizzleData::PlainTextDecoder::ProcessBlock( IsoString& s, const IsoString& itemId, size_type start, size_type end )
{
   if ( itemId == "P" ) // drizzle source image
   {
      m_data->m_sourceFilePath = s.Substring( start, end-start ).Trimmed().UTF8ToUTF16();
      if ( m_data->m_sourceFilePath.IsEmpty() )
         throw Error( "At offset=" + String( start ) + ": Empty file path defined." );
   }
   else if ( itemId == "T" ) // alignment target image (optional)
   {
      m_data->m_alignTargetFilePath = s.Substring( start, end-start ).Trimmed().UTF8ToUTF16();
      if ( m_data->m_alignTargetFilePath.IsEmpty() )
         throw Error( "At offset=" + String( start ) + ": Empty file path defined." );
   }
   else if ( itemId == "D" ) // alignment reference image dimensions
   {
      IVector v = ParseListOfIntegerValues( s, start, end, 2, 2 );
      m_data->m_referenceWidth = v[0];
      m_data->m_referenceHeight = v[1];
      if ( m_data->m_referenceWidth < 1 || m_data->m_referenceHeight < 1 )
         throw Error( "At offset=" + String( start ) + ": Invalid reference dimensions." );
   }
   else if ( itemId == "H" ) // alignment matrix (projective)
   {
      Vector v = ParseListOfRealValues( s, start, end, 9, 9 );
      m_data->m_H = Matrix( v.Begin(), 3, 3 );
   }
   else if ( itemId == "Sx" ) // registration thin plates, X-axis
   {
      m_data->m_Sx = ParseSurfaceSpline( s, start, end );
   }
   else if ( itemId == "Sy" ) // registration thin plates, Y-axis
   {
      m_data->m_Sy = ParseSurfaceSpline( s, start, end );
   }
   else if ( itemId == "m" ) // location vector
   {
      if ( !m_ignoreIntegrationData )
         m_data->m_location = ParseListOfRealValues( s, start, end, 1 );
   }
   else if ( itemId == "m0" ) // reference location vector
   {
      if ( !m_ignoreIntegrationData )
         m_data->m_referenceLocation = ParseListOfRealValues( s, start, end, 1 );
   }
   else if ( itemId == "s" ) // scaling factors vector
   {
      if ( !m_ignoreIntegrationData )
         m_data->m_scale = ParseListOfRealValues( s, start, end, 1 );
   }
   else if ( itemId == "w" ) // image weights vector
   {
      if ( !m_ignoreIntegrationData )
         m_data->m_weight = ParseListOfRealValues( s, start, end, 1 );
   }
   else if ( itemId == "Rl" ) // rejection pixel coordinates, low values
   {
      if ( !m_ignoreIntegrationData )
         m_data->m_rejectLowData = ParseRejectionData( s, start, end );
   }
   else if ( itemId == "Rh" ) // rejection pixel coordinates, high values
   {
      if ( !m_ignoreIntegrationData )
         m_data->m_rejectHighData = ParseRejectionData( s, start, end );
   }
   else
      throw Error( "At offset=" + String( start ) + ": Unknown item identifier \'" + itemId + '\'' );
}

// ----------------------------------------------------------------------------

DrizzleData::rejection_coordinates
DrizzleData::PlainTextDecoder::ParseRejectionCoordinates( IsoString& s, size_type start, size_type end )
{
   IVector v = ParseListOfIntegerValues( s, start, end );
   if ( v.Length() & 1 )
      throw Error( "Parsing list from offset=" + String( start ) + ": Missing point coordinate(s)." );
   rejection_coordinates P;
   for ( int i = 0; i < v.Length(); i += 2 )
      P << Point( v[i], v[i+1] );
   return P;
}

// ----------------------------------------------------------------------------

DrizzleData::rejection_data
DrizzleData::PlainTextDecoder::ParseRejectionData( IsoString& s, size_type start, size_type end )
{
   rejection_data R;
   for ( size_type i = start; i < end; ++i )
      if ( s[i] == '{' )
      {
         size_type j = s.Find( '}', ++i );
         if ( j >= end )
            throw Error( "At offset=" + String( i ) + ": Unterminated block." );
         R << ParseRejectionCoordinates( s, i, j );
         i = j;
      }
      else if ( !IsoCharTraits::IsSpace( s[i] ) )
         throw Error( "At offset=" + String( i ) + ": Unexpected token \'" + s[i] + '\'' );
   return R;
}

// ----------------------------------------------------------------------------

DrizzleData::spline
DrizzleData::PlainTextDecoder::ParseSurfaceSpline( IsoString& text, size_type start, size_type end )
{
   spline S;
   PlainTextSplineDecoder( S ).Decode( text, start, end );
   return S;
}

// ----------------------------------------------------------------------------

void DrizzleData::PlainTextSplineDecoder::ProcessBlock( IsoString& s, const IsoString& itemId, size_type start, size_type end )
{
   if ( itemId == "x" )
      m_S.m_x = ParseListOfRealValues( s, start, end, 3 );
   else if ( itemId == "y" )
      m_S.m_y = ParseListOfRealValues( s, start, end, 3 );
   else if ( itemId == "r0" )
      m_S.m_r0 = ParseRealValue( s, start, end );
   else if ( itemId == "x0" )
      m_S.m_x0 = ParseRealValue( s, start, end );
   else if ( itemId == "y0" )
      m_S.m_y0 = ParseRealValue( s, start, end );
   else if ( itemId == "m" )
      m_S.m_order = ParseIntegerValue( s, start, end );
   else if ( itemId == "r" )
      m_S.m_smoothing = ParseRealValue( s, start, end );
   else if ( itemId == "w" )
      m_S.m_weights = ParseListOfRealValues( s, start, end );
   else if ( itemId == "s" )
      m_S.m_spline = ParseListOfRealValues( s, start, end, 3 );
   else
      throw Error( "At offset=" + String( start ) + ": Unknown item identifier \'" + itemId + '\'' );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/DrizzleData.cpp - Released 2020-12-17T15:46:35Z
