#ifndef _SOURCE_PROGRAMS_SKYWINGUTILS_PARSERS_BUFFERPARSER_H
#define _SOURCE_PROGRAMS_SKYWINGUTILS_PARSERS_BUFFERPARSER_H

#include "Precomp.h"

#ifdef _MSC_VER
#pragma once
#elif(__linux__)
#include <unistd.h>
#include <string.h>
#include <cstddef>
#endif

namespace swutil
{
	class IBufferParser
	{

	public:

		template< class T >
		inline
		bool
		GetField(
			 T &Field
			)
		{
			return GetData(
				sizeof( T ),
				&Field
				);
		}

		virtual
		bool
		GetDataPtr(
			 size_t FieldLength,
			 const void **Field
			) = 0;

		virtual
		bool
		GetData(
			 size_t FieldLength,
			 void *Field
			) = 0;

		inline
		bool
		GetFieldBit(
			 bool &FieldBit
			)
		{
			uint64_t Bits;

			if (!GetFieldBits( 1, Bits ))
				return false;

			FieldBit = Bits ? true : false;

			return true;
		}

		inline
		bool
		GetFieldBits(
			 size_t NumBits,
			 unsigned char &FieldBits
			)
		{
			uint64_t Bits;

			if (NumBits > 8)
				return false;

			if (!GetFieldBits( NumBits, Bits ))
				return false;

			FieldBits = static_cast< unsigned char >( Bits );

			return true;
		}

		inline
		bool
		GetFieldBits(
			 size_t NumBits,
			 unsigned short &FieldBits
			)
		{
			uint64_t Bits;

			if (NumBits > 16)
				return false;

			if (!GetFieldBits( NumBits, Bits ))
				return false;

			FieldBits = static_cast< unsigned short >( Bits );

			return true;
		}

		inline
		bool
		GetFieldBits(
			 size_t NumBits,
			 unsigned long &FieldBits
			)
		{
			uint64_t Bits;

			if (NumBits > 32)
				return false;

			if (!GetFieldBits( NumBits, Bits ))
				return false;

			FieldBits = static_cast< unsigned long >( Bits );

			return true;
		}

		virtual
		bool
		GetFieldBits(
			 size_t NumBits,
			 uint64_t &FieldBits
			) = 0;

		virtual
		bool
		AtEndOfStream(
			) const = 0;

		//
		// Debugging routine to get the current byte position.
		//

		virtual
		size_t
		GetBytePos(
			) const = 0;

		//
		// Debugging routine to get the base data pointer for the parser.
		//

		virtual
		const unsigned char *
		GetBaseDataPointer(
			) const = 0;

		//
		// Debugging routine to get the current bit position.
		//

		virtual
		size_t
		GetBitPos(
			) const = 0;

		//
		// Debugging routine to get the highest legal bit number in the last
		// byte of the bit stream.
		//

		virtual
		size_t
		GetHighestValidBitPos(
			) const = 0;

		virtual
		inline
		size_t
		GetBytesRemaining(
			) const = 0;

		virtual
		bool
		SkipData(
			 size_t FieldLength
			) = 0;

		virtual
		bool
		SkipBits(
			 size_t NumBits
			) = 0;

		virtual
		void
		Reset(
			) = 0;

		virtual
		void
		SetHighestValidBitPos(
			 size_t HighestValidBitPos
			) = 0;

		virtual
		void
		RebaseBuffer(
			 const void *Data
			) = 0;

	};

	class BufferParser : public IBufferParser
	{

	public:

		enum BitOrderMode
		{
			BitOrderLowToHigh,
			BitOrderHighToLow
		};

		BufferParser(
			 const void *Data,
			 size_t Length,
			 BitOrderMode BitOrder = BitOrderLowToHigh
			);

		~BufferParser( );

		virtual
		bool
		GetDataPtr(
			 size_t FieldLength,
			 const void **Field
			);

		virtual
		bool
		GetData(
			 size_t FieldLength,
			 void *Field
			);

		inline
		bool
		GetFieldBits(
			 size_t NumBits,
			 unsigned char &FieldBits
			)
		{
			uint64_t Bits;

			if (NumBits > 8)
				return false;

			if (!GetFieldBits( NumBits, Bits ))
				return false;

			FieldBits = static_cast< unsigned char >( Bits );

			return true;
		}

		inline
		bool
		GetFieldBits(
			 size_t NumBits,
			 unsigned short &FieldBits
			)
		{
			uint64_t Bits;

			if (NumBits > 16)
				return false;

			if (!GetFieldBits( NumBits, Bits ))
				return false;

			FieldBits = static_cast< unsigned short >( Bits );

			return true;
		}

		inline
		bool
		GetFieldBits(
			 size_t NumBits,
			 unsigned long &FieldBits
			)
		{
			uint64_t Bits;

			if (NumBits > 32)
				return false;

			if (!GetFieldBits( NumBits, Bits ))
				return false;

			FieldBits = static_cast< unsigned long >( Bits );

			return true;
		}

		virtual
		bool
		GetFieldBits(
			 size_t NumBits,
			 uint64_t &FieldBits
			);

		virtual
		inline
		bool
		AtEndOfStream(
			) const
		{
			//
			// If we are in byte mode, just check if we've got any bytes left.
			//

			if (m_BitPos == 8)
				return (m_DataPosRemaining == 0);

			//
			// DataPosRemaining should never be 0 as we have a byte that is
			// still being worked on by definition if we are in bit mode.
			//

			//
			// Otherwise we are in bit mode.  We may have a byte left but have
			// zero valid bits left within it, so check that.
			//

			size_t BitsThisByte  = ((m_DataPosRemaining > 1) ? 8 : m_HighestValidBitPos);
			size_t BitsRemaining = BitsThisByte - ((m_BitPos == 8) ? 0 : m_BitPos);

			return (BitsRemaining == 0);
		}

		virtual
		inline
		size_t
		GetBytePos(
			) const
		{
			return m_DataPos - m_Data;
		}

		virtual
		inline
		const unsigned char *
		GetBaseDataPointer(
			) const
		{
			return m_Data;
		}

		virtual
		inline
		size_t
		GetBitPos(
			) const
		{
			return m_BitPos;
		}

		virtual
		inline
		size_t
		GetHighestValidBitPos(
			) const
		{
			return m_HighestValidBitPos;
		}

		virtual
		inline
		size_t
		GetBytesRemaining(
			) const
		{
			return (m_DataPosRemaining);
		}

		virtual
		bool
		SkipData(
			 size_t FieldLength
			);

		virtual
		bool
		SkipBits(
			 size_t NumBits
			);

		virtual
		void
		Reset(
			);

		virtual
		void
		SetHighestValidBitPos(
			 size_t HighestValidBitPos
			);

		virtual
		void
		RebaseBuffer(
			 const void *Data
			);

	private:

		//
		// Initial values for parsing reset.
		//

		const unsigned char *m_Data;
		size_t               m_DataLength;

		//
		// Current position and remaining data.
		//

		const unsigned char *m_DataPos;
		size_t               m_DataPosRemaining;
		size_t               m_BitPos;
		size_t               m_HighestValidBitPos;
		BitOrderMode         m_BitOrderMode;

	};

}

#endif


