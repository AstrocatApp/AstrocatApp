//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/ProgressDialog.cpp - Released 2020-12-17T15:46:35Z
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

#include <pcl/Graphics.h>
#include <pcl/ProgressDialog.h>

namespace pcl
{

// ----------------------------------------------------------------------------

ProgressDialog::ProgressDialog( const String& text, const String& title,
                                size_type lowerBound, size_type upperBound,
                                Control& parent )
   : Dialog( parent )
{
   m_infoLabel.SetText( text.Trimmed() );
   m_infoLabel.SetVisible( !m_infoLabel.Text().IsEmpty() );

   SetRange( lowerBound, upperBound );

   m_progressBar.SetScaledFixedHeight( 20 );
   m_progressBar.SetScaledMinWidth( 400 );

   m_cancelButton.SetText( "Cancel" );
   m_cancelButton.SetIcon( ScaledResource( ":/icons/cancel.png" ) );
   m_cancelButton.OnClick( (Button::click_event_handler)&ProgressDialog::e_Click, *this );

   HorizontalSizer buttonsSizer;
   buttonsSizer.AddStretch();
   buttonsSizer.Add( m_cancelButton );
   buttonsSizer.AddStretch();

   VerticalSizer sizer;
   sizer.SetMargin( 8 );
   sizer.SetSpacing( 8 );
   sizer.Add( m_infoLabel );
   sizer.Add( m_progressBar );
   sizer.AddSpacing( 8 );
   sizer.Add( buttonsSizer );

   SetSizer( sizer );
   EnsureLayoutUpdated();
   AdjustToContents();
   SetFixedHeight();
   SetMinWidth();

   if ( !title.IsEmpty() )
      SetWindowTitle( title );

   OnClose( (Control::close_event_handler)&ProgressDialog::e_Close, *this );
}

// ----------------------------------------------------------------------------

void ProgressDialog::SetTitle( const String& title )
{
   SetWindowTitle( title );
}

// ----------------------------------------------------------------------------

void ProgressDialog::SetRange( size_type lowerBound = 0, size_type upperBound = 100 )
{
   if ( upperBound < lowerBound )
      Swap( lowerBound, upperBound );
   else if ( upperBound == lowerBound )
      lowerBound = upperBound = 0;
   m_lowerBound = lowerBound;
   m_upperBound = upperBound;
   m_progressBar.m_bounded = m_lowerBound < m_upperBound;
   SetValue( m_lowerBound );
}

// ----------------------------------------------------------------------------

void ProgressDialog::SetText( const String& text )
{
   m_infoLabel.SetText( text.Trimmed() );
   m_infoLabel.SetVisible( !m_infoLabel.Text().IsEmpty() );
   SetVariableHeight();
   AdjustToContents();
   SetFixedHeight();
   SetMinWidth();
}

// ----------------------------------------------------------------------------

void ProgressDialog::SetValue( size_type value )
{
   m_value = Range( value, m_lowerBound, m_upperBound );
   if ( m_progressBar.m_bounded )
      m_progressBar.m_value = double( m_value - m_lowerBound )/(m_upperBound - m_lowerBound);
   else
      m_progressBar.m_step++;
   m_progressBar.Update();
}

// ----------------------------------------------------------------------------

void ProgressDialog::EnableCancelButton( bool enable )
{
   m_cancelButton.SetVisible( enable );
   SetVariableHeight();
   AdjustToContents();
   SetFixedHeight();
   SetMinWidth();
}

// ----------------------------------------------------------------------------

void ProgressDialog::e_Click( Button& sender, bool checked )
{
   if ( sender == m_cancelButton )
      m_canceled = true;
}

// ----------------------------------------------------------------------------

void ProgressDialog::e_Close( Control& sender, bool& allowClose )
{
   allowClose = false;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

ProgressDialog::ProgressBar::ProgressBar()
{
   SetFixedHeight( Label().Font().TightBoundingRect( "100%" ).Height() << 1 );
   OnPaint( (Control::paint_event_handler)&ProgressBar::e_Paint, *this );
}

// ----------------------------------------------------------------------------

void ProgressDialog::ProgressBar::e_Paint( Control& sender, const Rect& r )
{
   int d = sender.LogicalPixelsToPhysical( 1 );
   int d2 = d >> 1;
   Graphics G( sender );
   G.SetTransparentBackground();
   G.EnableTextAntialiasing();
   G.SetPen( 0xff505050, d );
   G.SetBrush( 0xfff0f0f0 );
   G.DrawRect( sender.BoundsRect().DeflatedBy( d2 ) );
   G.SetBrush( 0xffffa858 );
   if ( m_bounded )
   {
      G.FillRect( d, d, RoundInt( m_value*(sender.Width()-d-d2) ), sender.Height()-d-d2 );
      G.SetPen( 0xff000000 );
      G.DrawTextRect( sender.BoundsRect(), String().Format( "%.0f%%", m_value*100 ), TextAlign::Center );
   }
   else
   {
      if ( m_step >= size_type( sender.Width() ) )
         m_step = 0;
      G.FillRect( Max( d, int( m_step ) ), d,
                  Min( int( m_step ) + (sender.Width() >> 2), sender.Width()-d-d2 ), sender.Height()-d-d2 );
   }
   G.EndPaint();
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/ProgressDialog.cpp - Released 2020-12-17T15:46:35Z
