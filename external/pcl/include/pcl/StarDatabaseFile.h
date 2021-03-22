//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/StarDatabaseFile.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_StarDatabaseFile_h
#define __PCL_StarDatabaseFile_h

/// \file pcl/StarDatabaseFile.h

#include <pcl/Defs.h>

#include <pcl/AutoPointer.h>
#include <pcl/Compression.h>
#include <pcl/File.h>
#include <pcl/TimePoint.h>
#include <pcl/Vector.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \defgroup point_source_databases Star Catalogs and Point Source Databases
 */

// ----------------------------------------------------------------------------

class PCL_CLASS StarDatabaseFile;

/*!
 * \class XPSD
 * \brief Base class of point source database implementations.
 *
 * This class defines a set of fundamental data structures, properties and
 * support routines associated with point source database files (XPSD format).
 *
 * \ingroup point_source_databases
 */
class PCL_CLASS XPSD
{
public:

   /*!
    * \struct pcl::XPSD::Metadata
    * \brief %Metadata items available in point source database files.
    *
    * This structure holds metadata items that can be stored in point source
    * database files (current XPSD format version 1.0). For an existing
    * database file, available metadata are extracted directly from %XML file
    * headers. Currently all items are optional, so all data members of this
    * structure can be empty strings.
    *
    * For generation of new XPSD files, the creationTime and creatorOS members
    * of this structure will be ignored, since the corresponding metadata items
    * will always be defined automatically by the StarDatabaseFile::Serialize()
    * routine. The specified creatorApplication member, if empty, will be
    * replaced in the same routine with a default value identifying the current
    * PCL version.
    *
    * \ingroup point_source_databases
    */
   struct Metadata
   {
      TimePoint creationTime;       //!< The date this file was created.
      String    creatorOS;          //!< The operating system on which this file was created.
      String    creatorApplication; //!< The software application or program that created this file.
      String    databaseIdentifier; //!< The unique identifier of the database this file belongs to.
      String    databaseVersion;    //!< The version of the database this file belongs to.
      String    title;              //!< A title that represents or identifies this XPSD file.
      String    briefDescription;   //!< A brief (single-line) description of this XPSD file.
      String    description;        //!< A full description of the data stored in this XPSD file.
      String    organizationName;   //!< The name of the organization responsible for this file.
      String    authors;            //!< The names of one or more persons or groups that have created the data in this file.
      String    copyright;          //!< Copyright information applicable to the data stored in this XPSD file.
   };

   /*!
    * \struct pcl::XPSD::Statistics
    * \brief Structural and statistical data about an XPSD database file.
    *
    * This structure provides information about the number of sources included
    * in an XPSD file, as well as critical data about its tree-based database
    * index structure.
    *
    * \ingroup point_source_databases
    */
   struct Statistics
   {
      uint64   totalSources = 0;      //!< The total number of sources included in this database.
      uint32   totalNodes = 0;        //!< Number of quadtree index nodes, including structural and leaf nodes.
      uint32   totalLeaves = 0;       //!< Number of quadtree index leaf nodes.
      float    medianLeafLength = 0;  //!< The median of quadtree leaf node lengths.
      uint32   minimumLeafLength = 0; //!< Minimum quadtree leaf node length.
      uint32   maximumLeafLength = 0; //!< Maximum quadtree leaf node length.
   };

   /*!
    * \struct SearchData
    * \brief Data items and parameters for catalog search operations.
    *
    * The StarData template parameter represents a catalog-specific structure
    * to hold the data associated with a point source extracted during a
    * database search operation.
    *
    * \ingroup point_source_databases
    */
   template <class StarData>
   struct SearchData
   {
      double          centerRA = 0;             //!< Field center right ascension coordinate in degrees (search parameter).
      double          centerDec = 0;            //!< Field center declination coordinate in degrees (search parameter).
      double          radius = 1;               //!< Field radius in degrees (search parameter).
      float           magnitudeLow = -1.5;      /*!< Low magnitude (search parameter). Only stars of magnitude greater
                                                     than or equal to this value will be included in the stars list. */
      float           magnitudeHigh = 26;       /*!< High magnitude (search parameter). Only stars of magnitude less
                                                     than or equal to this value will be included in the stars list. */
      uint32          sourceLimit = uint32_max; /*!< The search will not include more objects than this limit
                                                     in the stars list (search parameter). */
      uint32          requiredFlags = 0u;       /*!< Required flags (search parameter). If non-zero, only stars with
                                                     \e all of these flags set will be included in the stars list. */
      uint32          inclusionFlags = 0u;      /*!< Inclusion flags (search parameter). If non-zero, only stars with
                                                     \e any of these flags set will be included in the stars list. */
      uint32          exclusionFlags = 0u;      /*!< Exclusion flags (search parameter). Stars with \e any of these flags
                                                         set will \e not be included in the stars list. */

