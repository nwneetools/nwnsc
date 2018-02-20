//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NwnScript.cpp - Script support routines |
//
// This module contains the definition of the script support routines.
//
// Copyright (c) 2002-2003 - Edward T. Smith
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
// $History: ScriptRawView.cpp $
//      
//-----------------------------------------------------------------------------

#include "Precomp.h"
#include "Nsc.h"

//-----------------------------------------------------------------------------
//
// @func Get a 4 byte long from the data
//
// @parm unsigned char * | pData | Pointer to the data
//
// @parm unsigned long * | pul | Pointer to the destination
//
// @rdesc Updated address.
//
//-----------------------------------------------------------------------------

static unsigned char *GetUINT32 (unsigned char *pData, unsigned long *pul)
{
	*pul = CNwnByteOrder<UINT32>::BigEndian (pData);
	return &pData [4];
}

//-----------------------------------------------------------------------------
//
// @func Get a 2 byte long from the data
//
// @parm unsigned char * | pData | Pointer to the data
//
// @parm unsigned long * | pul | Pointer to the destination
//
// @rdesc Updated address.
//
//-----------------------------------------------------------------------------

static unsigned char *GetUINT16 (unsigned char *pData, unsigned long *pul)
{
	*pul = CNwnByteOrder<UINT16>::BigEndian (pData);
	return &pData [2];
}

//-----------------------------------------------------------------------------
//
// @func Get a floating point value from the data
//
// @parm unsigned char * | pData | Pointer to the data
//
// @parm unsigned long * | pul | Pointer to the destination
//
// @rdesc Updated address.
//
//-----------------------------------------------------------------------------

static unsigned char *GetFLOAT (unsigned char *pData, float *pf)
{
	*pf = CNwnByteOrder<float>::BigEndian (pData);
	return &pData [4];
}

//-----------------------------------------------------------------------------
//
// @func Get the operator text
//
// @parm unsigned char | cOpType | Operator type
//
// @parm char * | szOpText | Output
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

static void GetOpText (unsigned char cOpType, char *szOpText)
{
	switch (cOpType)
	{
		case 0x03:
			strcpy (szOpText, "I");
			break;

		case 0x04:
			strcpy (szOpText, "F");
			break;

		case 0x05:
			strcpy (szOpText, "S");
			break;

		case 0x06:
			strcpy (szOpText, "O");
			break;

		case 0x10:
			strcpy (szOpText, "EFF");
			break;

		case 0x11:
			strcpy (szOpText, "EVNT");
			break;

		case 0x12:
			strcpy (szOpText, "LOC");
			break;

		case 0x13:
			strcpy (szOpText, "TAL");
			break;

		case 0x20:
			strcpy (szOpText, "II");
			break;

		case 0x21:
			strcpy (szOpText, "FF");
			break;

		case 0x22:
			strcpy (szOpText, "OO");
			break;

		case 0x23:
			strcpy (szOpText, "SS");
			break;

		case 0x24:
			strcpy (szOpText, "TT");
			break;

		case 0x25:
			strcpy (szOpText, "IF");
			break;

		case 0x26:
			strcpy (szOpText, "FI");
			break;

		case 0x30:
			strcpy (szOpText, "EFFEFF");
			break;

		case 0x3A:
			strcpy (szOpText, "VV");
			break;

		case 0x3B:
			strcpy (szOpText, "VF");
			break;

		case 0x3C:
			strcpy (szOpText, "FV");
			break;

		default:
			sprintf (szOpText, "P%d", cOpType);
			//if (g_fpDebug)
			//	fprintf (g_fpDebug, "Unknown optype %02X\r\n", cOpType);
			break;
	}
}

