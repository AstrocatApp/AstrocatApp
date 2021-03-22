//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/GaiaDatabaseFile.h - Released 2020-12-17T15:46:28Z
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

#ifndef __PCL_GaiaDatabaseFile_h
#define __PCL_GaiaDatabaseFile_h

/// \file pcl/GaiaDatabaseFile.h

#include <pcl/Defs.h>

#include <pcl/ElapsedTime.h>
#include <pcl/Flags.h>
#include <pcl/StarDatabaseFile.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \namespace pcl::GaiaStarFlag
 * \brief Data availability and quality flags for Gaia star data.
 *
 * <table border="1" cellpadding="4" cellspacing="0">
 * <tr><td>GaiaStarFlag::NoPM</td>             <td>No proper motions and parallax available.</td></tr>
 * <tr><td>GaiaStarFlag::NoGBPMag</td>         <td>No G-BP magnitude available.</td></tr>
 * <tr><td>GaiaStarFlag::NoGRPMag</td>         <td>No G-RP magnitude available.</td></tr>
 * <tr><td>GaiaStarFlag::LackingData</td>      <td>Equivalent to NoPM|NoGBPMag|NoGRPMag.</td></tr>

 * <tr><td>GaiaStarFlag::GoldRA</td>           <td>Standard error of right ascension &lt; 0.13 mas.</td></tr>
 * <tr><td>GaiaStarFlag::GoldDec</td>          <td>Standard error of declination &lt; 0.12 mas.</td></tr>
 * <tr><td>GaiaStarFlag::GoldParx</td>         <td>Standard error of parallax &lt; 0.13 mas.</td></tr>
 * <tr><td>GaiaStarFlag::GoldPMRA</td>         <td>Standard error of proper motion in right ascension &lt; 0.14 mas/year.</td></tr>
 * <tr><td>GaiaStarFlag::GoldPMDec</td>        <td>Standard error of proper motion in declination &lt; 0.12 mas/year.</td></tr>
 * <tr><td>GaiaStarFlag::GoldAstrometry</td>   <td>Equivalent to GoldRA|GoldDec|GoldParx|GoldPMRA|GoldPMDec.</td></tr>

 * <tr><td>GaiaStarFlag::SilverRA</td>         <td>Standard error of right ascension in the range [0.13,1.43) mas.</td></tr>
 * <tr><td>GaiaStarFlag::SilverDec</td>        <td>Standard error of declination in the range [0.12,1.28) mas.</td></tr>
 * <tr><td>GaiaStarFlag::SilverParx</td>       <td>Standard error of parallax in the range [0.13,0.86) mas.</td></tr>
 * <tr><td>GaiaStarFlag::SilverPMRA</td>       <td>Standard error of proper motion in right ascension in the range [0.14,0.97) mas/year.</td></tr>
 * <tr><td>GaiaStarFlag::SilverPMDec</td>      <td>Standard error of proper motion in declination in the range [0.12,0.85) mas/year.</td></tr>
 * <tr><td>GaiaStarFlag::SilverAstrometry</td> <td>Equivalent to SilverRA|SilverDec|SilverParx|SilverPMRA|SilverPMDec.</td></tr>

 * <tr><td>GaiaStarFlag::BronzeRA</td>         <td>Standard error of right ascension in the range [1.43,2.49) mas.</td></tr>
 * <tr><td>GaiaStarFlag::BronzeDec</td>        <td>Standard error of declination in the range [1.28,2.22) mas.</td></tr>
 * <tr><td>GaiaStarFlag::BronzeParx</td>       <td>Standard error of parallax in the range [0.86,1.38) mas.</td></tr>
 * <tr><td>GaiaStarFlag::BronzePMRA</td>       <td>Standard error of proper motion in right ascension in the range [0.97,1.58) mas/year.</td></tr>
 * <tr><td>GaiaStarFlag::BronzePMDec</td>      <td>Standard error of proper motion in declination in the range [0.85,1.38) mas/year.</td></tr>
 * <tr><td>GaiaStarFlag::BronzeAstrometry</td> <td>Equivalent to BronzeRA|BronzeDec|BronzeParx|BronzePMRA|BronzePMDec.</td></tr>

 * <tr><td>GaiaStarFlag::GoldGMag</td>         <td>Error on G-band mean flux < 0.84 e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::GoldGBPMag</td>       <td>Error on the integrated BP mean flux < 4.94 e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::GoldGRPMag</td>       <td>Error on the integrated RP mean flux < 5.89 e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::GoldPhotometry</td>   <td>Equivalent to GoldGMag|GoldGBPMag|GoldGRPMag.</td></tr>

 * <tr><td>GaiaStarFlag::SilverGMag</td>       <td>Error on G-band mean flux in the range [0.84,2.13) e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::SilverGBPMag</td>     <td>Error on the integrated BP mean flux in the range [4.94,12.61) e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::SilverGRPMag</td>     <td>Error on the integrated RP mean flux in the range [5.89,15.40) e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::SilverPhotometry</td> <td>Equivalent to SilverGMag|SilverGBPMag|SilverGRPMag.</td></tr>

 * <tr><td>GaiaStarFlag::BronzeGMag</td>       <td>Error on G-band mean flux in the range [2.13,3.08) e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::BronzeGBPMag</td>     <td>Error on the integrated BP mean flux in the range [12.61,18.04) e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::BronzeGRPMag</td>     <td>Error on the integrated RP mean flux in the range [15.40,22.35) e-/s.</td></tr>
 * <tr><td>GaiaStarFlag::BronzePhotometry</td> <td>Equivalent to BronzeGMag|BronzeGBPMag|BronzeGRPMag.</td></tr>

 * <tr><td>GaiaStarFlag::BPRPExcess</td>       <td>BP-RP excess factor &ge; 2.0</td></tr>
 * <tr><td>GaiaStarFlag::BPRPExcessHigh</td>   <td>BP-RP excess factor &ge; 5.0 (Gaia EDR3 only)</td></tr>

 * </table>
 *
 * The above data quality ranges correspond to the Gaia EDR3 XPSD database
 * version 1.0.0, released December 4, 2020.
 *
 * \ingroup point_source_databases
 */
