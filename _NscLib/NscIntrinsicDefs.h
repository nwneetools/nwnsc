//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscIntrinsicDefs.h - Script built-in intrinsic definitions |
//
// This module contains the definition of the script support intrinsics that
// are injected into nwscript.nss compilation
//
// Copyright (c) 2008-2011 - Ken Johnson (Skywing)
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are 
// met:
// 
// 1. Redistributions of source code must retain the above copyright notice, 
//    this list of conditions and the following disclaimer. 
// 2. Neither the name of Edward T. Smith nor the names of its contributors 
//    may be used to endorse or promote products derived from this software 
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// @end
//
// $History: NscIntrinsicDefs.h $
//      
//-----------------------------------------------------------------------------

//
// Globals
//

static const char g_szNscIntrinsicsText [] =
"/*++\n"
"\n"
"Copyright (c) Ken Johnson (Skywing). All rights reserved.\n"
"\n"
"Module Name:\n"
"\n"
"	NscIntrinsics.nss\n"
"\n"
"Abstract:\n"
"\n"
"	This module houses definitions related to compiler internal intrinsics.\n"
"\n"
"--*/\n"
"\n"
"#pragma nsc_intrinsics\n"
"\n"
"int __readbp ();\n"
"void __writebp (int BP);\n"
"int __readrelativesp ();\n"
"int __readsp ();\n"
"int __readpc ();\n"
"\n";

static const size_t g_nNscIntrinsicsTextSize = sizeof (g_szNscIntrinsicsText) -
	sizeof (g_szNscIntrinsicsText [0]);