//-----------------------------------------------------------------------------
//
// @func Dump a script
//
// @parm CNwnStream & | sStream | Destination stream
//
// @parm unsigned char * | pStart | Start of the data
//
// @parm unsigned char * | pEnd | End of the data
//
// @param NscCompiler * | pCompiler | Compiler instance
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscScriptDecompile (CNwnStream &sStream, 
	unsigned char *pauchData, unsigned long ulSize, NscCompiler *pCompiler)
{

	//
	// Loop through the data
	//

	unsigned char *pStart = pauchData;
	unsigned char *pEnd = &pauchData [ulSize];
	unsigned char *pData = pStart;
	pData += 8;
	while (pData < pEnd)
	{
		char szByteText [128];
		char szOpText [512];
		char szOpType [32];
		char *pszOpRoot;

		//
		// Switch based on the next opcode
		//

		unsigned char *pOp = pData;
		unsigned char cOp = *pData++;
		switch (cOp)
		{

			case NscCode_CPDOWNSP:
				{
					if (&pData [7] > pEnd || pData [0] != 1)
						goto invalid_op;
					unsigned long ul1, ul2;
					pData = GetUINT32 (pData + 1, &ul1);
					pData = GetUINT16 (pData, &ul2);
					sprintf (szByteText, "%02X 01 %08X %04X", cOp, ul1, ul2);
					sprintf (szOpText, "CPDOWNSP %08X, %04X", ul1, ul2);
				}
				break;

			case NscCode_RSADD:
				pszOpRoot = "RSADD";
do_simple_operator:;
				{
					if (&pData [1] > pEnd)
						goto invalid_op;
					unsigned char cOpType = *pData++;
					GetOpText (cOpType, szOpType);
					sprintf (szByteText, "%02X %02X", cOp, cOpType);
					sprintf (szOpText, "%s%s", pszOpRoot, szOpType);
				}
				break;

			case NscCode_CPTOPSP:
				{
					if (&pData [7] > pEnd || pData [0] != 1)
						goto invalid_op;
					unsigned long ul1, ul2;
					pData = GetUINT32 (pData + 1, &ul1);
					pData = GetUINT16 (pData, &ul2);
					sprintf (szByteText, "%02X 01 %08X %04X", cOp, ul1, ul2);
					sprintf (szOpText, "CPTOPSP %08X, %04X", ul1, ul2);
				}
				break;

			case NscCode_CONST:
				{
					if (&pData [1] > pEnd)
						goto invalid_op;
					unsigned char cOpType = *pData;
					GetOpText (cOpType, szOpType);
					switch (cOpType)
					{
						case 3:
						case 6:
							{
								if (&pData [5] > pEnd)
									goto invalid_op;
								unsigned long ul;
								pData = GetUINT32 (pData + 1, &ul);
								sprintf (szByteText, "%02X %02X %08X", cOp, cOpType, ul);
								sprintf (szOpText, "CONST%s %08X", szOpType, ul);
							}
							break;

						case 4:
							{
								if (&pData [5] > pEnd)
									goto invalid_op;
								union
								{
									unsigned long ul;
									float f;
								} val;
								pData = GetFLOAT (pData + 1, &val .f);
								sprintf (szByteText, "%02X %02X %08X", cOp, cOpType, val .ul);
								sprintf (szOpText, "CONST%s %f", szOpType, val .f);
							}
							break;

						case 5:
							{
								if (&pData [3] > pEnd)
									goto invalid_op;
								unsigned long ul;
								GetUINT16 (pData + 1, &ul);
								if (&pData [3 + ul] > pEnd)
									goto invalid_op;
								pData += 3;
								char szValue [129];
								unsigned long ulCopy = ul > 128 ? 128 : ul;
								memmove (szValue, pData, ulCopy);
								szValue [ulCopy] = 0;
								sprintf (szByteText, "%02X %02X %04X str", cOp, cOpType, ul);
								snprintf (szOpText, _countof (szOpText), 
									"CONST%s \"%s\"", szOpType, szValue);
								pData += ul;
							}
							break;

						default:
							goto invalid_op;
					}
				}
				break;

			case NscCode_ACTION:
				{
					if (&pData [4] > pEnd || pData [0] != 0)
						goto invalid_op;
					unsigned long ul1, ul2;
					pData = GetUINT16 (pData + 1, &ul1);
					ul2 = *pData++;
					const char *pszName = NscGetActionName ((int) ul1, pCompiler);
					sprintf (szByteText, "%02X 00 %04X %02X", cOp, ul1, ul2);
					sprintf (szOpText, "ACTION %s(%04X), %02X", pszName, ul1, ul2);
				}
				break;

			case NscCode_LOGAND:
				pszOpRoot = "LOGAND";
do_binary_operator:;
				{
					if (&pData [1] > pEnd)
						goto invalid_op;
					unsigned char cOpType = *pData++;
					GetOpText (cOpType, szOpType);
					if (cOpType == 0x24)
					{
						if (&pData [2] > pEnd)
							goto invalid_op;
						unsigned long ul1;
						pData = GetUINT16 (pData, &ul1);
						sprintf (szByteText, "%02X %02X %04X", cOp, cOpType, ul1);
						sprintf (szOpText, "%s%s %04X", pszOpRoot, szOpType, ul1);
					}
					else
					{
						sprintf (szByteText, "%02X %02X", cOp, cOpType);
						sprintf (szOpText, "%s%s", pszOpRoot, szOpType);
					}
				}
				break;

			case NscCode_LOGOR:
				pszOpRoot = "LOGOR";
				goto do_binary_operator;

			case NscCode_INCOR:
				pszOpRoot = "INCOR";
				goto do_binary_operator;

			case NscCode_EXCOR:
				pszOpRoot = "EXCOR";
				goto do_binary_operator;

			case NscCode_BOOLAND:
				pszOpRoot = "BOOLAND";
				goto do_binary_operator;

			case NscCode_EQUAL:
				pszOpRoot = "EQUAL";
				goto do_binary_operator;

			case NscCode_NEQUAL:
				pszOpRoot = "NEQUAL";
				goto do_binary_operator;

			case NscCode_GEQ:
				pszOpRoot = "GEQ";
				goto do_binary_operator;

			case NscCode_GT:
				pszOpRoot = "GT";
				goto do_binary_operator;

			case NscCode_LT:
				pszOpRoot = "LT";
				goto do_binary_operator;

			case NscCode_LEQ:
				pszOpRoot = "LEQ";
				goto do_binary_operator;

			case NscCode_SHLEFT:
				pszOpRoot = "SHLEFT";
				goto do_binary_operator;

			case NscCode_SHRIGHT:
				pszOpRoot = "SHRIGHT";
				goto do_binary_operator;

			case NscCode_USHRIGHT:
				pszOpRoot = "USHRIGHT";
				goto do_binary_operator;

			case NscCode_ADD:
				pszOpRoot = "ADD";
				goto do_binary_operator;

			case NscCode_SUB:
				pszOpRoot = "SUB";
				goto do_binary_operator;

			case NscCode_MUL:
				pszOpRoot = "MUL";
				goto do_binary_operator;

			case NscCode_DIV:
				pszOpRoot = "DIV";
				goto do_binary_operator;

			case NscCode_MOD:
				pszOpRoot = "MOD";
				goto do_binary_operator;

			case NscCode_NEG:
				pszOpRoot = "NEG";
				goto do_simple_operator;

			case NscCode_COMP:
				pszOpRoot = "COMP";
				goto do_simple_operator;

			case NscCode_MOVSP:
				{
					if (&pData [5] > pEnd || pData [0] != 0)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 00 %08X", cOp, ul1);
					sprintf (szOpText, "MOVSP %08X", ul1);
				}
				break;

			case NscCode_STORE_STATEALL:
				{
					if (&pData [1] > pEnd)
						goto invalid_op;
					unsigned long ul = *pData++;
					sprintf (szByteText, "%02X %02X", cOp, ul);
					sprintf (szOpText, "SAVE_STATEALL %02x", ul);
				}
				break;

			case NscCode_JMP:
				{
					if (&pData [5] > pEnd || pData [0] != 0)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 00 %08X", cOp, ul1);
					sprintf (szOpText, "JMP off_%08X", (pOp - pStart) + ul1);
				}
				break;

			case NscCode_JSR:
				{
					if (&pData [5] > pEnd || pData [0] != 0)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 00 %08X", cOp, ul1);
					sprintf (szOpText, "JSR fn_%08X", (pOp - pStart) + ul1);
				}
				break;

			case NscCode_JZ:
				{
					if (&pData [5] > pEnd || pData [0] != 0)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 00 %08X", cOp, ul1);
					sprintf (szOpText, "JZ off_%08X", (pOp - pStart) + ul1);
				}
				break;

			case NscCode_RETN:
				{
					if (&pData [1] > pEnd || pData [0] != 0)
						goto invalid_op;
					pData++;
					sprintf (szByteText, "%02X 00", cOp);
					sprintf (szOpText, "RETN");
				}
				break;

			case NscCode_DESTRUCT:
				{
					if (&pData [7] > pEnd || pData [0] != 1)
						goto invalid_op;
					unsigned long ul1, ul2, ul3;
					pData = GetUINT16 (pData + 1, &ul1);
					pData = GetUINT16 (pData, &ul2);
					pData = GetUINT16 (pData, &ul3);
					sprintf (szByteText, "%02X 01 %04X %04X %04x", cOp, ul1, ul2, ul3);
					sprintf (szOpText, "DESTRUCT %04X, %04X, %04X", ul1, ul2, ul3);
					// First parameter, number of bytes to destroy
					// Second parameter, offset of element not to destroy
					// Third parameter, number of bytes no to destroy
				}
				break;

			case NscCode_NOT:
				pszOpRoot = "NOT";
				goto do_simple_operator;

			case NscCode_DECISP:
				{
					if (&pData [5] > pEnd || pData [0] != 3)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 03 %08X", cOp, ul1);
					sprintf (szOpText, "DECISP %08X", ul1);
				}
				break;

			case NscCode_INCISP:
				{
					if (&pData [5] > pEnd || pData [0] != 3)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 03 %08X", cOp, ul1);
					sprintf (szOpText, "INCISP %08X", ul1);
				}
				break;

			case NscCode_JNZ:
				{
					if (&pData [5] > pEnd || pData [0] != 0)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 00 %08X", cOp, ul1);
					sprintf (szOpText, "JNZ off_%08X", (pOp - pStart) + ul1);
				}
				break;

			case NscCode_CPDOWNBP:
				{
					if (&pData [7] > pEnd || pData [0] != 1)
						goto invalid_op;
					unsigned long ul1, ul2;
					pData = GetUINT32 (pData + 1, &ul1);
					pData = GetUINT16 (pData, &ul2);
					sprintf (szByteText, "%02X 01 %08X %04X", cOp, ul1, ul2);
					sprintf (szOpText, "CPDOWNBP %08X, %04X", ul1, ul2);
				}
				break;

			case NscCode_CPTOPBP:
				{
					if (&pData [7] > pEnd || pData [0] != 1)
						goto invalid_op;
					unsigned long ul1, ul2;
					pData = GetUINT32 (pData + 1, &ul1);
					pData = GetUINT16 (pData, &ul2);
					sprintf (szByteText, "%02X 01 %08X %04X", cOp, ul1, ul2);
					sprintf (szOpText, "CPTOPBP %08X, %04X", ul1, ul2);
				}
				break;

			case NscCode_DECIBP:
				{
					if (&pData [5] > pEnd || pData [0] != 3)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 03 %08X", cOp, ul1);
					sprintf (szOpText, "DECIBP %08X", ul1);
				}
				break;

			case NscCode_INCIBP:
				{
					if (&pData [5] > pEnd || pData [0] != 3)
						goto invalid_op;
					unsigned long ul1;
					pData = GetUINT32 (pData + 1, &ul1);
					sprintf (szByteText, "%02X 03 %08X", cOp, ul1);
					sprintf (szOpText, "INCIBP %08X", ul1);
				}
				break;

			case NscCode_SAVEBP:
				{
					if (&pData [1] > pEnd || pData [0] != 0)
						goto invalid_op;
					pData++;
					sprintf (szByteText, "%02X 00", cOp);
					sprintf (szOpText, "SAVEBP");
				}
				break;

			case NscCode_RESTOREBP:
				{
					if (&pData [1] > pEnd || pData [0] != 0)
						goto invalid_op;
					pData++;
					sprintf (szByteText, "%02X 00", cOp);
					sprintf (szOpText, "RESTOREBP");
				}
				break;

			case NscCode_STORE_STATE:
				{
					if (&pData [9] > pEnd)
						goto invalid_op;
					unsigned long ul1, ul2, ul3;
					ul3 = *pData++;
					pData = GetUINT32 (pData, &ul1);
					pData = GetUINT32 (pData, &ul2);
					sprintf (szByteText, "%02X %02X %08X %08X", cOp, ul3, ul1, ul2);
					sprintf (szOpText, "STORE_STATE %02X, %08X, %08X", ul3, ul1, ul2);
				    // First value is BP stack size to save
					// second value is SP stack size to save
				}
				break;

			case NscCode_NOP:
				{
					if (&pData [1] > pEnd || pData [0] != 0)
						goto invalid_op;
					pData++;
					sprintf (szByteText, "%02X 00", cOp);
					sprintf (szOpText, "NOP");
				}
				break;

			case NscCode_Size:
				{
					if (&pData [4] > pEnd)
						goto invalid_op;
					unsigned long ul;
					pData = GetUINT32 (pData, &ul);
					sprintf (szByteText, "%02X %08X", cOp, ul);
					sprintf (szOpText, "T %08X", ul);
				}
				break;

			default:
invalid_op:;
				sprintf (szByteText, "%02X", cOp);
				sprintf (szOpText, "??");
				//if (g_fpDebug)
				//	fprintf (g_fpDebug, "Unknown opcode %02x\r\n", cOp);
				break;
		}

		//
		// Format the final line
		//

		char szText [1024];
		sprintf (szText, "%08X %-24s %s", pOp - pStart, szByteText, szOpText);
		sStream .WriteLine (szText);
	}
}