      Array<StarData> stars;                    //!< The list of stars found by the search operation (output data).
      uint32          excessCount = 0u;         /*!< When \a sourceLimit is exceeded, this is the number of
                                                     additional objects found but not included in the stars list (output data). */
      uint32          rejectCount = 0u;         /*!< Total number of rejected objects (output data). This refers to
                                                     point sources that have been tested for inclusion in the search
                                                     result, but have not matched the search criteria. */
      double          timeTotal = 0;            //!< Total search time in seconds (output data).
      double          timeIO = 0;               //!< Time consumed by I/O operations in seconds (output data).
      uint32          countIO = 0u;             //!< Total number of I/O operations performed (output data).
      double          timeUncompress = 0;       //!< Time consumed by data uncompression in seconds (output data).
      double          timeDecode = 0;           //!< Time consumed by data decoding in seconds (output data).

      /*!
       * Sets all search result data items to null values.
       */
      void ResetSearchResults()
      {
         stars.Clear();
         excessCount = rejectCount = 0u;
         timeTotal = timeIO = 0;
         countIO = 0u;
         timeUncompress = timeDecode = 0;
      }
   };

protected:

   struct ChildNodeData
   {
      // Zero-based quadtree child node positions in an index node array.
      uint32 nw; // top-left child node
      uint32 ne; // top-right child node
      uint32 sw; // bottom-left child node
      uint32 se; // bottom-right child node
   };

#ifdef _MSC_VER
   /*
    * Our favorite brain-damaged thing does not know how to implement bit
    * fields. Oh well...
    */
   struct LeafNodeData
   {
      uint64 blockOffsetAndLeafFlag;
      uint32 blockSize;
      uint32 compressedBlockSize;
   };
#else
   struct LeafNodeData
   {
      uint64 blockOffset : 63;    // position of source data block, byte offset
      bool   leafFlag    :  1;    // quadtree node type: 0=structural 1=leaf
      uint32 blockSize;           // size of point source data, in bytes
      uint32 compressedBlockSize; // size of compressed data, in bytes
   };
#endif

   /*
    * Quadtree index node (48 bytes).
    */
   struct IndexNode
   {
      // Projected coordinates of quadtree node rectangle.
      double x0; // left
      double y0; // top
      double x1; // right
      double y1; // bottom

      // Quadtree child node indexes or leaf node data.
      union { ChildNodeData child;
              LeafNodeData  leaf; } index;

      IndexNode()
      {
         static_assert( sizeof( *this ) == 48, "Invalid sizeof( XPSD::IndexNode )" );
         static_assert( sizeof( ChildNodeData ) == 16, "Invalid sizeof( XPSD::ChildNodeData )" );
         static_assert( sizeof( LeafNodeData ) == 16, "Invalid sizeof( XPSD::LeafNodeData )" );
         static_assert( sizeof( index ) == 16, "Invalid sizeof( XPSD::IndexNode::index )" );

         index.child.nw = index.child.ne = index.child.sw = index.child.se = 0;
      }

      bool IsLeaf() const
      {
#ifdef _MSC_VER
         return (index.leaf.blockOffsetAndLeafFlag & 0x8000000000000000) != 0;
#else
         return index.leaf.leafFlag;
#endif
      }

      uint64 BlockOffset() const
      {
#ifdef _MSC_VER
         return index.leaf.blockOffsetAndLeafFlag & 0x7FFFFFFFFFFFFFFF;
#else
         return index.leaf.blockOffset;
#endif
      }

