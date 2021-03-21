//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/ProgressBarStatus.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_ProgressBarStatus_h
#define __PCL_ProgressBarStatus_h

/// \file pcl/ProgressBarStatus.h

#ifndef __PCL_BUILDING_PIXINSIGHT_APPLICATION

#include <pcl/Defs.h>

#include <pcl/AutoPointer.h>
#include <pcl/StatusMonitor.h>

namespace pcl
{

// ----------------------------------------------------------------------------

class ProgressDialog;

/*!
 * \class ProgressBarStatus
 * \brief A status monitoring callback that shows a modal progress dialog.
 *
 * %ProgressBarStatus is a StatusCallback derived class that opens a modal
 * ProgressDialog to provide visual feedback about a running process, without
 * freezing the graphical user interface dugin potentially long tasks. It
 * allows the user to interrupt the process by activating a standard Cancel
 * button on the progress dialog. This class supports both bounded and
 * unbounded status monitors.
 *
 * \sa StatusCallback, StatusMonitor, StandardStatus, SpinStatus, MuteStatus
 */
class PCL_CLASS ProgressBarStatus : public StatusCallback
{
public:

   /*!
    * Constructs a %ProgressBarStatus object.
    *
    * \param title   The window title for the modal progress bar dialog box.
    */
   ProgressBarStatus( const String& title );

   /*!
    * Virtual destructor.
    */
   virtual ~ProgressBarStatus();

   /*!
    * This function is called by a status \a monitor object when a new
    * monitored process is about to start.
    */
   int Initialized( const StatusMonitor& monitor ) const override;

   /*!
    * Function called by a status \a monitor object to signal an update of the
    * progress count for the current process.
    */
   int Updated( const StatusMonitor& monitor ) const override;

   /*!
    * Function called by a status \a monitor object to signal that the current
    * process has finished.
    */
   int Completed( const StatusMonitor& monitor ) const override;

   /*!
    * Function called by a status \a monitor object when the progress
    * information for the current process has been changed.
    */
   void InfoUpdated( const StatusMonitor& monitor ) const override;

private:

   mutable AutoPointer<ProgressDialog> m_progressDialog;
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_BUILDING_PIXINSIGHT_APPLICATION

#endif   // __PCL_ProgressBarStatus_h

// ----------------------------------------------------------------------------
// EOF pcl/ProgressBarStatus.h - Released 2020-12-17T15:46:29Z