#ifdef XXX
01 01 4BYTE 2BYTE - CPDOWNSP x, y
02 03 - RSADDI(x)
03 01 4BYTE 2BYTE - CPTOPSP x, y
04 03 4BYTE - CONSTI x
04 04 4BYTE - CONSTF x
04 05 2BYTE DATA - CONSTS str
04 06 4BYTE - CONSTO x
05 00 2BYTE 1BYTE - ACTION x y

06 20 - LOGANDII
07 20 - LOGORII
08 20 - INCORII
09 20 - EXCORII
0A 20 - BOOLANDII
0B 20 - EQUALII
0C 20 - NEQUALII
0D xx - GEQ
0E xx - GT
0F xx - LT
10 xx - LEQ
11 20 - SHLEFT
12 20 - SHRIGHT
13 20 - USHRIGHTII
14 xx - ADD
15 xx - SUB
16 xx - MUL
17 xx - DIV
18 20 - MODII
19 03 - NEGI
19 04 - NEGF
1A 03 - COMPI
1B 00 4BYTE - MOVSP x
1D 00 - JMP offset
1E 00 OFFSET - JSR fn_%08x
1F 00 OFFSET - JZ offset

20 00 - RETN
21 00 2BYTE 2BYTE 2BYTE - DESTRUCT %04x,%04x,%04x
22 03 - NOTI
23 03 4BYTE - DECISP x
24 03 4BYTE - INCISP x
25 00 - JNZ offset
26 01 4BYTE 2BYTE - CPDOWNBP x, y
27 01 4BYTE 2BYTE - CPTOPBP x, y
28 03 4BYTE - DECI x
29 03 4BYTE - INCI x
2A 00 - SAVEBP
2B 00 - RESTOREBP
2C 10 4BYTE 4BYTE - STORE_STATE 0x10 x y
42 LENGTH-MSB - T %08x

3 - INT
4 - REAL
5 - STRING
6 - OBJECT
20 - II
21 - FF
22 - 00
23 - SS
24 - TT
25 - IF
26 - FI
?? - VV
?? - VF
?? - FV

1c??
#endif