      uint32 BlockSize() const
      {
         return index.leaf.blockSize;
      }

      uint32 CompressedBlockSize() const
      {
         return index.leaf.compressedBlockSize;
      }
   };

   static double Distance( double lon1, double lat1, double lon2, double lat2 )
   {
      return Vector::FromSpherical( Rad( lon1 ), Rad( lat1 ) ).Angle3D( Vector::FromSpherical( Rad( lon2 ), Rad( lat2 ) ) );
   }

   static double CrossTrackDistance( double lon, double lat, double lon1, double lat1, double lon2, double lat2 )
   {
      if ( lon == lon1 )
         if ( lat == lat1 )
            return 0;

      Vector p = Vector::FromSpherical( Rad( lon ), Rad( lat ) );
      Vector c = Vector::FromSpherical( Rad( lon1 ), Rad( lat1 ) ).Cross( Vector::FromSpherical( Rad( lon2 ), Rad( lat2 ) ) );
      return c.Angle3D( p ) - Pi()/2;
   }

   static bool WithinExtent( double lon, double lat, double lon1, double lat1, double lon2, double lat2 )
   {
      if ( lon1 == lon2 )
         if ( lat1 == lat2 )
            return lon == lon1 && lat == lat1; // null segment

      Vector n0 = Vector::FromSpherical( Rad( lon ), Rad( lat ) );
      Vector n1 = Vector::FromSpherical( Rad( lon1 ), Rad( lat1 ) );
      Vector n2 = Vector::FromSpherical( Rad( lon2 ), Rad( lat2 ) );

      // Get vectors representing p0->p1, p0->p2, p1->p2, p2->p1
      Vector d10 = n0 - n1, d12 = n2 - n1;
      Vector d20 = n0 - n2, d21 = n1 - n2;

      // Dot product d10*d12 tells us if p0 is on p2 side of p1, similarly for d20*d21
      if ( d10 * d12 >= 0 )
         if ( d20 * d21 >= 0 )
            return (n0 * n1) >= 0 && (n0 * n2) >= 0; // same hemisphere

      return false;
   }

   static bool InRegion( double lon, double lat,
                         double lon1, double lat1, double lon2, double lat2,
                         double lon3, double lat3, double lon4, double lat4 )
   {
      Vector p = Vector::FromSpherical( Rad( lon ), Rad( lat ) );
      Vector v1 = p - Vector::FromSpherical( Rad( lon1 ), Rad( lat1 ) );
      Vector v2 = p - Vector::FromSpherical( Rad( lon2 ), Rad( lat2 ) );
      Vector v3 = p - Vector::FromSpherical( Rad( lon3 ), Rad( lat3 ) );
      Vector v4 = p - Vector::FromSpherical( Rad( lon4 ), Rad( lat4 ) );
      return Abs( v1.Angle3D( v2, p ) + v2.Angle3D( v3, p ) + v3.Angle3D( v4, p ) + v4.Angle3D( v1, p ) ) > Pi();
   }

   enum projection_type { Equirectangular, TransverseEquirectangular, AzimuthalEquidistant };

   static String ProjectionToAttributeValue( int );
   static projection_type ProjectionFromAttributeValue( const String& );

   class IndexTree
   {
   public:

      IndexTree( StarDatabaseFile* parent,
                 projection_type projection, double centerRA, double centerDec,
                 const Array<IndexNode>& nodes )
         : m_parent( parent )
         , m_projection( projection )
         , m_centerRA( centerRA )
         , m_centerDec( centerDec )
         , m_nodes( nodes )
      {
      }

      IndexTree() = default;
      IndexTree( const IndexTree& ) = default;

