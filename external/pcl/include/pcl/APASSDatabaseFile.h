//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/APASSDatabaseFile.h - Released 2020-12-17T15:46:28Z
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

#ifndef __PCL_APASSDatabaseFile_h
#define __PCL_APASSDatabaseFile_h

/// \file pcl/APASSDatabaseFile.h

#include <pcl/Defs.h>

#include <pcl/ElapsedTime.h>
#include <pcl/Flags.h>
#include <pcl/StarDatabaseFile.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \namespace pcl::APASSStarFlag
 * \brief Data availability and quality flags for APASS star data.
 *
 * <table border="1" cellpadding="4" cellspacing="0">
 * <tr><td>APASSStarFlag::NoMag_V</td>      <td>No Johnson V magnitude available.</td></tr>
 * <tr><td>APASSStarFlag::NoMag_B</td>      <td>No Johnson B magnitude available.</td></tr>
 * <tr><td>APASSStarFlag::NoMag_u</td>      <td>No Sloan u' magnitude available (APASS DR10 only).</td></tr>
 * <tr><td>APASSStarFlag::NoMag_g</td>      <td>No Sloan g' magnitude available.</td></tr>
 * <tr><td>APASSStarFlag::NoMag_r</td>      <td>No Sloan r' magnitude available.</td></tr>
 * <tr><td>APASSStarFlag::NoMag_i</td>      <td>No Sloan i' magnitude available.</td></tr>
 * <tr><td>APASSStarFlag::NoMag_z_s</td>    <td>No Sloan z_s magnitude available (APASS DR10 only).</td></tr>
 * <tr><td>APASSStarFlag::NoMag_Y</td>      <td>No Sloan Y magnitude available (APASS DR10 only).</td></tr>
 * <tr><td>APASSStarFlag::PosErrorHigh</td> <td>Uncertainty in right ascension or declination greater than 0.75 arcseconds.</td></tr>
 * </table>
 *
 * \ingroup point_source_databases
 */
namespace APASSStarFlag
{
   enum mask_type
   {
      NoMag_V      = 0x0001,
      NoMag_B      = 0x0002,
      NoMag_u      = 0x0004,
      NoMag_g      = 0x0008,
      NoMag_r      = 0x0010,
      NoMag_i      = 0x0020,
      NoMag_z_s    = 0x0040,
      NoMag_Y      = 0x0080,
      PosErrorHigh = 0x0100
   };
}

// ----------------------------------------------------------------------------

/*!
 * \struct APASSStarData
 * \brief Star data structure for APASS catalog search operations.
 *
 * \ingroup point_source_databases
 */
struct PCL_CLASS APASSStarData
{
   double ra = 0;       //!< Right ascension in degrees, in the range [0,360).
   double dec = 0;      //!< Declination in degrees, in the range [-90,+90].
   float  mag_V = 0;    //!< Magnitude in Johnson V (Vega system).
   float  mag_B = 0;    //!< Magnitude in Johnson B (Vega system).
   float  mag_u = 0;    //!< Magnitude in Sloan u' (AB system) (APASS DR10 only).
   float  mag_g = 0;    //!< Magnitude in Sloan g' (AB system).
   float  mag_r = 0;    //!< Magnitude in Sloan r' (AB system).
   float  mag_i = 0;    //!< Magnitude in Sloan i' (AB system).
   float  mag_z_s = 0;  //!< Magnitude in Sloan z_s (AB system) (APASS DR10 only).
   float  mag_Y = 0;    //!< Magnitude in Sloan Y (AB system) (APASS DR10 only).
   float  err_V = 0;    //!< Uncertainty in mag_V.
   float  err_B = 0;    //!< Uncertainty in mag_B.
   float  err_u = 0;    //!< Uncertainty in mag_u (APASS DR10 only).
   float  err_g = 0;    //!< Uncertainty in mag_g.
   float  err_r = 0;    //!< Uncertainty in mag_r.
   float  err_i = 0;    //!< Uncertainty in mag_i.
   float  err_z_s = 0;  //!< Uncertainty in mag_z_s (APASS DR10 only).
   float  err_Y = 0;    //!< Uncertainty in mag_Y (APASS DR10 only).
   uint16 flags = 0u;   //!< Data availability and quality flags. See the APASSStarFlag namespace.
};

// ----------------------------------------------------------------------------

