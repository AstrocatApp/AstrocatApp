//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/IntegrationMetadata.h - Released 2020-12-17T15:46:29Z
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

#ifndef __PCL_IntegrationMetadata_h
#define __PCL_IntegrationMetadata_h

/// \file pcl/IntegrationMetadata.h

#include <pcl/Console.h>
#include <pcl/FITSHeaderKeyword.h>
#include <pcl/Property.h>
#include <pcl/TimePoint.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class ConsistentlyDefined
 * \brief An object that can be in consistently defined or undefined/inconsistent states.
 *
 * %ConsistentlyDefined is similar to Optional: it stores an instance of the
 * template argument T, along with a <em>defined state</em> Boolean flag. In
 * addition, this class includes a \e consistency state flag. A
 * <em>consistently defined</em> object has been both defined and optionally
 * ckecked for consistency against other objects or values. Two objects are
 * considered consistent if an equality operator is true for them.
 *
 * As happens with %Optional, %ConsistentlyDefined objects update their defined
 * and consistency states automatically as they are created, copied and
 * assigned.
 *
 * \note This is a support utility class used internally by several modules,
 * made public mainly for code organization purposes. We make this class public
 * because it can be useful, but it still has not been fully developed as it
 * should to fulfill standard PCL quality standards.
 */
template <class T>
class PCL_CLASS ConsistentlyDefined
{
public:

   /*!
    * Constructs an undefined, consistent %ConsistentlyDefined object.
    *
    * The value instance will be default-constructed implicitly, which means
    * that the type T must provide valid default construction semantics.
    */
   ConsistentlyDefined()
      : m_value() // N.B: this initialization prevents warnings such as
   {              // 'ConsistentlyDefined<>::m_value may be used uninitialized...'
   }

   /*!
    * Copy constructor.
    *
    * The value instance will be copy-constructed implicitly, which means that
    * the type T must provide valid copy construction semantics if this
    * constructor is invoked.
    */
   ConsistentlyDefined( const ConsistentlyDefined& ) = default;

   /*
    * Move constructor.
    */
   ConsistentlyDefined( ConsistentlyDefined&& ) = default;

   /*!
    * Constructs a defined, consistent %ConsistentlyDefined object with the
    * specified \a value.
    */
   ConsistentlyDefined( const T& value )
      : m_value( value )
      , m_defined( true )
   {
   }

   /*!
    * Assigns the specified \a value to this object. Returns a reference to
    * this object. After assigning a value, a %ConsistentlyDefined object will
    * be in defined/consistent state.
    */
   T& operator =( const T& value )
   {
      m_value = value;
      m_defined = m_consistent = true;
      return m_value;
   }

   /*!
    * Copy assignment operator. Returns a reference to this object.
    *
    * If this object is inconsistent, then this function does nothing but
    * returning a reference to this.
    *
    * If this object is consistent, then:
    *
    * - If \a other is defined and this is defined, then if the values of both
    * objects are different, this object will be in defined/inconsistent state.
    *
    * - If \a other is defined and this is not defined, then the value of
    * \a other will be assigned to this object, and this object will be in
    * defined/consistent state.
    *
    * - If \a other is not defined and this is defined, then this object will
    * be in defined/inconsistent state.
    */
   ConsistentlyDefined& operator =( const ConsistentlyDefined& other )
   {
      if ( m_consistent )
         if ( other.m_defined )
         {
            if ( m_defined )
            {
               if ( m_value != other.m_value )
                  m_consistent = false;
            }
            else
            {
               m_value = other.m_value;
               m_defined = true;
            }
         }
         else
         {
            if ( m_defined )
               m_consistent = false;
         }

      return *this;
   }

   /*!
    * Addition/assignment operator. Returns a reference to this object.
    *
    * If this object is inconsistent, then this function does nothing but
    * returning a reference to this.
    *
    * If this object is consistent, then:
    *
    * - If \a other is defined and this is defined, then the value of \a other
    * will be added to this object's value, and this object will remain in
    * defined/consistent state.
    *
    * - If \a other is defined and this is not defined, then the value of
    * \a other will be assigned to this object, and this object will be in
    * defined/consistent state.
    *
    * - If \a other is not defined and this is defined, then this object will
    * be in defined/inconsistent state.
    */
   ConsistentlyDefined& operator +=( const ConsistentlyDefined& other )
   {
      if ( m_consistent )
         if ( other.m_defined )
         {
            if ( m_defined )
               m_value += other.m_value;
            else
            {
               m_value = other.m_value;
               m_defined = true;
            }
         }
         else
         {
            if ( m_defined )
               m_consistent = false;
         }

      return *this;
   }

   /*!
    * Addition/assignment operator. Returns a reference to this object.
    *
    * If this object is inconsistent, then this function does nothing but
    * returning a reference to this.
    *
    * If this object is consistent, then:
    *
    * - If this object is defined, then \a otherValue will be added to this
    * object's value, and this object will remain in defined/consistent state.
    *
    * - If this object is not defined, then \a otherValue will be assigned to
    * this object, and this object will be in defined/consistent state.
    */
   ConsistentlyDefined& operator +=( const T& otherValue )
   {
      if ( m_consistent )
         if ( m_defined )
            m_value += otherValue;
         else
         {
            m_value = otherValue;
            m_defined = true;
         }

      return *this;
   }