      void Project( double& x, double& y, double ra, double dec ) const
      {
         switch( m_projection )
         {
         default: // ?!
         case Equirectangular:
            x = ra - m_centerRA;
            y = dec;
            break;
         case AzimuthalEquidistant:
            {
               double sa, ca;
               SinCos( Rad( ra ), sa, ca );
               double r = 90 - Abs( dec );
               x = r*sa;
               y = r*ca;
            }
            break;
         case TransverseEquirectangular:
            {
               double sa, ca;
               SinCos( Rad( ra ), sa, ca );
               double sd, cd;
               SinCos( Rad( Abs( dec ) ), sd, cd );
               x = Deg( ArcSin( cd*sa ) );
               y = Deg( ArcTan( sd, cd*ca ) ) - 90;
            }
            break;
         }
      }

      void Unproject( double& ra, double& dec, double x, double y ) const
      {
         switch( m_projection )
         {
         default: // ?!
         case Equirectangular:
            ra = x + m_centerRA;
            dec = y;
            break;
         case AzimuthalEquidistant:
            x = Rad( x );
            y = Rad( y );
            ra = Deg( ArcTan( x, y ) );
            if ( ra < 0 )
               ra += 360;
            dec = Deg( ArcSin( Cos( Sqrt( x*x + y*y ) ) ) );
            if ( m_centerDec < 0 )
               dec = -dec;
            break;
         case TransverseEquirectangular:
            {
               double sx, cx;
               SinCos( Rad( x ), sx, cx );
               double sy, cy;
               SinCos( Rad( y + 90 ), sy, cy );
               ra = Deg( ArcTan( sx, cx*cy ) );
               if ( ra < 0 )
                  ra += 360;
               dec = Deg( ArcSin( sy*cx ) );
               if ( m_centerDec < 0 )
                  dec = -dec;
            }
            break;
         }
      }

      void Search( double ra, double dec, double r, void* searchData ) const
      {
         SearchRecursive( 0, ra, dec, r, searchData );
      }

   private:

      StarDatabaseFile* m_parent = nullptr;
      projection_type   m_projection = Equirectangular;
      double            m_centerRA = 0;
      double            m_centerDec = 0;
      Array<IndexNode>  m_nodes;

      void GetNodeBounds( double& ra1, double& dec1, double& ra2, double& dec2,
                          double& ra3, double& dec3, double& ra4, double& dec4, const IndexNode& node ) const
      {
         Unproject( ra1, dec1, node.x0, node.y0 );
         Unproject( ra2, dec2, node.x1, node.y0 );
         Unproject( ra3, dec3, node.x1, node.y1 );
         Unproject( ra4, dec4, node.x0, node.y1 );
      }

      bool InNodeRegion( double ra, double dec, const IndexNode& node ) const
      {
         double ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4;
         GetNodeBounds( ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4, node );
         return InRegion( ra, dec, ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4 );
      }

      bool IntersectsNodeRegion( double ra, double dec, double r, const IndexNode& node ) const
      {
         double ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4;
         GetNodeBounds( ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4, node );
         double rr = Rad( r );
         return InRegion( ra, dec, ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4 )
             || Distance( ra, dec, ra1, dec1 ) < rr
             || Distance( ra, dec, ra2, dec2 ) < rr
             || Distance( ra, dec, ra3, dec3 ) < rr
             || Distance( ra, dec, ra4, dec4 ) < rr
             || WithinExtent( ra, dec, ra1, dec1, ra2, dec2 ) && CrossTrackDistance( ra, dec, ra1, dec1, ra2, dec2 ) < rr
             || WithinExtent( ra, dec, ra2, dec2, ra3, dec3 ) && CrossTrackDistance( ra, dec, ra2, dec2, ra3, dec3 ) < rr
             || WithinExtent( ra, dec, ra3, dec3, ra4, dec4 ) && CrossTrackDistance( ra, dec, ra3, dec3, ra4, dec4 ) < rr
             || WithinExtent( ra, dec, ra4, dec4, ra1, dec1 ) && CrossTrackDistance( ra, dec, ra4, dec4, ra1, dec1 ) < rr;
      }

      // Defined after StarDatabaseFile declaration.
      void SearchRecursive( uint32 nodeIndex, double ra, double dec, double r, void* searchData ) const;

      friend class StarDatabaseFile;
   };
};

// ----------------------------------------------------------------------------

