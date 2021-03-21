//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/OnlineObjectSearchDialog.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_OnlineObjectSearchDialog_h
#define __PCL_OnlineObjectSearchDialog_h

/// \file pcl/OnlineObjectSearchDialog.h

#include <pcl/Defs.h>
#include <pcl/Diagnostics.h>

#include <pcl/ComboBox.h>
#include <pcl/Dialog.h>
#include <pcl/Edit.h>
#include <pcl/Label.h>
#include <pcl/PushButton.h>
#include <pcl/Sizer.h>
#include <pcl/TextBox.h>

namespace pcl
{

// ----------------------------------------------------------------------------

class PCL_CLASS NetworkTransfer;

// ----------------------------------------------------------------------------

/*!
 * \class OnlineObjectSearchDialog
 * \brief A dialog box to search for object data on online astronomical
 * database services.
 *
 * %OnlineObjectSearchDialog allows the user to enter the name or identifier of
 * an object to search for, such as 'M31', 'Pleiades', 'NGC 253',
 * 'Orion Nebula', 'Antares', or 'alpha Lyr'. The dialog sends an ADQL query to
 * a public SIMBAD database service to retrieve several object properties,
 * including ICRS equatorial coordinates, proper motions and visual magnitude,
 * among others.
 *
 * This class is a useful component for processes requiring a flexible and fast
 * way to retrieve data for user-selected objects. Currently two public SIMBAD
 * database services can be used: the master SIMBAD service in France (Centre
 * de Donn&eacute;es Astronomiques de Strasbourg) and its mirror site in the
 * USA (Harvard-Smithsonian Center for Astrophysics).
 *
 * \b References
 *
 * SIMBAD Astronomical Database - CDS Strasbourg:\n
 * http://simbad.u-strasbg.fr/simbad/
 *
 * SIMBAD mirror site in the USA - CFA Harvard:\n
 * http://simbad.cfa.harvard.edu/
 *
 * SIMBAD TAP Service:\n
 * http://simbad.u-strasbg.fr/simbad/sim-tap
 *
 * ADQL Cheat sheet:\n
 * http://simbad.u-strasbg.fr/simbad/tap/help/adqlHelp.html
 */
class PCL_CLASS OnlineObjectSearchDialog : public Dialog
{
public:

   /*!
    * Default constructor.
    */
   OnlineObjectSearchDialog();

   /*!
    * Returns the name of the object that has been found, or an empty string if
    * no object has been found or searched for.
    */
   const String& ObjectName() const
   {
      return m_objectName;
   }

   /*!
    * Returns a code representing the type of the object, or an empty string if
    * no object has been found or searched for.
    *
    * Object type codes are abbreviations standardized on the SIMBAD database,
    * such as 'GlC' (globular cluster), 'G' (galaxy), or '*' (star).
    */
   const String& ObjectType() const
   {
      return m_objectType;
   }

   /*!
    * Returns a string representing the spectral type, or an empty string if no
    * object has been found or searched for, or if the spectral type is not
    * available for the specified object.
    */
   const String& SpectralType() const
   {
      return m_spectralType;
   }

   /*!
    * Returns the V (visual) magnitude of the object, or zero if no object has
    * been found or searched for, or if the V magnitude is not available for
    * the specified object.
    */
   double VMagnitude() const
   {
      return m_vmag;
   }

   /*!
    * Returns the ICRS right ascension coordinate in degrees, or zero if no
    * object has been found or searched for.
    */
   double RA() const
   {
      return m_RA;
   }

   /*!
    * Returns the ICRS declination coordinate in degrees, or zero if no object
    * has been found or searched for.
    */
   double Dec() const
   {
      return m_Dec;
   }

   /*!
    * Returns the proper motion in right ascension in mas/year, or zero if no
    * object has been found or searched for, or if proper motions are not
    * available for the specified object.
    */
   double MuRA() const
   {
      return m_muRA;
   }

   /*!
    * Returns the proper motion in declination in mas/year, or zero if no
    * object has been found or searched for, or if proper motions are not
    * available for the specified object.
    */
   double MuDec() const
   {
      return m_muDec;
   }

   /*!
    * Returns the parallax in mas, or zero if no object has been found or
    * searched for, or if the parallax is not available for the specified
    * object.
    */
   double Parallax() const
   {
      return m_parallax;
   }

   /*!
    * Returns the radial velocity in km/s, or zero if no object has been found
    * or searched for, or if the radial velocity is not available for the
    * specified object.
    */
   double RadialVelocity() const
   {
      return m_radVel;
   }

   /*!
    * Returns true iff an object has been specified and valid data have been
    * found. In such case valid right ascension and declination coordinates are
    * always available. Other data items (proper motions, etc) are optional,
    * depending on the type of the object that has been searched for.
    */
   bool IsValid() const
   {
      return m_valid;
   }

   /*!
    * Returns the user-defined text that has been searched for during the
    * latest dialog execution. Typically this is either an empty string or the
    * name or identifier of an astronomical object, such as 'M31', 'Pleiades',
    * 'NGC 253', 'Orion Nebula', 'Antares', or 'alpha Lyr'.
    */
   String SearchText() const
   {
      return ObjectName_Edit.Text();
   }

   /*!
    * Returns the URL of the online database service provider that is currently
    * selected on this dialog.
    *
    * The database server URL selected by the user on these dialogs is a global
    * item stored in core application settings. It is restored by each instance
    * of this class upon creation.
    */
   String ServerURL() const;

protected:

   VerticalSizer       Global_Sizer;
      HorizontalSizer      Search_Sizer;
         Label                ObjectName_Label;
         Edit                 ObjectName_Edit;
         PushButton           Search_Button;
      HorizontalSizer      Server_Sizer;
         Label                Server_Label;
         ComboBox             Server_ComboBox;
      TextBox              SearchInfo_TextBox;
      HorizontalSizer      Buttons_Sizer;
         PushButton           Get_Button;
         PushButton           Cancel_Button;

   String    m_objectName;
   String    m_objectType;
   String    m_spectralType;
   double    m_vmag = 0;     // V magnitude
   double    m_RA = 0;       // degrees
   double    m_Dec = 0;      // degrees
   double    m_muRA = 0;     // mas/year
   double    m_muDec = 0;    // mas/year
   double    m_parallax = 0; // mas
   double    m_radVel = 0;   // km/s
   bool      m_valid = false;
   bool      m_downloading = false;
   bool      m_abort = false;
   IsoString m_downloadData;

   void e_Show( Control& sender );
   void e_GetFocus( Control& sender );
   void e_LoseFocus( Control& sender );
   bool e_Download( NetworkTransfer& sender, const void* buffer, fsize_type size );
   bool e_Progress( NetworkTransfer& sender,
                    fsize_type downloadTotal, fsize_type downloadCurrent,
                    fsize_type uploadTotal, fsize_type uploadCurrent );
   void e_Click( Button& sender, bool checked );
   void e_ItemSelected( ComboBox& sender, int itemIndex );

   void LoadSettings();
   void SaveSettings() const;
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_OnlineObjectSearchDialog_h

// ----------------------------------------------------------------------------
// EOF pcl/OnlineObjectSearchDialog.h - Released 2020-12-17T15:46:29Z
