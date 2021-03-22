//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/Win32Exception.cpp - Released 2020-12-17T15:46:35Z
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

#ifdef __PCL_WINDOWS

#include <pcl/AutoLock.h>
#include <pcl/Win32Exception.h>

#include <eh.h>

#ifndef __PCL_WINDOWS_NO_BACKTRACE

#pragma warning( push )
#pragma warning( disable: 4091 ) // typedef ignored on left of <unnamed-enum...>
#include <dbghelp.h>
#pragma warning( pop )

#pragma comment( lib, "dbghelp.lib" )

#endif // !__PCL_WINDOWS_NO_BACKTRACE

namespace pcl
{

// ----------------------------------------------------------------------------

#define DATA   reinterpret_cast<const EXCEPTION_RECORD*>( m_data )

// ----------------------------------------------------------------------------

Win32Exception::exception_address Win32Exception::ExceptionAddress() const
{
   return DATA->ExceptionAddress;
}

// ----------------------------------------------------------------------------

static void My_se_translator( unsigned int /*code*/, EXCEPTION_POINTERS* pointers )
{
   if ( pointers->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE )
      throw pcl::FatalError( "Noncontinuable system exception" );

   IsoString details;

#ifndef __PCL_WINDOWS_NO_BACKTRACE
   {
      static Mutex mutex;
      volatile AutoLock lock( mutex );

      HANDLE process = GetCurrentProcess();

//       CONTEXT* context = pointers->ContextRecord;
      CONTEXT context;
      RtlCaptureContext( &context );

      STACKFRAME stack = {};
#if _WIN64
      stack.AddrPC.Offset = context.Rip;
      stack.AddrPC.Mode = AddrModeFlat;
      stack.AddrFrame.Offset = context.Rbp;
      stack.AddrFrame.Mode = AddrModeFlat;
      stack.AddrStack.Offset = context.Rsp;
      stack.AddrStack.Mode = AddrModeFlat;
#else
      stack.AddrPC.Offset = context.Eip;
      stack.AddrPC.Mode = AddrModeFlat;
      stack.AddrFrame.Offset = context.Ebp;
      stack.AddrFrame.Mode = AddrModeFlat;
      stack.AddrStack.Offset = context.Esp;
      stack.AddrStack.Mode = AddrModeFlat;
#endif

      details << "\n*** Backtrace Information ***\n"
            << IsoString( '=', 80 ) << '\n';

      HANDLE thread  = GetCurrentThread();

      char buffer[ sizeof( SYMBOL_INFO ) + MAX_SYM_NAME*sizeof( TCHAR ) ];

      for ( unsigned frame = 0; ; ++frame )
      {
         if ( !StackWalk64(
#if defined( _M_X64 )
               IMAGE_FILE_MACHINE_AMD64
#else
               IMAGE_FILE_MACHINE_I386
#endif
               ,
               process, thread, &stack, &context,
               NULL/*ReadMemoryRoutine*/,
               SymFunctionTableAccess64,
               SymGetModuleBase64,
               NULL/*TranslateAddress*/ ) )
         {
            break;
         }

         PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
         pSymbol->SizeOfStruct = sizeof( SYMBOL_INFO );
         pSymbol->MaxNameLen = MAX_SYM_NAME;
         DWORD64 displacement = 0;
         SymFromAddr( process, (ULONG64)stack.AddrPC.Offset, &displacement, pSymbol );

         IMAGEHLP_LINE64* line = (IMAGEHLP_LINE64*)malloc( sizeof( IMAGEHLP_LINE64 ) );
         line->SizeOfStruct = sizeof( IMAGEHLP_LINE64 );

         details.AppendFormat( "%3u: ", frame );

         DWORD disp;
         if ( SymGetLineFromAddr64( process, stack.AddrPC.Offset, &disp, line ) )
         {
            details.AppendFormat( "%s in module: %s line: %lu at address: 0x%0X",
                                 pSymbol->Name, line->FileName, line->LineNumber, pSymbol->Address );
         }
         else
         {
            HMODULE hModule = NULL;
            GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCTSTR)stack.AddrPC.Offset, &hModule );
            if ( hModule != NULL )
            {
               char module[ MAX_PATH ] = {};
               GetModuleFileNameA( hModule, module, MAX_PATH );
               details.AppendFormat( "%s in module: %s at address: 0x%0X", pSymbol->Name, module, pSymbol->Address );
            }
            else
               details.AppendFormat( "%s at address: 0x%0X", pSymbol->Name, pSymbol->Address );
         }

         details << '\n';

         free( line );
      }

