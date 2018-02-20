#include "BufferParser.h"

using swutil::BufferParser;

//
// Define to 1 to break into the debugger on parse failure.
//

#define BUFFERPARSE_BREAK_ON_FAIL 0

BufferParser::BufferParser(
	 const void *Data,
	 size_t Length,
	 BitOrderMode BitOrder /* = BitOrderLowToHigh */
	)
	: m_Data( reinterpret_cast< const unsigned char * >( Data ) ),
	  m_DataLength( Length ),
	  m_DataPos( m_Data ),
	  m_DataPosRemaining( m_DataLength ),
	  m_BitPos( 8 ),
	  m_HighestValidBitPos( 8 ),
	  m_BitOrderMode( BitOrder )
{
}

BufferParser::~BufferParser()
{
}

bool
BufferParser::GetDataPtr(
	 size_t FieldLength,
	 const void **Field
	)
{
	if (m_DataPosRemaining < FieldLength)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	if (m_BitPos != 8)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	//
	// If we have an output pointer, update it to point into the packet.
	// Otherwise, we'll just advance the buffer pointer and discard the data.
	//

	if (Field)
		*Field = reinterpret_cast< const void *>( m_DataPos );

	m_DataPosRemaining -= FieldLength;
	m_DataPos          += FieldLength;

	return true;
}

bool
BufferParser::GetData(
	 size_t FieldLength,
	 void *Field
	)
{
	if (m_DataPosRemaining < FieldLength)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	if (m_BitPos != 8)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	//
	// If we have an output pointer, update it to point into the packet.
	// Otherwise, we'll just advance the buffer pointer and discard the data.
	//

	if (Field)
	{
		memcpy( Field, m_DataPos, FieldLength );
	}

	m_DataPosRemaining -= FieldLength;
	m_DataPos          += FieldLength;

	return true;
}

bool
BufferParser::GetFieldBits(
	 size_t NumBits,
	 uint64_t &FieldBits
	)
{
	if (NumBits > 64)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	//
	// No bytes left?  All done.
	//

	if (!m_DataPosRemaining)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	//
	// Calculate the number of bits remaining in this byte.  This is normally
	// 8, but if we are the last byte, then we may have a lesser set of valid
	// bits.
	//

	size_t BitsThisByte  = ((m_DataPosRemaining > 1) ? 8 : m_HighestValidBitPos);
	size_t BitsRemaining = BitsThisByte - ((m_BitPos == 8) ? 0 : m_BitPos);
	size_t CurrOutBit    = 0;

	if (NumBits > BitsRemaining)
	{
		size_t BitsExtra = NumBits - BitsRemaining;
		size_t BytesRequired = 1 + (BitsExtra / 8) + ((BitsExtra % 8) ? 1 : 0);

		//
		// N.B.  1 + above as m_DataPosRemaining includes the current byte that
		// is being worked on.
		//

		if (m_DataPosRemaining < BytesRequired)
		{
#if BUFFERPARSE_BREAK_ON_FAIL
			__debugbreak( );
#endif
			return false;
		}
		
		//
		// If we are going to be operating on the last byte, then ensure that
		// we don't extend past the last valid bit.
		//

		if ((m_DataPosRemaining == BytesRequired) &&
		    ((BitsExtra % 8) > m_HighestValidBitPos))
		{
#if BUFFERPARSE_BREAK_ON_FAIL
			__debugbreak( );
#endif
			return false;
		}
	}

	FieldBits = 0;

	while (CurrOutBit < NumBits)
	{
		//
		// If we haven't claimed any data for bit reading yet, then we shall
		// need to do so first.  Note that after having done this we need to
		// keep on reading bits until we finish with a byte before the normal
		// byte-oriented read operations will function.
		//

		if (m_BitPos == 8)
			m_BitPos = 0;

		//
		// Grab the current bit.
		//

		switch (m_BitOrderMode)
		{

		case BitOrderLowToHigh:
			FieldBits <<= 1;
			FieldBits  |= static_cast< uint64_t >( ((*m_DataPos >> (    m_BitPos)) & 1ull) );
//			FieldBits |= static_cast< uint64_t >( (*m_DataPos & (1ull << (    m_BitPos))) << CurrOutBit );
			break;

		case BitOrderHighToLow:
			FieldBits <<= 1;
			FieldBits  |= static_cast< uint64_t >( ((*m_DataPos >> (7 - m_BitPos)) & 1ull) );
//			FieldBits |= static_cast< uint64_t >( (*m_DataPos & (1ull << (7 - m_BitPos))) << CurrOutBit );
			break;

		default:
#ifdef _MSC_VER
			__assume( 0 );
#endif
			break;

		}

		CurrOutBit += 1;
		m_BitPos   += 1;

		//
		// If we're at the last bit in this byte, then increment the current
		// read position, but don't claim the first bit just yet.  This allows
		// us to use byte-level addressing until we're called to read a sub-
		// byte quantity once more.
		//

		if (m_BitPos == 8)
		{
			m_DataPos          += 1;
			m_DataPosRemaining -= 1;
		}

	}

	return true;
}