/*!
 * \struct pcl::APASSSearchData
 * \brief Data items and parameters for APASS catalog search operations.
 *
 * \ingroup point_source_databases
 */
typedef XPSD::SearchData<APASSStarData> APASSSearchData;

// ----------------------------------------------------------------------------

/*!
 * \class APASSDatabaseFile
 * \brief APASS catalog star database file (XPSD format).
 *
 * This class implements an interface to XPSD files serializing encoded APASS
 * star data. As of writing this documentation (December 2020), APASS DR9 and
 * DR10 are supported and have been implemented.
 *
 * The most important functionality of this class is performing fast indexed
 * search operations to retrieve point source data for APASS stars matching a
 * set of user-defined criteria. See the APASSDatabaseFile::Search() member
 * function and the APASSSearchData structure for detailed information.
 *
 * This implementation provides the following data for the complete APASS DR9
 * and DR10 catalogs:
 *
 * \li Source positions.
 * \li Magnitudes on the Johnson V and B bands (Vega system) and Sloan u', g',
 * r', i', z_s and Y magnitudes (AB system).
 * \li Data availability and quality flags.
 *
 * \b References
 *
 * \li APASS: The AAVSO Photometric All-Sky Survey:
 * https://www.aavso.org/apass
 *
 * \b Credits
 *
 * This work makes use of data from the AAVSO Photometric All Sky Survey, whose
 * funding has been provided by the Robert Martin Ayers Sciences Fund and from
 * the NSF (AST-1412587).
 *
 * \sa StarDatabaseFile, GaiaDatabaseFile
 * \ingroup point_source_databases
 */
class PCL_CLASS APASSDatabaseFile : public StarDatabaseFile
{
public:

   /*!
    * Default constructor.
    *
    * Constructs an invalid instance that cannot be used until initialized by
    * calling the Open() member function.
    */
   APASSDatabaseFile() = default;

   /*!
    * Constructs a &APASSDatabaseFile instance initialized from the specified
    * point source database file in XPSD format. As of writing this
    * documentation (December 2020), The APASS DR9 and DR10 catalogs are
    * available.
    *
    * In the event of errors or invalid data, this constructor will throw the
    * appropriate Error exception.
    */
   APASSDatabaseFile( const String& filePath )
      : StarDatabaseFile( filePath )
   {
      static_assert( sizeof( EncodedDR9StarData ) == 32, "Invalid sizeof( APASSDatabaseFile::EncodedDR9StarData )" );
      static_assert( sizeof( EncodedDR10StarData ) == 36, "Invalid sizeof( APASSDatabaseFile::EncodedDR10StarData )" );
      if ( Metadata().databaseIdentifier == "APASSDR9" )
      {
         m_dr = "DR9";
         m_decoder = &APASSDatabaseFile::GetEncodedDR9Data;
      }
      else if ( Metadata().databaseIdentifier == "APASSDR10" )
      {
         m_dr = "DR10";
         m_decoder = &APASSDatabaseFile::GetEncodedDR10Data;
      }
      else
         throw Error( "Invalid or unsupported APASS database file with unknown identifier '"
                     + Metadata().databaseIdentifier  + "': " + filePath );
   }

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   APASSDatabaseFile& operator =( APASSDatabaseFile&& ) = default;

   /*!
    * Deleted copy constructor. %APASSDatabaseFile instances are unique,
    * hence cannot be copied.
    */
   APASSDatabaseFile( const APASSDatabaseFile& ) = delete;

   /*!
    * Deleted copy assignment operator. %APASSDatabaseFile instances are
    * unique, hence cannot be copied.
    */
   APASSDatabaseFile& operator =( const APASSDatabaseFile& ) = delete;

   /*!
    * Performs a search operation for point sources matching the specified
    * criteria.
    *
    * This member function performs a fast indexed search for point sources in
    * this database file matching the criteria defined in the specified \a data
    * structure. See the APASSSearchData structure for detailed information on
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
   void Search( APASSSearchData& data ) const
   {
      ElapsedTime T;
      for ( const XPSD::IndexTree& tree : m_index )
         tree.Search( data.centerRA, data.centerDec, data.radius, &data );
      data.timeTotal += T();
   }

   /*!
    * Returns the name of the APASS data release corresponding to the data
    * available in this database file. As of writing this documentation
    * (December 2020), this member function can return either "DR9" or "DR10".
    */
   const IsoString& DataRelease() const
   {
      return m_dr;
   }

private:

   IsoString m_dr; // data release, one of "DR9", "DR10"

   typedef void (APASSDatabaseFile::*star_decoder)( const ByteArray&, const XPSD::IndexTree&, const XPSD::IndexNode&, void* ) const;
   star_decoder m_decoder = nullptr;

#pragma pack(push, 1)

   /*
    * Encoded DR9 star record (32 bytes uncompressed).
    */
   struct EncodedDR9StarData
   {
      // Projected coordinates relative to the origin of the parent quadtree
      // node, in mas units.
      uint32 dx;
      uint32 dy;
      // Magnitudes in 0.001 mag units, encoded as (mag + 1.5)*1000.
      uint16 mag_V;
      uint16 mag_B;
      uint16 mag_g;
      uint16 mag_r;
      uint16 mag_i;
      // Magnitude uncertainties in 0.001 mag units.
      uint16 err_V;
      uint16 err_B;
      uint16 err_g;
      uint16 err_r;
      uint16 err_i;
      // Right ascension correction for high declinations, in 0.1 mas units.
      int16  dra;
      // Data availability and quality flags.
      uint16 flags;
   };

   /*
    * Encoded DR10 star record (36 bytes uncompressed).
    */
   struct EncodedDR10StarData
   {
      // Projected coordinates relative to the origin of the parent quadtree
      // node, in mas units.
      uint32 dx;
      uint32 dy;
      // Magnitudes in 0.001 mag units, encoded as (mag + 1.5)*1000.
      uint16 mag_V;
      uint16 mag_B;
   // uint16 mag_u;
      uint16 mag_g;
      uint16 mag_r;
      uint16 mag_i;
      uint16 mag_z_s;
   // uint16 mag_Y;
      // Magnitude uncertainties in 0.001 mag units.
      uint16 err_V;
      uint16 err_B;
   // uint16 err_u;
      uint16 err_g;
      uint16 err_r;
      uint16 err_i;
      uint16 err_z_s;
   // uint16 err_Y;
      // Right ascension correction for high declinations, in 0.1 mas units.
      int16  dra;
      // Data availability and quality flags.
      uint16 flags;
   };

#pragma pack(pop)

   void LoadData( void* block, uint64 offset, uint32 size, void* searchData ) const override
   {
      ElapsedTime T;
      StarDatabaseFile::LoadData( block, offset, size, searchData );
      reinterpret_cast<APASSSearchData*>( searchData )->timeIO += T();
      ++reinterpret_cast<APASSSearchData*>( searchData )->countIO;
   }

   void Uncompress( ByteArray& block, uint32 uncompressedSize, void* searchData ) const override
   {
      ElapsedTime T;
      StarDatabaseFile::Uncompress( block, uncompressedSize, searchData );
      reinterpret_cast<APASSSearchData*>( searchData )->timeUncompress += T();
   }

   void GetEncodedData( const ByteArray& data, const XPSD::IndexTree& tree, const XPSD::IndexNode& node, void* searchData ) const override
   {
      (this->*m_decoder)( data, tree, node, searchData );
   }