/*!
 * \class StarDatabaseFile
 * \brief Point source and star catalog database files (XPSD format).
 *
 * This class implements fast access to point source data stored in XPSD files
 * (Extensible Point Source Database format). It also implements serialization
 * of new XPSD files from existing point source or star catalog data.
 *
 * On the PixInsight/PCL platform, the XPSD file format allows for fast and
 * efficient access to large star catalogs, such as Gaia (as of writing this
 * documentation the Gaia DR2 and EDR3 catalogs are available) or PPMXL. The
 * XPSD format allows for serialization of general purpose star catalogs, with
 * special emphasis on astrometric and photometric data.
 *
 * \ingroup point_source_databases
 */
class PCL_CLASS StarDatabaseFile : public XPSD
{
public:

   /*!
    * Default constructor.
    *
    * Constructs an invalid instance that cannot be used until initialized by
    * calling the Open() member function.
    */
   StarDatabaseFile() = default;

   /*!
    * Constructs a &StarDatabaseFile instance initialized from the specified
    * point source database file in XPSD format.
    *
    * In the event of errors or invalid data, this constructor will throw the
    * appropriate Error exception.
    */
   StarDatabaseFile( const String& filePath )
   {
      Open( filePath );
   }

   /*!
    * Move constructor.
    */
   StarDatabaseFile( StarDatabaseFile&& ) = default;

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   StarDatabaseFile& operator =( StarDatabaseFile&& ) = default;

   /*!
    * Deleted copy constructor. %StarDatabaseFile instances are unique, hence
    * cannot be copied.
    */
   StarDatabaseFile( const StarDatabaseFile& ) = delete;

   /*!
    * Deleted copy assignment operator. %StarDatabaseFile instances are unique,
    * hence cannot be copied.
    */
   StarDatabaseFile& operator =( const StarDatabaseFile& ) = delete;

   /*!
    * Virtual destructor.
    */
   virtual ~StarDatabaseFile() noexcept( false )
   {
   }

   /*!
    * Initializes this object to provide access to the specified point source
    * database file in XPSD format.
    *
    * This member function opens an existing file at the specified \a filePath,
    * loads and parses its XML header, and loads the file indexes ready for
    * fast access to point source data. The file will remain open until this
    * object is destroyed, or until a new call to this function is made.
    *
    * If a previous file was already opened by this instance, it will be closed
    * and all associated control and file indexing structures will be destroyed
    * and deallocated, before accessing the new file.
    */
   void Open( const String& filePath );

   /*!
    * Closes the point source database file represented by this object and
    * resets all internal structures to a default, uninitialized state.
    *
    * If a previous file was already opened by this instance, it will be closed
    * and all associated control and file indexing structures will be destroyed
    * and deallocated. If no file is currently open, calling this member has no
    * effect.
    */
   void Close();

   /*!
    * Returns true iff this object has an open database file and is ready for
    * point source data retrieval.
    */
   bool IsOpen() const
   {
      return m_file.IsOpen();
   }

   /*!
    * Returns the path of the point source database file represented by this
    * object. Returned file paths are always absolute, full file paths.
    */
   const String& FilePath() const
   {
      return m_file.FilePath();
   }

   /*!
    * Returns the low limiting magnitude of this database file. All contained
    * sources should have magnitudes greater than the value returned by this
    * function.
    */
   float MagnitudeLow() const
   {
      return m_magnitudeLow;
   }

   /*!
    * Returns the high limiting magnitude of this database file. All contained
    * sources should have magnitudes less than or equal to the value returned
    * by this function.
    */
   float MagnitudeHigh() const
   {
      return m_magnitudeHigh;
   }

   /*!
    * Returns a reference to the (immutable) set of metadata items available in
    * the point source database file loaded by this object.
    */
   const XPSD::Metadata& Metadata() const
   {
      return m_metadata;
   }

   /*!
    * Returns a reference to the (immutable) set of statistical and structural
    * information items available in the point source database file loaded by
    * this object.
    */
   const XPSD::Statistics& Statistics() const
   {
      return m_statistics;
   }

