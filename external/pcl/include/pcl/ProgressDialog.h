//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/ProgressDialog.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_ProgressDialog_h
#define __PCL_ProgressDialog_h

/// \file pcl/ProgressDialog.h

#include <pcl/Defs.h>

#include <pcl/Dialog.h>
#include <pcl/Label.h>
#include <pcl/PushButton.h>
#include <pcl/Sizer.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class ProgressDialog
 * \brief A simple progress bar dialog box.
 *
 * %ProgressDialog is a specialized modal dialog box to provide visual feedback
 * about an ongoing task during potentially long processes. It includes a
 * progress bar indicator that can change its graphical appearance to show the
 * percentage of the total work already done (\e bounded progress bar), or just
 * to inform the user that the task is still being done (unbounded case). The
 * dialog also provides a customizable text label and an optional Cancel button
 * that the user can activate to interrupt the process.
 *
 * \sa ProgressBarStatus
 */
class PCL_CLASS ProgressDialog : public Dialog
{
public:

   /*!
    * Constructs a %ProgressDialog.
    *
    * \param text    The text that will be shown on the informative label above
    *                the progress bar. The default text is an empty string,
    *                which hides the informative label.
    *
    * \param title   The title of the dialog box window. The default value is
    *                an empty string, which causes the window to show a default
    *                title defined by the PixInsight core application.
    *
    * \param lowerBound    The minimum value of the progress bar. The default
    *                value is zero.
    *
    * \param upperBound    The maximum value of the progress bar. The default
    *                value is 100, which is appropriate to show progress values
    *                as a percentage of the total work.
    *
    * \param parent        The parent control of this dialog. The default value
    *                is Control::Null(), which creates a child top-level window
    *                of the current workspace.
    */
   ProgressDialog( const String& text = String(),
                   const String& title = String(),
                   size_type lowerBound = 0, size_type upperBound = 100,
                   Control& parent = Control::Null() );

   /*!
    * Sets the window title for this dialog box.
    */
   void SetTitle( const String& title );

   /*!
    * Sets the range of values for the progress bar indicator.
    *
    * \param lowerBound    The minimum value of the progress bar.
    *
    * \param upperBound    The maximum value of the progress bar.
    *
    * If both bounds are set to the same value, e.g. \a lowerBound =
    * \a upperBound = 0, the progress bar will be \e unbounded. Unbounded
    * progress bars change their appearance in a special way to show that the
    * running process is still working, but without any specific information
    * about the amount of the total work already done.
    */
   void SetRange( size_type lowerBound, size_type upperBound );

   /*!
    * Causes the progress bar to be \e unbounded.
    *
    * Bounded progress bars show information about the amount of the total work
    * already done. Unbounded progress bars change their appearance to inform
    * that the process is still running, but without any specific hint about
    * the amount of pending work. Calling this function is equivalent to:
    *
    * \code SetRange( 0, 0 ); \endcode
    */
   void SetUnbounded()
   {
      SetRange( 0, 0 );
   }

   /*!
    * Returns the lower bound of the progress bar indicator, or zero if the
    * progress bar is unbounded.
    */
   size_type LowerBound() const
   {
      return m_lowerBound;
   }

   /*!
    * Returns the upper bound of the progress bar indicator, or zero if the
    * progress bar is unbounded.
    */
   size_type UpperBound() const
   {
      return m_upperBound;
   }

   /*!
    * Sets the informative text shown on a label above the progress bar.
    */
   void SetText( const String& text );

   /*!
    * Sets the current value of the progress bar indicator.
    *
    * If the progress bar is bounded, the specified \a value will be
    * constrained to the current limits, as reported by LowerBound() and
    * UpperBound().
    *
    * If the progress bar is unbounded, the \a value argument will be ignored
    * and the progress bar indicator will change graphically to provide
    * feedback about a running process, but without any specific information
    * about the amount of work already done.
    *
    * In all cases the progress bar will be updated on the screen as soon as
    * possible after returning from this function.
    */
   void SetValue( size_type value );

   /*!
    * Increments the value of the progress bar indicator.
    *
    * Calling this function is equivalent to:
    *
    * \code SetValue( m_value + 1 ); \endcode
    */
   void Increment()
   {
      SetValue( m_value + 1 );
   }

   /*!
    * Returns true if the Cancel button has been activated by the user, if it
    * is enabled.
    */
   bool IsCanceled() const
   {
      return m_canceled;
   }

   /*!
    * Enables the Cancel button of this progress dialog. When the Cancel button
    * is enabled, the user can activate it to interrupt the running process.
    */
   void EnableCancelButton( bool enable = true );

   /*!
    * Disables the Cancel button of this progress dialog. When the Cancel
    * button is disabled, the user has no way to interrupt the running process.
    */
   void DisableCancelButton( bool disable = true )
   {
      EnableCancelButton( !disable );
   }

   /*!
    * Returns true if the Cancel button is currently enabled.
    */
   bool IsCancelButtonEnabled() const
   {
      return m_cancelButton.IsVisible();
   }

protected:

   /*!
    * \class ProgressDialog::ProgressBar
    * \internal
    */
   class ProgressBar : public Control
   {
   public:

      ProgressBar();

   private:

      double    m_value = 0; // only if bounded, in [0,1]
      size_type m_step = 0;  // only if unbounded, in [0,Width()-1]
      bool      m_bounded = true;

      void e_Paint( Control&, const Rect& );

      friend class ProgressDialog;
   };

   size_type   m_lowerBound = 0;
   size_type   m_upperBound = 100;
   size_type   m_value = 0;
   bool        m_canceled = false;
   Label       m_infoLabel;
   ProgressBar m_progressBar;
   PushButton  m_cancelButton;

   void e_Click( Button& sender, bool checked );
   void e_Close( Control& sender, bool& allowClose );
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_ProgressDialog_h

// ----------------------------------------------------------------------------
// EOF pcl/ProgressDialog.h - Released 2020-12-17T15:46:29Z