namespace GaiaStarFlag
{
   enum mask_type
   {
      NoPM             = 0x00000001,
      NoGBPMag         = 0x00000002,
      NoGRPMag         = 0x00000004,
      LackingData      = 0x00000007,

      GoldRA           = 0x00000010,
      GoldDec          = 0x00000020,
      GoldPMRA         = 0x00000040,
      GoldPMDec        = 0x00000080,

      SilverRA         = 0x00000100,
      SilverDec        = 0x00000200,
      SilverPMRA       = 0x00000400,
      SilverPMDec      = 0x00000800,

      BronzeRA         = 0x00001000,
      BronzeDec        = 0x00002000,
      BronzePMRA       = 0x00004000,
      BronzePMDec      = 0x00008000,

      GoldGMag         = 0x00010000,
      GoldGBPMag       = 0x00020000,
      GoldGRPMag       = 0x00040000,
      GoldParx         = 0x00080000,

      SilverGMag       = 0x00100000,
      SilverGBPMag     = 0x00200000,
      SilverGRPMag     = 0x00400000,
      SilverParx       = 0x00800000,

      BronzeGMag       = 0x01000000,
      BronzeGBPMag     = 0x02000000,
      BronzeGRPMag     = 0x04000000,
      BronzeParx       = 0x08000000,

      BPRPExcess       = 0x10000008,
      BPRPExcessHigh   = 0x20000000,

      GoldAstrometry   = 0x000800F0,
      SilverAstrometry = 0x00800F00,
      BronzeAstrometry = 0x0800F000,

      GoldPhotometry   = 0x00070000,
      SilverPhotometry = 0x00700000,
      BronzePhotometry = 0x07000000
   };
}