      details << IsoString( '=', 80 ) << '\n';
   }
#endif // !__PCL_WINDOWS_NO_BACKTRACE

   Win32Exception::exception_code code = pointers->ExceptionRecord->ExceptionCode;
   Win32Exception::exception_data_pointer data = pointers->ExceptionRecord;

   switch ( code )
   {
   case EXCEPTION_ACCESS_VIOLATION:
      throw Win32AccessViolationException( code, data, details );
      break;

   case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      throw EWin32ArrayBoundsExceeded( code, data, details );
      break;

   case EXCEPTION_BREAKPOINT:
      throw EWin32Breakpoint( code, data, details );
      break;

   case EXCEPTION_DATATYPE_MISALIGNMENT:
      throw EWin32DataMisalignment( code, data, details );
      break;

   case EXCEPTION_FLT_DENORMAL_OPERAND:
      throw EWin32FloatingPointDenormalOperand( code, data, details );
      break;

   case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      throw EWin32FloatingPointDivideByZero( code, data, details );
      break;

   case EXCEPTION_FLT_INEXACT_RESULT:
      throw EWin32FloatingPointInexactResult( code, data, details );
      break;

   case EXCEPTION_FLT_INVALID_OPERATION:
      throw EWin32FloatingPointInvalidOperation( code, data, details );
      break;

   case EXCEPTION_FLT_OVERFLOW:
      throw EWin32FloatingPointOverflow( code, data, details );
      break;

   case EXCEPTION_FLT_STACK_CHECK:
      throw EWin32FloatingPointStackCheck( code, data, details );
      break;

   case EXCEPTION_FLT_UNDERFLOW:
      throw EWin32FloatingPointUnderflow( code, data, details );
      break;

   case EXCEPTION_ILLEGAL_INSTRUCTION:
      throw EWin32IllegalInstruction( code, data, details );
      break;

   case EXCEPTION_IN_PAGE_ERROR:
      throw EWin32PageError( code, data, details );
      break;

   case EXCEPTION_INT_DIVIDE_BY_ZERO:
      throw EWin32DivideByZero( code, data, details );
      break;

   case EXCEPTION_INT_OVERFLOW:
      throw EWin32Overflow( code, data, details );
      break;

   case EXCEPTION_INVALID_DISPOSITION:
      throw EWin32InvalidDisposition( code, data, details );
      break;

   case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      throw EWin32NonContinuableException( code, data, details );
      break;

   case EXCEPTION_PRIV_INSTRUCTION:
      throw EWin32PrivilegedInstruction( code, data, details );
      break;

   case EXCEPTION_SINGLE_STEP:
      throw EWin32SingleStep( code, data, details );
      break;

   case EXCEPTION_STACK_OVERFLOW:
      throw EWin32StackOverflow( code, data, details );
      break;

   default:
      throw Win32Exception( code, data, details );
      break;
   }
}

// ----------------------------------------------------------------------------

void Win32Exception::Show() const
{
   /*
    * N.B.: The following function call must be expanded inline.
    */
   ShowOnConsole();
}

// ----------------------------------------------------------------------------

void Win32Exception::Initialize()
{
#ifndef __PCL_WINDOWS_NO_BACKTRACE
   {
      static bool initialized = false;
      static Mutex mutex;
      volatile AutoLock lock( mutex );

      HANDLE process = GetCurrentProcess();
      if ( initialized )
         SymCleanup( process );
      SymInitialize( process, NULL/*UserSearchPath*/, TRUE/*fInvadeProcess*/ );
      initialized = true;
   }
#endif

   _set_se_translator( My_se_translator );
}

// ----------------------------------------------------------------------------

String Win32AccessViolationException::Message() const
{
   return String().Format( "Access violation: invalid memory %s operation at address %p",
                           (DATA->ExceptionInformation[0] == 0) ? "read" : "write",
                           DATA->ExceptionInformation[1] );
}

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_WINDOWS

// ----------------------------------------------------------------------------
// EOF pcl/Win32Exception.cpp - Released 2020-12-17T15:46:35Z