bool
BufferParser::SkipData(
	 size_t FieldLength
	)
{
	if (m_BitPos != 8)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	if (m_DataPosRemaining < FieldLength)
	{
#if BUFFERPARSE_BREAK_ON_FAIL
		__debugbreak( );
#endif
		return false;
	}

	m_DataPosRemaining -= FieldLength;
	m_DataPos          += FieldLength;

	return true;
}

bool
BufferParser::SkipBits(
	 size_t NumBits
	)
{

	//
	// Calculate the number of bits remaining in this byte.  This is normally
	// 8, but if we are the last byte, then we may have a lesser set of valid
	// bits.
	//

	size_t BitsThisByte  = ((m_DataPosRemaining > 1) ? 8 : m_HighestValidBitPos);
	size_t BitsRemaining = BitsThisByte - ((m_BitPos == 8) ? 0 : m_BitPos);

	if (NumBits > BitsRemaining)
	{
		size_t BitsExtra = NumBits - BitsRemaining;
		size_t BytesRequired = 1 + (BitsExtra / 8) + ((BitsExtra % 8) ? 1 : 0);

		//
		// N.B.  1 + above as m_DataPosRemaining includes the current byte that
		// is being worked on.
		//

		if (m_DataPosRemaining < BytesRequired)
		{
#if BUFFERPARSE_BREAK_ON_FAIL
			__debugbreak( );
#endif
			return false;
		}
		
		//
		// If we are going to be operating on the last byte, then ensure that
		// we don't extend past the last valid bit.
		//

		if ((m_DataPosRemaining == BytesRequired) &&
		    ((BitsExtra % 8) > m_HighestValidBitPos))
		{
#if BUFFERPARSE_BREAK_ON_FAIL
			__debugbreak( );
#endif
			return false;
		}

		m_DataPosRemaining -= BytesRequired;
		m_DataPos          += BytesRequired;
		m_BitPos            = BitsExtra % 8;
	}
	else
	{
		if (m_BitPos == 8)
			m_BitPos = 0;

		m_BitPos += NumBits;
	}

	return true;
}

void
BufferParser::Reset(
	)
{
	m_DataPos            = m_Data;
	m_DataPosRemaining   = m_DataLength;
	m_BitPos             = 8;
	m_HighestValidBitPos = 8;
}

void
BufferParser::SetHighestValidBitPos(
	 size_t HighestValidBitPos
	)
{
//	if (HighestValidBitPos > 8)
//		__debugbreak( );
	m_HighestValidBitPos = HighestValidBitPos;
}

void
BufferParser::RebaseBuffer(
	 const void *Data
	)
{
	ptrdiff_t PtrDiff;

	PtrDiff = (ptrdiff_t) Data - (ptrdiff_t) m_Data;

	m_Data = (const unsigned char *) ((ptrdiff_t) m_Data + PtrDiff);
	m_DataPos = (const unsigned char *) ((ptrdiff_t) m_DataPos + PtrDiff);
}