   void GetEncodedDR9Data( const ByteArray& data, const XPSD::IndexTree& tree, const XPSD::IndexNode& node, void* searchData ) const
   {
      ElapsedTime T;
      APASSSearchData* search = reinterpret_cast<APASSSearchData*>( searchData );
      double r = Rad( search->radius );
      const EncodedDR9StarData* S = reinterpret_cast<const EncodedDR9StarData*>( data.Begin() );
      int count = int( data.Size() / sizeof( EncodedDR9StarData ) );
      int matched = 0;
      for ( int i = 0; i < count; ++i, ++S )
         if ( search->requiredFlags == 0 || (S->flags & search->requiredFlags) == search->requiredFlags )
            if ( search->inclusionFlags == 0 || (S->flags & search->inclusionFlags) != 0 )
               if ( search->exclusionFlags == 0 || (S->flags & search->exclusionFlags) == 0 )
               {
                  float mag_V = 0.001*S->mag_V - 1.5;
                  if ( mag_V >= search->magnitudeLow )
                     if ( mag_V <= search->magnitudeHigh )
                     {
                        APASSStarData star;
                        double x = node.x0 + double( S->dx )/3600/1000;
                        double y = node.y0 + double( S->dy )/3600/1000;
                        tree.Unproject( star.ra, star.dec, x, y );
                        if ( unlikely( S->dra != 0 ) )
                        {
                           star.ra += double( S->dra )/3600/1000/10;
                           if ( star.ra < 0 )
                              star.ra += 360;
                           else if ( star.ra >= 360 )
                              star.ra -= 360;
                        }
                        if ( Distance( search->centerRA, search->centerDec, star.ra, star.dec ) < r )
                        {
                           if ( search->stars.Length() < size_type( search->sourceLimit ) )
                           {
                              star.mag_V = mag_V;
                              star.mag_B = 0.001*S->mag_B - 1.5;
                              star.mag_g = 0.001*S->mag_g - 1.5;
                              star.mag_r = 0.001*S->mag_r - 1.5;
                              star.mag_i = 0.001*S->mag_i - 1.5;
                              star.err_V = 0.001*S->err_V;
                              star.err_B = 0.001*S->err_B;
                              star.err_g = 0.001*S->err_g;
                              star.err_r = 0.001*S->err_r;
                              star.err_i = 0.001*S->err_i;
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

   void GetEncodedDR10Data( const ByteArray& data, const XPSD::IndexTree& tree, const XPSD::IndexNode& node, void* searchData ) const
   {
      ElapsedTime T;
      APASSSearchData* search = reinterpret_cast<APASSSearchData*>( searchData );
      double r = Rad( search->radius );
      const EncodedDR10StarData* S = reinterpret_cast<const EncodedDR10StarData*>( data.Begin() );
      int count = int( data.Size() / sizeof( EncodedDR10StarData ) );
      int matched = 0;
      for ( int i = 0; i < count; ++i, ++S )
         if ( search->requiredFlags == 0 || (S->flags & search->requiredFlags) == search->requiredFlags )
            if ( search->inclusionFlags == 0 || (S->flags & search->inclusionFlags) != 0 )
               if ( search->exclusionFlags == 0 || (S->flags & search->exclusionFlags) == 0 )
               {
                  float mag_V = 0.001*S->mag_V - 1.5;
                  if ( mag_V >= search->magnitudeLow )
                     if ( mag_V <= search->magnitudeHigh )
                     {
                        APASSStarData star;
                        double x = node.x0 + double( S->dx )/3600/1000;
                        double y = node.y0 + double( S->dy )/3600/1000;
                        tree.Unproject( star.ra, star.dec, x, y );
                        if ( unlikely( S->dra != 0 ) )
                        {
                           star.ra += double( S->dra )/3600/1000/10;
                           if ( star.ra < 0 )
                              star.ra += 360;
                           else if ( star.ra >= 360 )
                              star.ra -= 360;
                        }
                        if ( Distance( search->centerRA, search->centerDec, star.ra, star.dec ) < r )
                        {
                           if ( search->stars.Length() < size_type( search->sourceLimit ) )
                           {
                              star.mag_V   = mag_V;
                              star.mag_B   = 0.001*S->mag_B   - 1.5;
                           // star.mag_u   = 0.001*S->mag_u   - 1.5;
                              star.mag_g   = 0.001*S->mag_g   - 1.5;
                              star.mag_r   = 0.001*S->mag_r   - 1.5;
                              star.mag_i   = 0.001*S->mag_i   - 1.5;
                              star.mag_z_s = 0.001*S->mag_z_s - 1.5;
                           // star.mag_Y   = 0.001*S->mag_Y   - 1.5;
                              star.err_V   = 0.001*S->err_V;
                              star.err_B   = 0.001*S->err_B;
                           // star.err_u   = 0.001*S->err_u;
                              star.err_g   = 0.001*S->err_g;
                              star.err_r   = 0.001*S->err_r;
                              star.err_i   = 0.001*S->err_i;
                              star.err_z_s = 0.001*S->err_z_s;
                           // star.err_Y   = 0.001*S->err_Y;
                              star.flags   = S->flags;
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

   friend class APASSDR9DatabaseFileGenerator;
   friend class APASSDR10DatabaseFileGenerator;
};

// ----------------------------------------------------------------------------

} // pcl

#endif  // __PCL_APASSDatabaseFile_h

// ----------------------------------------------------------------------------
// EOF pcl/APASSDatabaseFile.h - Released 2020-12-17T15:46:28Z
