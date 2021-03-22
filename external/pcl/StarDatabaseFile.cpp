//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/StarDatabaseFile.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/Console.h>
#include <pcl/StarDatabaseFile.h>
#include <pcl/Version.h>
#include <pcl/XML.h>

namespace pcl
{

// ----------------------------------------------------------------------------

struct XPSDFileSignature
{
   uint8  magic[ 8 ]   = { 'X', 'P', 'S', 'D', '0', '1', '0', '0' };
   uint32 headerLength = 0;  // length in bytes of the XML file header
   uint32 reserved     = 0;  // reserved - must be zero

   XPSDFileSignature() = default;
   XPSDFileSignature( const XPSDFileSignature& ) = default;

   XPSDFileSignature( uint32 length )
      : headerLength( length )
   {
   }

   XPSDFileSignature& operator =( const XPSDFileSignature& ) = default;

   void Validate() const
   {
      if ( magic[0] != 'X' || magic[1] != 'P' || magic[2] != 'S' || magic[3] != 'D' )
         throw Error( "Not an XPSD file." );
      if ( magic[4] != '0' || magic[5] != '1' || magic[6] != '0' || magic[7] != '0' )
         throw Error( "Not an XPSD version 1.0 file." );
      if ( headerLength < 65 ) // minimum length of an empty XPSD header, from "<?xml..." to </xpsd>
         throw Error( "Invalid or corrupted XPSD file." );
   }
};

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

void StarDatabaseFile::Open( const String& filePath )
{
   Close();

   m_file.OpenForReading( filePath );

   XMLDocument xml;
   size_type minPos;
   {
      XPSDFileSignature signature;
      m_file.Read( signature );
      signature.Validate();

      minPos = signature.headerLength + sizeof( XPSDFileSignature );

      IsoString header;
      header.SetLength( signature.headerLength );
      m_file.Read( reinterpret_cast<void*>( header.Begin() ), signature.headerLength );

      xml.SetParserOption( XMLParserOption::IgnoreComments );
      xml.SetParserOption( XMLParserOption::IgnoreUnknownElements );
      xml.Parse( header.UTF8ToUTF16() );
   }

   if ( xml.RootElement()->Name() != "xpsd" || xml.RootElement()->AttributeValue( "version" ) != "1.0" )
      throw Error( "Not an XPSD version 1.0 file." );

   for ( const XMLNode& node : *xml.RootElement() )
   {
      if ( !node.IsElement() )
      {
         WarnOnUnexpectedChildNode( node, "xpsd root" );
         continue;
      }

//    <Data magnitudeRange="12.34,15.67" position="123456" compression="lz4hc+sh" itemSize="26"/>
//    <Tree projection="Equirectangular" center="45,0" rootPosition="123456" nodeCount="123456"/>
//    <Tree projection="Equirectangular" center="135,0" rootPosition="123456" nodeCount="123456"/>
//    ...
//    <Metadata>
//       ...
//    </Metadata>

      const XMLElement& element = static_cast<const XMLElement&>( node );

      try
      {
         if ( element.Name() == "Data" )
         {
            if ( m_dataPosition != 0 )
               throw Error( "Duplicate root Data element." );

            String attrValue = element.AttributeValue( "magnitudeRange" );
            if ( attrValue.IsEmpty() )
               throw Error( "Missing magnitudeRange attribute." );
            StringList tokens;
            attrValue.Break( tokens, ',' );
            if ( tokens.Length() != 2 )
               throw Error( "Invalid magnitudeRange attribute value." );
            m_magnitudeLow = tokens[0].ToFloat();
            m_magnitudeHigh = tokens[1].ToFloat();
            if ( m_magnitudeHigh <= m_magnitudeLow )
               throw Error( "Invalid magnitudeRange attribute value." );

            attrValue = element.AttributeValue( "position" );
            if ( attrValue.IsEmpty() )
               throw Error( "Missing position attribute." );
            m_dataPosition = attrValue.ToUInt64();
            if ( m_dataPosition < minPos )
               throw Error( "Wrong position attribute value." );

            attrValue = element.AttributeValue( "compression" ).CaseFolded();
            if ( !attrValue.IsEmpty() )
            {
               if ( attrValue == "lz4" || attrValue == "lz4+sh" )
                  m_compression = new LZ4Compression;
               else if ( attrValue == "lz4-hc" || attrValue == "lz4-hc+sh" )
                  m_compression = new LZ4HCCompression;
               else if ( attrValue == "zlib" || attrValue == "zlib+sh" )
                  m_compression = new ZLibCompression;
               else
                  throw Error( "Unknown or unsupported compression codec '" + attrValue + '\'' );

               if ( attrValue.EndsWith( "+sh" ) )
               {
                  attrValue = element.AttributeValue( "itemSize" );
                  if ( !attrValue.IsEmpty() )
                  {
                     m_compression->SetItemSize( attrValue.ToUInt() );
                     m_compression->EnableByteShuffling();
                  }
               }
            }
         }
         else if ( element.Name() == "Tree" )
         {
            String attrValue = element.AttributeValue( "projection" );
            if ( attrValue.IsEmpty() )
               throw Error( "Missing projection attribute." );
            projection_type projection = ProjectionFromAttributeValue( attrValue );

            attrValue = element.AttributeValue( "center" );
            if ( attrValue.IsEmpty() )
               throw Error( "Missing projection center attribute." );
            StringList tokens;
            attrValue.Break( tokens, ',' );
            if ( tokens.Length() != 2 )
               throw Error( "Invalid projection center attribute value." );
            double centerRA = tokens[0].ToDouble();
            double centerDec = tokens[1].ToDouble();
            if ( centerRA < 0 || centerRA >= 360 )
               throw Error( "Invalid projection center right ascension coordinate '" + tokens[0] + '\'' );
            if ( centerDec < -90 || centerDec > +90 )
               throw Error( "Invalid projection center declination coordinate '" + tokens[1] + '\'' );
            if ( (projection == TransverseEquirectangular || projection == AzimuthalEquidistant) && Abs( centerDec ) != 90
               || projection == Equirectangular && centerDec != 0 )
               throw Error( "Unsupported center declination coordinate for "
                           + ProjectionToAttributeValue( projection ) + " projection" );

            attrValue = element.AttributeValue( "rootPosition" );
            if ( attrValue.IsEmpty() )
               throw Error( "Missing rootPosition attribute." );
            uint64 rootPosition = attrValue.ToUInt64();
            if ( rootPosition < minPos )
               throw Error( "Wrong rootPosition attribute value." );

            attrValue = element.AttributeValue( "nodeCount" );
            if ( attrValue.IsEmpty() )
               throw Error( "Missing nodeCount attribute." );
            uint32 nodeCount = attrValue.ToUInt();

            Array<XPSD::IndexNode> nodes;
            m_file.SetPosition( rootPosition );
            for ( uint32 i = 0; i < nodeCount; ++i )
            {
               XPSD::IndexNode node;
               m_file.Read( node );
               nodes << node;
            }

            m_index << XPSD::IndexTree( this, projection, centerRA, centerDec, nodes );
         }
         else if ( element.Name() == "Metadata" )
         {
            for ( const XMLNode& node : element )
            {
               if ( !node.IsElement() )
               {
                  WarnOnUnexpectedChildNode( node, "Metadata" );
                  continue;
               }

               const XMLElement& element = static_cast<const XMLElement&>( node );
               String text = element.Text().Trimmed();
               if ( element.Name() == "CreationTime" )
                  m_metadata.creationTime = TimePoint( text );
               else if ( element.Name() == "CreatorOS" )
                  m_metadata.creatorOS = text;
               else if ( element.Name() == "CreatorApplication" )
                  m_metadata.creatorApplication = text;
               else if ( element.Name() == "DatabaseIdentifier" )
                  m_metadata.databaseIdentifier = text;
               else if ( element.Name() == "DatabaseVersion" )
                  m_metadata.databaseVersion = text;
               else if ( element.Name() == "Title" )
                  m_metadata.title = text;
               else if ( element.Name() == "BriefDescription" )
                  m_metadata.briefDescription = text;
               else if ( element.Name() == "Description" )
                  m_metadata.description = text;
               else if ( element.Name() == "OrganizationName" )
                  m_metadata.organizationName = text;
               else if ( element.Name() == "Authors" )
                  m_metadata.authors = text;
               else if ( element.Name() == "Copyright" )
                  m_metadata.copyright = text;
               else
                  WarnOnUnknownChildElement( element, "Metadata" );
            }
         }
         else if ( element.Name() == "Statistics" )
         {
            String attrValue = element.AttributeValue( "totalSources" );
            if ( !attrValue.IsEmpty() )
               m_statistics.totalSources = attrValue.ToUInt64();
            attrValue = element.AttributeValue( "totalNodes" );
            if ( !attrValue.IsEmpty() )
               m_statistics.totalNodes = attrValue.ToUInt();
            attrValue = element.AttributeValue( "totalLeaves" );
            if ( !attrValue.IsEmpty() )
               m_statistics.totalLeaves = attrValue.ToUInt();
            attrValue = element.AttributeValue( "medianLeafLength" );
            if ( !attrValue.IsEmpty() )
               m_statistics.medianLeafLength = attrValue.ToFloat();
            attrValue = element.AttributeValue( "minimumLeafLength" );
            if ( !attrValue.IsEmpty() )
               m_statistics.minimumLeafLength = attrValue.ToUInt();
            attrValue = element.AttributeValue( "maximumLeafLength" );
            if ( !attrValue.IsEmpty() )
               m_statistics.maximumLeafLength = attrValue.ToUInt();
         }
         else
         {
            WarnOnUnknownChildElement( element, "xpsd root" );
         }
      }
      catch ( Exception& x )
      {
         throw XMLParseError( element, "Parsing " + element.Name() + " element", x.Message() );
      }
      catch ( ... )
      {
         throw;
      }
   }

   if ( m_dataPosition == 0 )
      throw Error( "Missing mandatory Data element." );
   if ( m_index.IsEmpty() )
      throw Error( "Missing mandatory Tree element(s)." );
}

// ----------------------------------------------------------------------------

void StarDatabaseFile::Close()
{
   if ( IsOpen() )
   {
      m_file.Close();
      m_metadata = XPSD::Metadata();
      m_statistics = XPSD::Statistics();
      m_magnitudeLow = 0;
      m_magnitudeHigh = 0;
      m_index.Clear();
      m_dataPosition = 0;
      m_compression.Destroy();
   }
}

// ----------------------------------------------------------------------------

void StarDatabaseFile::Serialize( const String& filePath,
                                  const XPSD::Metadata& metadata,
                                  const XPSD::Statistics& statistics,
                                  float magnitudeLow, float magnitudeHigh,
                                  const Array<XPSD::IndexTree>& index,
                                  const ByteArray& data,
                                  const Compression* compression )
{
   // Validate data
   if ( filePath.IsEmpty() )
      throw Error( "Empty file path." );

   if ( magnitudeHigh < magnitudeLow )
      Swap( magnitudeLow, magnitudeHigh );
   if ( 1 + (magnitudeHigh - magnitudeLow) == 1 )
      throw Error( "Empty or insignificant magnitude range." );

   if ( index.IsEmpty() )
      throw Error( "Empty index." );
   if ( data.IsEmpty() )
      throw Error( "Empty point source data." );

   IsoString header;
   IsoString dataPosition;
   IsoStringList rootPositions;
   {
      XMLDocument xml;
      xml.SetXML( "1.0", "UTF-8" );
      xml << new XMLComment( "\nPixInsight Point Source Database Format - XPSD version 1.0"
                             "\nCreated with PixInsight software - http://pixinsight.com/"
                             "\n" );

      XMLElement* root = new XMLElement( "xpsd", XMLAttributeList()
         << XMLAttribute( "version", "1.0" )
         << XMLAttribute( "xmlns", "http://www.pixinsight.com/xpsd" )
         << XMLAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" )
         << XMLAttribute( "xsi:schemaLocation", "http://www.pixinsight.com/xpsd http://pixinsight.com/xpsd/xpsd-1.0.xsd" ) );

      xml.SetRootElement( root );

      XMLElement* metadataElement = new XMLElement( *root, "Metadata" );
      *(new XMLElement( *metadataElement, "CreationTime" )) << new XMLText( TimePoint::Now().ToString() );
      *(new XMLElement( *metadataElement, "CreatorOS" )) << new XMLText(
#ifdef __PCL_FREEBSD
                                                   "FreeBSD"
#endif
#ifdef __PCL_LINUX
                                                   "Linux"
#endif
#ifdef __PCL_MACOSX
                                                   "macOS"
#endif
#ifdef __PCL_WINDOWS
                                                   "Windows"
#endif
                                                   );

      *(new XMLElement( *metadataElement, "CreatorApplication" )) << new XMLText( metadata.creatorApplication.IsEmpty() ? pcl::Version::AsString() : metadata.creatorApplication );

      if ( !metadata.databaseIdentifier.IsEmpty() )
         *(new XMLElement( *metadataElement, "DatabaseIdentifier" )) << new XMLText( metadata.databaseIdentifier );
      if ( !metadata.databaseVersion.IsEmpty() )
         *(new XMLElement( *metadataElement, "DatabaseVersion" )) << new XMLText( metadata.databaseVersion );
      if ( !metadata.title.IsEmpty() )
         *(new XMLElement( *metadataElement, "Title" )) << new XMLText( metadata.title );
      if ( !metadata.briefDescription.IsEmpty() )
         *(new XMLElement( *metadataElement, "BriefDescription" )) << new XMLText( metadata.briefDescription );
      if ( !metadata.description.IsEmpty() )
         *(new XMLElement( *metadataElement, "Description" )) << new XMLText( metadata.description );
      if ( !metadata.organizationName.IsEmpty() )
         *(new XMLElement( *metadataElement, "OrganizationName" )) << new XMLText( metadata.organizationName );
      if ( !metadata.authors.IsEmpty() )
         *(new XMLElement( *metadataElement, "Authors" )) << new XMLText( metadata.authors );
      if ( !metadata.copyright.IsEmpty() )
         *(new XMLElement( *metadataElement, "Copyright" )) << new XMLText( metadata.copyright );

      if ( statistics.totalSources > 0 )
      {
         XMLElement* statisticsElement = new XMLElement( *root, "Statistics" );
         statisticsElement->SetAttribute( "totalSources", String().Format( "%llu", statistics.totalSources ) );
         if ( statistics.totalNodes > 0 )
            statisticsElement->SetAttribute( "totalNodes", String().Format( "%u", statistics.totalNodes ) );
         if ( statistics.totalLeaves > 0 )
            statisticsElement->SetAttribute( "totalLeaves", String().Format( "%u", statistics.totalLeaves ) );
         if ( statistics.medianLeafLength > 0 )
            statisticsElement->SetAttribute( "medianLeafLength", String().Format( "%.2f", statistics.medianLeafLength ) );
         if ( statistics.minimumLeafLength > 0 )
            statisticsElement->SetAttribute( "minimumLeafLength", String().Format( "%u", statistics.minimumLeafLength ) );
         if ( statistics.maximumLeafLength > 0 )
            statisticsElement->SetAttribute( "maximumLeafLength", String().Format( "%u", statistics.maximumLeafLength ) );
      }

      XMLElement* dataElement = new XMLElement( *root, "Data" );
      dataElement->SetAttribute( "magnitudeRange", String().Format( "%.2f,%.2f", magnitudeLow, magnitudeHigh ) );
      dataElement->SetAttribute( "position", dataPosition = IsoString::Random( 16 ) );

      if ( compression != nullptr )
      {
         dataElement->SetAttribute( "compression", compression->AlgorithmName().CaseFolded()
                                                + (compression->ByteShufflingEnabled() ? "+sh" : "") );
         if ( compression->ByteShufflingEnabled() )
            dataElement->SetAttribute( "itemSize", String( compression->ItemSize() ) );
      }

      for ( const XPSD::IndexTree& tree : index )
      {
         IsoString rootPosition = IsoString::Random( 16 );
         rootPositions << rootPosition;
         XMLElement* treeElement = new XMLElement( *root, "Tree" );
         treeElement->SetAttribute( "projection", ProjectionToAttributeValue( tree.m_projection ) );
         treeElement->SetAttribute( "center", String().Format( "%g,%g", tree.m_centerRA, tree.m_centerDec ) );
         treeElement->SetAttribute( "rootPosition", rootPosition );
         treeElement->SetAttribute( "nodeCount", String().Format( "%u", tree.m_nodes.Length() ) );
      }

      xml.EnableAutoFormatting();
      xml.SetIndentSize( 3 );
      header = xml.Serialize();
   }

   /*
    * Replace index position attributes. This is an iterative algorithm
    * resilient to changes in attribute value lengths.
    */
   for ( ;; )
   {
      size_type n = header.Length();
      size_type position = sizeof( XPSDFileSignature ) + n;
      int i = 0;
      for ( const XPSD::IndexTree& tree : index )
      {
         IsoString rootPosition = IsoString().Format( "%lld", position );
         header.ReplaceString( rootPositions[i].DoubleQuoted(), rootPosition.DoubleQuoted() );
         position += tree.m_nodes.Size();
         rootPositions[i++] = rootPosition;
      }
      IsoString newDataPosition = IsoString().Format( "%lld", position );
      header.ReplaceString( dataPosition.DoubleQuoted(), newDataPosition.DoubleQuoted() );
      dataPosition = newDataPosition;
      if ( header.Length() == n )
         break;
   }

   /*
    * Write the XPSD file
    */
   File file = File::CreateFileForWriting( filePath );

   // 1. XPSD signature
   file.Write( XPSDFileSignature( uint32( header.Length() ) ) );

   // 2. XPSD header
   file.Write( reinterpret_cast<const void*>( header.Begin() ), header.Length() );

   // 3. Index trees
   for ( const XPSD::IndexTree& tree : index )
      file.Write( reinterpret_cast<const void*>( tree.m_nodes.Begin() ), tree.m_nodes.Size() );

   // 4. Point source data
   file.Write( reinterpret_cast<const void*>( data.Begin() ), data.Size() );

   file.Close();
}

// ----------------------------------------------------------------------------

String XPSD::ProjectionToAttributeValue( int projection )
{
   switch ( projection )
   {
   case Equirectangular:           return "Equirectangular";
   case TransverseEquirectangular: return "TransverseEquirectangular";
   case AzimuthalEquidistant:      return "AzimuthalEquidistant";
   default:
      throw Error( String().Format( "Internal: Invalid or unsupported projection value \'%d\'", projection ) );
   }
}

XPSD::projection_type XPSD::ProjectionFromAttributeValue( const String& value )
{
   if ( value == "Equirectangular" )
      return Equirectangular;
   if ( value == "TransverseEquirectangular" )
      return TransverseEquirectangular;
   if ( value == "AzimuthalEquidistant" )
      return AzimuthalEquidistant;
   throw Error( "Invalid or unsupported projection identifier '" + value + '\'' );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/StarDatabaseFile.cpp - Released 2020-12-17T15:46:35Z