   /*!
    * Returns a reference to the value stored in this object.
    *
    * If this object is undefined, the returned value may be unpredictable,
    * depending on construction semantics for the type T.
    */
   const T& operator ()() const
   {
      return m_value;
   }

   /*!
    * Returns true iff this object has been defined, even if it's inconsistent.
    */
   bool IsDefined() const
   {
      return m_defined;
   }

   /*!
    * Returns true iff this object is consistent, even if it has not been
    * defined.
    */
   bool IsConsistent() const
   {
      return m_consistent;
   }

   /*!
    * If this object is in defined/consistent state, then this function returns
    * true.
    *
    * If this object is in undefined state, this function returns false.
    *
    * If this object is in defined/inconsistent state, this function returns
    * false. In this case, if the \a what string is not empty, a warning
    * message will be written to the core platform's console alerting about the
    * inconsistent state.
    */
   bool IsConsistentlyDefined( const String& what = String() ) const
   {
      if ( m_defined )
      {
         if ( m_consistent )
            return true;
         if ( !what.IsEmpty() )
            Console().WarningLn( "<end><cbr>** Warning: Inconsistent " + what + " value(s) - metadata not generated." );
      }
      return false;
   }

   /*!
    * Force this object to be in undefined state.
    */
   void Undefine()
   {
      m_defined = false;
   }

   /*!
    * Force this object to be in inconsistent state.
    */
   void SetInconsistent()
   {
      m_consistent = false;
   }

   /*!
    * Changes the value in this object in a forcible way, i.e. without checking
    * for consistency.
    */
   void ForceValue( const T& value )
   {
      m_value = value;
   }

   /*!
    * Returns a String representation of the value in this object if it has
    * been defined (even if it's inconsistent). Otherwise returns an empty
    * string.
    */
   String ToString() const
   {
      return m_defined ? String( m_value ) : String();
   }

private:

   T    m_value;
   bool m_defined = false;
   bool m_consistent = true;
};

// ----------------------------------------------------------------------------

#define __PCL_INTEGRATION_METADATA_VERSION   "1.2"

/*
 * Optional, consistently defined metadata of an integrable image.
 *
 * ### N.B. Internal use - Not a public interface.
 */
class IntegrationMetadata
{
public:

   String                         version = __PCL_INTEGRATION_METADATA_VERSION;
   ConsistentlyDefined<String>    author;
   ConsistentlyDefined<String>    observer;
   ConsistentlyDefined<String>    instrumentName;
   ConsistentlyDefined<String>    frameType;
   ConsistentlyDefined<String>    filterName;
   ConsistentlyDefined<IsoString> cfaPatternName;
   ConsistentlyDefined<IsoString> cfaPattern;
   ConsistentlyDefined<int>       cfaXOffset;    // px
   ConsistentlyDefined<int>       cfaYOffset;    // px
   ConsistentlyDefined<double>    pedestal;      // DN
   ConsistentlyDefined<double>    expTime;       // s
   ConsistentlyDefined<double>    sensorTemp;    // C
   ConsistentlyDefined<double>    xPixSize;      // um
   ConsistentlyDefined<double>    yPixSize;      // um
   ConsistentlyDefined<double>    cameraGain;
   ConsistentlyDefined<unsigned>  cameraISO;
   ConsistentlyDefined<unsigned>  xBinning;
   ConsistentlyDefined<unsigned>  yBinning;
   ConsistentlyDefined<unsigned>  xOrigin;       // px
   ConsistentlyDefined<unsigned>  yOrigin;       // px
   ConsistentlyDefined<String>    telescopeName;
   ConsistentlyDefined<double>    focalLength;   // mm
   ConsistentlyDefined<double>    aperture;      // mm
   ConsistentlyDefined<double>    apertureArea;  // mm^2
   ConsistentlyDefined<String>    objectName;
   ConsistentlyDefined<TimePoint> startTime;     // UTC
   ConsistentlyDefined<TimePoint> endTime;       // UTC
   ConsistentlyDefined<double>    ra;            // deg (-180,+180]
   ConsistentlyDefined<double>    dec;           // deg [-90,+90]
   ConsistentlyDefined<IsoString> celCrdSys;     // ICRS, FK5
   ConsistentlyDefined<double>    equinox;       // yr
   ConsistentlyDefined<double>    longObs;       // deg (-180,+180]
   ConsistentlyDefined<double>    latObs;        // deg [-90,+90]
   ConsistentlyDefined<double>    altObs;        // m

   IntegrationMetadata() = default;
   IntegrationMetadata( const IntegrationMetadata& ) = default;

   IntegrationMetadata( const PropertyArray&, const FITSKeywordArray& );
   IntegrationMetadata( const String& serialization );

   String Serialize() const;

   bool IsValid() const
   {
      return m_valid;
   }

   void UpdatePropertiesAndKeywords( PropertyArray&, FITSKeywordArray& ) const;

   static IntegrationMetadata Summary( const Array<IntegrationMetadata>& );

private:

   bool m_valid = false;

   // Block separators for text metadata serialization (UTF-16).
   constexpr static char16_type ItemSeparator = char16_type( 0x2028 );  // Unicode Line Separator
   constexpr static char16_type TokenSeparator = char16_type( 0x2029 ); // Unicode Paragraph Separator
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_IntegrationMetadata_h

// ----------------------------------------------------------------------------
// EOF pcl/IntegrationMetadata.h - Released 2020-12-17T15:46:29Z