// ----------------------------------------------------------------------------

/*!
 * \struct GaiaStarData
 * \brief Star data structure for Gaia catalog search operations.
 *
 * \ingroup point_source_databases
 */
struct PCL_CLASS GaiaStarData
{
   double ra = 0;     //!< Right ascension in degrees, in the range [0,360).
   double dec = 0;    //!< Declination in degrees, in the range [-90,+90].
   float  parx = 0;   //!< Parallax in mas.
   float  pmra = 0;   //!< Proper motion in right ascension * cos(dec), in mas/year.
   float  pmdec = 0;  //!< Proper motion in declination, in mas/year.
   float  magG = 0;   //!< Mean G magnitude.
   float  magBP = 0;  //!< Mean G_BP magnitude.
   float  magRP = 0;  //!< Mean G_RP magnitude.
   uint32 flags = 0u; //!< Data availability and quality flags. See the GaiaStarFlag namespace.
};

// ----------------------------------------------------------------------------

/*!
 * \struct pcl::GaiaSearchData
 * \brief Data items and parameters for Gaia catalog search operations.
 *
 * \ingroup point_source_databases
 */
typedef XPSD::SearchData<GaiaStarData> GaiaSearchData;

// ----------------------------------------------------------------------------

/*!
 * \class GaiaDatabaseFile
 * \brief Gaia catalog star database file (XPSD format).
 *
 * This class implements an interface to XPSD files serializing encoded Gaia
 * star data. As of writing this documentation (December 2020), Gaia DR2 and
 * EDR3 are supported and have been implemented.
 *
 * The most important functionality of this class is performing fast indexed
 * search operations to retrieve point source data for Gaia stars matching a
 * set of user-defined criteria. See the GaiaDatabaseFile::Search() member
 * function and the GaiaSearchData structure for detailed information.
 *
 * This implementation provides the following data for the complete Gaia DR2
 * and EDR3 catalogs:
 *
 * \li Source positions.
 * \li Parallaxes.
 * \li Proper motions.
 * \li Mean magnitudes on the G, GBP and GRP bands.
 * \li Data availability and quality flags.
 *
 * \b References
 *
 * \li Gaia Data Release 2 - online resources:
 * https://www.cosmos.esa.int/web/gaia/data-release-2
 *
 * \li <em>Gaia Data Release 2. Summary of the contents and survey
 * properties.</em> Gaia Collaboration, Brown, A.G.A., et al.:
 * https://arxiv.org/abs/1804.09365v2
 *
 * \li Gaia Data Release 2. Documentation release 1.2:
 * https://gea.esac.esa.int/archive/documentation/GDR2/index.html
 *
 * \li Gaia Early Data Release 3 - online resources:
 * https://www.cosmos.esa.int/web/gaia/early-data-release-3
 *
 * \li <em>Gaia Early Data Release 3. Summary of the contents and survey
 * properties.</em> Gaia Collaboration, A.G.A. Brown, A. Vallenari, T. Prusti,
 * J.H.J. de Bruijne, et al.:
 * https://www.aanda.org/articles/aa/pdf/forth/aa39657-20.pdf
 *
 * \li Gaia Early Data Release 3. Documentation release 1.0:
 * https://gea.esac.esa.int/archive/documentation/GEDR3/index.html
 *
 * \b Credits
 *
 * This work has made use of data from the European Space Agency (ESA) mission
 * Gaia (https://www.cosmos.esa.int/gaia), processed by the Gaia Data
 * Processing and Analysis Consortium (DPAC,
 * https://www.cosmos.esa.int/web/gaia/dpac/consortium). Funding for the DPAC
 * has been provided by national institutions, in particular the institutions
 * participating in the Gaia Multilateral Agreement.
 *
 * \sa StarDatabaseFile, APASSDatabaseFile
 * \ingroup point_source_databases
 */