   /*!
    * Generates a file to store a point source database in XPSD format.
    *
    * \param filePath      Path to the file that will be generated in the local
    *                      filesystem. The file name should carry the '.xpsd'
    *                      suffix.
    *
    * \param metadata      Reference to an XPSD::Metadata structure with
    *                      optional metadata information that will be included
    *                      in the generated XPSD file.
    *
    * \param statistics    Reference to an XPSD::Statistics structure with
    *                      statistical and structural information about the
    *                      XPSD database, which will be included in the
    *                      generated XPSD file.
    *
    * \param magnitudeLow  Low limiting magnitude. All point sources serialized
    *                      in the \a data array should have magnitudes greater
    *                      than the value of this parameter.
    *
    * \param magnitudeHigh High limiting magnitude. All point sources
    *                      serialized in the \a data array should have
    *                      magnitudes less than or equal to the value of this
    *                      parameter.
    *
    * \param index         Array of quadtree index structures.
    *
    * \param data          Serialized point source data.
    *
    * \param compression   Pointer to a Compression object used to compress
    *                      point source data blocks (leaf node data), or
    *                      nullptr if no compression has been applied. If
    *                      specified, this object will be used exclusively to
    *                      gather information about the compression algorithm
    *                      and parameters used, \e not to compress any data.
    *
    * In the event of invalid, incongruent or malformed data, or if an I/O
    * error occurs, this function will throw an Error exception.
    *
    * \warning If a file already exists at the specified path, its previous
    * contents will be lost after calling this function.
    */
   static void Serialize( const String& filePath,
                          const XPSD::Metadata& metadata,
                          const XPSD::Statistics& statistics,
                          float magnitudeLow, float magnitudeHigh,
                          const Array<XPSD::IndexTree>& index,
                          const ByteArray& data,
                          const Compression* compression = nullptr );

protected:

   mutable File                     m_file;
           XPSD::Metadata           m_metadata;
           XPSD::Statistics         m_statistics;
           float                    m_magnitudeLow = 0;
           float                    m_magnitudeHigh = 0;
           Array<XPSD::IndexTree>   m_index;
           uint64                   m_dataPosition = 0;
           AutoPointer<Compression> m_compression;

   virtual void LoadData( void* block, uint64 offset, uint32 size, void* ) const
   {
      m_file.SetPosition( m_dataPosition + offset );
      m_file.Read( block, size );
   }

   virtual void Uncompress( ByteArray& block, uint32 uncompressedSize, void* ) const
   {
      if ( m_compression )
         block = m_compression->Uncompress( block, uncompressedSize );
   }

   virtual void GetEncodedData( const ByteArray&, const XPSD::IndexTree&, const XPSD::IndexNode&, void* ) const = 0;

   friend class XPSD::IndexTree;
};

// ----------------------------------------------------------------------------

inline void
XPSD::IndexTree::SearchRecursive( uint32 nodeIndex, double ra, double dec, double r, void* searchData ) const
{
   const IndexNode& node = m_nodes[nodeIndex];
   if ( IntersectsNodeRegion( ra, dec, r, node ) )
   {
      if ( node.IsLeaf() )
      {
         ByteArray block( size_type( node.CompressedBlockSize() ) );
         m_parent->LoadData( block.Begin(), node.BlockOffset(), node.CompressedBlockSize(), searchData );
         if ( node.CompressedBlockSize() != node.BlockSize() )
            m_parent->Uncompress( block, node.BlockSize(), searchData );
         m_parent->GetEncodedData( block, *this, node, searchData );
      }
      else
      {
         if ( node.index.child.nw != 0 )
            SearchRecursive( node.index.child.nw, ra, dec, r, searchData );
         if ( node.index.child.ne != 0 )
            SearchRecursive( node.index.child.ne, ra, dec, r, searchData );
         if ( node.index.child.sw != 0 )
            SearchRecursive( node.index.child.sw, ra, dec, r, searchData );
         if ( node.index.child.se != 0 )
            SearchRecursive( node.index.child.se, ra, dec, r, searchData );
      }
   }
}

// ----------------------------------------------------------------------------

} // pcl

#endif  // __PCL_StarDatabaseFile_h

// ----------------------------------------------------------------------------
// EOF pcl/StarDatabaseFile.h - Released 2020-12-17T15:46:29Z
