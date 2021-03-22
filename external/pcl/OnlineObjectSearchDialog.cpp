//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/OnlineObjectSearchDialog.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/MetaModule.h>
#include <pcl/NetworkTransfer.h>
#include <pcl/OnlineObjectSearchDialog.h>
#include <pcl/Settings.h>

#define SIMBAD_SERVER_KEY  "/Global/Dialogs/OnlineObjectSearch/SIMBADServerIdx"

namespace pcl
{

// ----------------------------------------------------------------------------

struct ServerData
{
   String name;
   String url;
};

static Array<ServerData> s_simbadServers;
static bool              s_dataInitialized = false;

static void InitializeData()
{
   if ( !s_dataInitialized )
   {
      s_simbadServers.Clear();
      s_simbadServers << ServerData{ "CDS Strasbourg, France", "http://simbad.u-strasbg.fr/" }
                      << ServerData{ "CFA Harvard, Cambridge, USA", "http://simbad.cfa.harvard.edu/" };
      s_dataInitialized = true;
   }
}

// ----------------------------------------------------------------------------

OnlineObjectSearchDialog::OnlineObjectSearchDialog()
{
   InitializeData();

   int labelWidth1 = Max( Font().Width( "Object:" ), Font().Width( "Server:" ) );

   const char* objectNameToolTip =
      "<p>Name or identifier of the object to search for. "
      "Examples: M31, Pleiades, NGC 253, Orion Nebula, Antares, alpha Lyr, SAO 67174.</p>";

   ObjectName_Label.SetText( "Object:" );
   ObjectName_Label.SetFixedWidth( labelWidth1 );
   ObjectName_Label.SetToolTip( objectNameToolTip );
   ObjectName_Label.SetTextAlignment( TextAlign::Left|TextAlign::VertCenter );

   ObjectName_Edit.SetToolTip( objectNameToolTip );
   ObjectName_Edit.OnGetFocus( (Control::event_handler)&OnlineObjectSearchDialog::e_GetFocus, *this );
   ObjectName_Edit.OnLoseFocus( (Control::event_handler)&OnlineObjectSearchDialog::e_LoseFocus, *this );

   Search_Button.SetText( "Search" );
   Search_Button.SetIcon( ScaledResource( ":/icons/find.png" ) );
   Search_Button.SetToolTip( "<p>Perform online coordinate search.</p>" );
   Search_Button.OnClick( (Button::click_event_handler)&OnlineObjectSearchDialog::e_Click, *this );

   Search_Sizer.SetSpacing( 4 );
   Search_Sizer.Add( ObjectName_Label );
   Search_Sizer.Add( ObjectName_Edit, 100 );
   Search_Sizer.Add( Search_Button );

   const char* serverToolTip =
      "<p>URL of a SIMBAD database server. Currently you can choose either the master SIMBAD service in France "
      "(Centre de Donn&eacute;es Astronomiques de Strasbourg), "
      "or the mirror site in the USA (Harvard-Smithsonian Center for Astrophysics)</p>";

   Server_Label.SetText( "Server:" );
   Server_Label.SetFixedWidth( labelWidth1 );
   Server_Label.SetToolTip( serverToolTip );
   Server_Label.SetTextAlignment( TextAlign::Left|TextAlign::VertCenter );

   for ( const ServerData& server : s_simbadServers )
      Server_ComboBox.AddItem( server.name );
   Server_ComboBox.OnItemSelected( (ComboBox::item_event_handler)&OnlineObjectSearchDialog::e_ItemSelected, *this );

   Server_Sizer.SetSpacing( 4 );
   Server_Sizer.Add( Server_Label );
   Server_Sizer.Add( Server_ComboBox );
   Server_Sizer.AddStretch();

   SearchInfo_TextBox.SetReadOnly();
   SearchInfo_TextBox.SetStyleSheet( ScaledStyleSheet(
         "QTextEdit {"
            "font-family: Hack, DejaVu Sans Mono, Monospace;"
            "font-size: 8pt;"
            "background: #141414;" // borrowed from /rsc/qss/core-standard.qss
            "color: #E8E8E8;"
         "}"
      ) );
   SearchInfo_TextBox.Restyle();
   SearchInfo_TextBox.SetMinSize( SearchInfo_TextBox.Font().Width( 'm' )*81, SearchInfo_TextBox.Font().Height()*22 );

   Get_Button.SetText( "Get" );
   Get_Button.SetIcon( ScaledResource( ":/icons/window-import.png" ) );
   Get_Button.SetToolTip( "<p>Acquire object coordinates.</p>" );
   Get_Button.OnClick( (Button::click_event_handler)&OnlineObjectSearchDialog::e_Click, *this );
   Get_Button.Disable();

   Cancel_Button.SetText( "Cancel" );
   Cancel_Button.SetIcon( ScaledResource( ":/icons/cancel.png" ) );
   Cancel_Button.OnClick( (Button::click_event_handler)&OnlineObjectSearchDialog::e_Click, *this );

   Buttons_Sizer.SetSpacing( 8 );
   Buttons_Sizer.AddStretch();
   Buttons_Sizer.Add( Get_Button );
   Buttons_Sizer.Add( Cancel_Button );

   Global_Sizer.SetSpacing( 8 );
   Global_Sizer.SetMargin( 8 );
   Global_Sizer.Add( Search_Sizer );
   Global_Sizer.Add( Server_Sizer );
   Global_Sizer.Add( SearchInfo_TextBox, 100 );
   Global_Sizer.Add( Buttons_Sizer );

   SetSizer( Global_Sizer );

   EnsureLayoutUpdated();
   AdjustToContents();
   SetMinSize();

   SetWindowTitle( "Online Object Search" );

   LoadSettings();
}

// ----------------------------------------------------------------------------

void OnlineObjectSearchDialog::e_GetFocus( Control& sender )
{
   if ( sender == ObjectName_Edit )
      Search_Button.SetDefault();
}

// ----------------------------------------------------------------------------

void OnlineObjectSearchDialog::e_LoseFocus( Control& sender )
{
   if ( sender == ObjectName_Edit )
      Get_Button.SetDefault();
}

// ----------------------------------------------------------------------------

bool OnlineObjectSearchDialog::e_Download( NetworkTransfer& sender, const void* buffer, fsize_type size )
{
   if ( m_abort )
      return false;

   m_downloadData.Append( static_cast<const char*>( buffer ), size );
   return true;
}

// ----------------------------------------------------------------------------

bool OnlineObjectSearchDialog::e_Progress( NetworkTransfer& sender,
                                         fsize_type downloadTotal, fsize_type downloadCurrent,
                                         fsize_type uploadTotal, fsize_type uploadCurrent )
{
   if ( m_abort )
      return false;

   if ( downloadTotal > 0 )
      SearchInfo_TextBox.Insert( String().Format( "<end><clrbol>%u of %u bytes transferred (%d%%)<flush>",
                                                  downloadCurrent, downloadTotal,
                                                  RoundInt( 100.0*downloadCurrent/downloadTotal ) ) );
   else
      SearchInfo_TextBox.Insert( String().Format( "<end><clrbol>%u bytes transferred (unknown size)<flush>",
                                                  downloadCurrent ) );
   SearchInfo_TextBox.Focus();
   Module->ProcessEvents();
   return true;
}

// ----------------------------------------------------------------------------

void OnlineObjectSearchDialog::e_Click( Button& sender, bool checked )
{
   if ( sender == Search_Button )
   {
      String objectName = ObjectName_Edit.Text().Trimmed();
      ObjectName_Edit.SetText( objectName );
      if ( objectName.IsEmpty() )
      {
         SearchInfo_TextBox.SetText( "\x1b[31m*** Error: No object has been specified.\x1b[39m<br>" );
         ObjectName_Edit.Focus();
         return;
      }

      m_valid = false;
      Get_Button.Disable();

      int serverIdx = Range( Server_ComboBox.CurrentItem(), 0, int( s_simbadServers.Length()-1 ) );
      String url( s_simbadServers[serverIdx].url + "simbad/sim-tap/sync?request=doQuery&lang=adql&format=TSV&query=" );
      String select_stmt = "SELECT oid, ra, dec, pmra, pmdec, plx_value, rvz_radvel, main_id, otype_txt, sp_type, flux "
                           "FROM basic "
                           "JOIN ident ON ident.oidref = oid "
                           "LEFT OUTER JOIN flux ON flux.oidref = oid AND flux.filter = 'V' "
                           "WHERE id = '" + objectName + "';";
      url << select_stmt;

      NetworkTransfer transfer;
      transfer.SetURL( url );
      transfer.OnDownloadDataAvailable( (NetworkTransfer::download_event_handler)&OnlineObjectSearchDialog::e_Download, *this );
      transfer.OnTransferProgress( (NetworkTransfer::progress_event_handler)&OnlineObjectSearchDialog::e_Progress, *this );

      SearchInfo_TextBox.SetText( "<wrap><raw>" + url + "</raw><br><br><flush>" );
      Module->ProcessEvents();

      m_downloadData.Clear();
      m_downloading = true;
      m_abort = false;
      bool ok = transfer.Download();
      m_downloading = false;

      if ( ok )
      {
         SearchInfo_TextBox.Insert( String().Format( "<end><clrbol>%d bytes downloaded @ %.3g KiB/s<br>",
                                                     transfer.BytesTransferred(), transfer.TotalSpeed() ) );
         //SearchInfo_TextBox.Insert( "<end><cbr><br><raw>" + m_downloadData + "</raw>" );

         StringList lines;
         m_downloadData.Break( lines, '\n' );
         if ( lines.Length() >= 2 )
         {
            // The first line has column titles. The second line has values.
            StringList tokens;
            lines[1].Break( tokens, '\t', true/*trim*/ );
            if ( tokens.Length() == 11 )
            {
               m_RA = tokens[1].ToDouble();                                   // degrees
               m_Dec = tokens[2].ToDouble();                                  // degrees
               m_muRA = tokens[3].IsEmpty() ? 0.0 : tokens[3].ToDouble();     // mas/yr
               m_muDec = tokens[4].IsEmpty() ? 0.0 : tokens[4].ToDouble();    // mas/yr
               m_parallax = tokens[5].IsEmpty() ? 0.0 : tokens[5].ToDouble(); // mas
               m_radVel = tokens[6].IsEmpty() ? 0.0 : tokens[6].ToDouble();   // km/s
               m_objectName = tokens[7].Unquoted().Trimmed();
               m_objectType = tokens[8].Unquoted().Trimmed();
               m_spectralType = tokens[9].Unquoted().Trimmed();
               m_vmag = tokens[10].IsEmpty() ? 101.0 : tokens[10].ToDouble();

               try
               {
                  String info =
                           "<end><cbr><br><b>Object            :</b> "
                        + m_objectName
                        +            "<br><b>Object type       :</b> "
                        + m_objectType
                        +            "<br><b>Right Ascension   :</b> "
                        + String::ToSexagesimal( m_RA/15,
                                       SexagesimalConversionOptions( 3/*items*/, 3/*precision*/, false/*sign*/, 3/*width*/ ) )
                        +            "<br><b>Declination       :</b> "
                        + String::ToSexagesimal( m_Dec,
                                       SexagesimalConversionOptions( 3/*items*/, 2/*precision*/, true/*sign*/, 3/*width*/ ) );
                  if ( m_muRA != 0 )
                     info
                        +=           "<br><b>Proper motion RA  :</b> "
                        + String().Format( "%+8.2f mas/year", m_muRA );
                  if ( m_muDec != 0 )
                     info
                        +=           "<br><b>Proper motion Dec :</b> "
                        + String().Format( "%+8.2f mas/year", m_muDec );
                  if ( m_parallax != 0 )
                     info
                        +=           "<br><b>Parallax          :</b> "
                        + String().Format( "%8.2f mas", m_parallax );
                  if ( m_radVel != 0 )
                     info
                        +=           "<br><b>Radial velocity   :</b> "
                        + String().Format( "%+.3g km/s", m_radVel );
                  if ( !m_spectralType.IsEmpty() )
                     info
                        +=           "<br><b>Spectral type     :</b> "
                        + m_spectralType;
                  if ( m_vmag < 100 )
                     info
                        +=           "<br><b>V Magnitude       :</b> "
                        + String().Format( "%.4g", m_vmag );
                  info += "<br>";

                  SearchInfo_TextBox.Insert( info );
                  m_valid = true;
                  Get_Button.Enable();
               }
               catch ( ... )
               {
               }
            }
         }

         if ( !m_valid )
            SearchInfo_TextBox.Insert( "<end><cbr><br>\x1b[31m*** Error: Unable to acquire valid coordinate information.\x1b[39m<br>" );
      }
      else
      {
         if ( m_abort )
            SearchInfo_TextBox.Insert( "<end><cbr><br>\x1b[31m<raw><* abort *></raw>\x1b[39m<br>" );
         else
            SearchInfo_TextBox.Insert( "<end><cbr><br>\x1b[31m*** Error: Download failed: <raw>" + transfer.ErrorInformation() + "</raw>\x1b[39m<br>" );
      }

      // ### FIXME: Workaround to force visibility of TextBox focus.
      SearchInfo_TextBox.Unfocus();
      Module->ProcessEvents();
      SearchInfo_TextBox.Focus();

      m_downloadData.Clear();
   }
   else if ( sender == Get_Button )
   {
      Ok();
   }
   else if ( sender == Cancel_Button )
   {
      if ( m_downloading )
         m_abort = true;
      else
         Cancel();
   }
}

// ----------------------------------------------------------------------------

void OnlineObjectSearchDialog::e_ItemSelected( ComboBox& sender, int itemIndex )
{
   if ( sender == Server_ComboBox )
      if ( itemIndex >= 0 )
         if ( size_type( itemIndex ) < s_simbadServers.Length() )
         {
            SaveSettings();
            Server_ComboBox.SetToolTip( s_simbadServers[itemIndex].url );
         }
}

// ----------------------------------------------------------------------------

void OnlineObjectSearchDialog::LoadSettings()
{
   int itemIndex = 0;
   Settings::Read( SIMBAD_SERVER_KEY, itemIndex );
   itemIndex = Range( itemIndex, 0, int( s_simbadServers.Length() ) );
   Server_ComboBox.SetCurrentItem( itemIndex );
   Server_ComboBox.SetToolTip( s_simbadServers[itemIndex].url );
}

// ----------------------------------------------------------------------------

void OnlineObjectSearchDialog::SaveSettings() const
{
   Settings::Write( SIMBAD_SERVER_KEY, Server_ComboBox.CurrentItem() );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/OnlineObjectSearchDialog.cpp - Released 2020-12-17T15:46:35Z