class PCL_CLASS GaiaDatabaseFile : public StarDatabaseFile
{
public:

   /*!
    * Default constructor.
    *
    * Constructs an invalid instance that cannot be used until initialized by
    * calling the Open() member function.
    */
   GaiaDatabaseFile() = default;

   /*!
    * Constructs a &GaiaDatabaseFile instance initialized from the specified
    * point source database file in XPSD format. As of writing this
    * documentation (December 2020), The Gaia DR2 and EDR3 catalogs are
    * available.
    *
    * In the event of errors or invalid data, this constructor will throw the
    * appropriate Error exception.
    */
   GaiaDatabaseFile( const String& filePath )
      : StarDatabaseFile( filePath )
   {
      static_assert( sizeof( EncodedStarData ) == 32, "Invalid sizeof( GaiaDatabaseFile::EncodedStarData )" );
      if ( Metadata().databaseIdentifier == "GaiaEDR3" )
         m_dr = "EDR3";
      else if ( Metadata().databaseIdentifier == "GaiaDR2" )
      {
         m_dr = "DR2";
         StringList tokens;
         Metadata().databaseVersion.Break( tokens, '.' );
         // Make sure we reject an unsupported DR2 version older than 1.0.2
         if ( tokens.Length() < 3 || tokens[0].ToInt() < 1 || tokens[1].ToInt() < 0 || tokens[2].ToInt() < 2 )
            throw Error( "Unsupported Gaia DR2 database version '"
                        + Metadata().databaseVersion  + "': " + filePath );
      }
      else
         throw Error( "Invalid or unsupported Gaia database file with unknown identifier '"
                     + Metadata().databaseIdentifier  + "': " + filePath );
   }

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   GaiaDatabaseFile& operator =( GaiaDatabaseFile&& ) = default;

   /*!
    * Deleted copy constructor. %GaiaDatabaseFile instances are unique,
    * hence cannot be copied.
    */
   GaiaDatabaseFile( const GaiaDatabaseFile& ) = delete;

   /*!
    * Deleted copy assignment operator. %GaiaDatabaseFile instances are
    * unique, hence cannot be copied.
    */
   GaiaDatabaseFile& operator =( const GaiaDatabaseFile& ) = delete;

   /*!
    * Performs a search operation for point sources matching the specified
    * criteria.
    *
    * This member function performs a fast indexed search for point sources in
    * this database file matching the criteria defined in the specified \a data
    * structure. See the GaiaSearchData structure for detailed information on
    * search parameters and output data.
    *
    * Summarily, search criteria include:
    *
    * \li The region of the sky where point sources will be searched for. This
    * region is defined by the equatorial coordinates of a field center and a
    * field radius.
    *
    * \li An optional range of magnitudes.
    *
    * \li Optional inclusion/exclusion flags.
    *
    * \li An optional limit for the number of sources included in the search
    * result.
    *
    * The result of the search operation is also returned in the specified
    * \a data structure, including, among others, the following items:
    *
    * \li The list of point sources found.
    *
    * \li Instrumentation items for performance analysis, including: total
    * search time, time used for I/O operations, total I/O operations, time
    * used for data decoding, and time used for data decompression.
    */
   void Search( GaiaSearchData& data ) const
   {
      ElapsedTime T;
      for ( const XPSD::IndexTree& tree : m_index )
         tree.Search( data.centerRA, data.centerDec, data.radius, &data );
      data.timeTotal += T();
   }

   /*!
    * Returns the name of the Gaia data release corresponding to the data
    * available in this database file. As of writing this documentation
    * (December 2020), this member function can return either "DR2" or "EDR3".
    */
   const IsoString& DataRelease() const
   {
      return m_dr;
   }

private:

   IsoString m_dr; // data release, one of "DR2", "EDR3"

#pragma pack(push, 1)
   /*
    * Encoded star record (32 bytes uncompressed).
    */
   struct EncodedStarData
   {
      // Projected coordinates relative to the origin of the parent quadtree
      // node, in 0.002 mas units.
      uint32 dx;
      uint32 dy;
      // Parallax in mas units.
      float  parx;
      // Proper motions, mas/yr.
      float  pmra;
      float  pmdec;
      // Mean magnitudes in 0.001 mag units, encoded as (mag + 1.5)*1000.
      uint16 magG;
      uint16 magBP;
      uint16 magRP;
      // Right ascension correction for high declinations, in 0.01 mas units.
      int16  dra;
      // Data availability and quality flags.
      uint32 flags;
   };
#pragma pack(pop)

   void LoadData( void* block, uint64 offset, uint32 size, void* searchData ) const override
   {
      ElapsedTime T;
      StarDatabaseFile::LoadData( block, offset, size, searchData );
      reinterpret_cast<GaiaSearchData*>( searchData )->timeIO += T();
      ++reinterpret_cast<GaiaSearchData*>( searchData )->countIO;
   }

   void Uncompress( ByteArray& block, uint32 uncompressedSize, void* searchData ) const override
   {
      ElapsedTime T;
      StarDatabaseFile::Uncompress( block, uncompressedSize, searchData );
      reinterpret_cast<GaiaSearchData*>( searchData )->timeUncompress += T();
   }

   void GetEncodedData( const ByteArray& data, const XPSD::IndexTree& tree, const XPSD::IndexNode& node, void* searchData ) const override
   {
      ElapsedTime T;
      GaiaSearchData* search = reinterpret_cast<GaiaSearchData*>( searchData );
      double r = Rad( search->radius );
      const EncodedStarData* S = reinterpret_cast<const EncodedStarData*>( data.Begin() );
      int count = int( data.Size() / sizeof( EncodedStarData ) );
      int matched = 0;
      for ( int i = 0; i < count; ++i, ++S )
         if ( search->requiredFlags == 0 || (S->flags & search->requiredFlags) == search->requiredFlags )
            if ( search->inclusionFlags == 0 || (S->flags & search->inclusionFlags) != 0 )
               if ( search->exclusionFlags == 0 || (S->flags & search->exclusionFlags) == 0 )
               {
                  float magG = 0.001*S->magG - 1.5;
                  if ( magG >= search->magnitudeLow )
                     if ( magG <= search->magnitudeHigh )
                     {
                        GaiaStarData star;
                        double x = node.x0 + double( S->dx )/3600/1000/500;
                        double y = node.y0 + double( S->dy )/3600/1000/500;
                        tree.Unproject( star.ra, star.dec, x, y );
                        if ( unlikely( S->dra != 0 ) )
                        {
                           star.ra += double( S->dra )/3600/1000/100;
                           if ( star.ra < 0 )
                              star.ra += 360;
                           else if ( star.ra >= 360 )
                              star.ra -= 360;
                        }
                        if ( Distance( search->centerRA, search->centerDec, star.ra, star.dec ) < r )
                        {
                           if ( search->stars.Length() < size_type( search->sourceLimit ) )
                           {
                              star.parx = S->parx;
                              star.pmra = S->pmra;
                              star.pmdec = S->pmdec;
                              star.magG = magG;
                              star.magBP = 0.001*S->magBP - 1.5;
                              star.magRP = 0.001*S->magRP - 1.5;
                              star.flags = S->flags;
                              search->stars << star;
                           }
                           else
                              ++search->excessCount;
                           ++matched;
                        }
                     }
               }

      search->rejectCount += count - matched;
      search->timeDecode += T();
   }

   friend class GaiaDR2DatabaseFileGenerator;
   friend class GaiaEDR3DatabaseFileGenerator;
};

// ----------------------------------------------------------------------------

} // pcl

#endif  // __PCL_GaiaDatabaseFile_h

// ----------------------------------------------------------------------------
// EOF pcl/GaiaDatabaseFile.h - Released 2020-12-17T15:46:28Z
