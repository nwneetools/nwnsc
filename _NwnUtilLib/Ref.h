#ifndef _SOURCE_PROGRAMS_SKYWINGUTILS_REF_REF_H
#define _SOURCE_PROGRAMS_SKYWINGUTILS_REF_REF_H

#include "Precomp.h"
#include "OsCompat.h"

#ifdef _MSC_VER
#pragma once
#endif

#ifdef _M_CEE
#pragma warning(push)
#pragma warning(disable:4793) // warning C4793: 'swutil::InterlockedIncrementPtr' : function compiled as native : Found an intrinsic not supported in managed code
#endif

namespace swutil
{

	inline
	LONG_PTR
	InterlockedIncrementPtr(
		 LONG_PTR volatile * Addend
		)
	{
#ifdef _WINDOWS
		return InterlockedIncrement( Addend );
#else
		__sync_add_and_fetch(Addend,1);
#endif
	}

	inline
	LONG_PTR
	InterlockedDecrementPtr(
		 LONG_PTR volatile * Addend
		)
	{
#ifdef _WINDOWS
		return InterlockedIncrement( Addend );
#else
		__sync_add_and_fetch(Addend,1);
#endif
	}

	//
	// The IRef interface describes a reference counting contract.  An object
	// implementing the IRef interface conforms to the following requirements:
	//
	// - The object initially has a non-zero reference count.
	// - Once the reference count goes to zero, it will never increase.
	// - Reference creation and deletion may be processed from any thread
	//   context.  If deletion work is non-trivial, then it will be processed
	//   in an asynchronous fashion in a safe context.
	// - Referencing an object is not a must succeed operation as it is
	//   possible that a reference count overflow might occur.  Implementers
	//   should typically provide pointer-sized reference count value ranges.
	//
	class IRef
	{

	public:

		//
		// Adds a reference, returning false if the operation is not
		// successful (due to a reference count overflow).
		//
		virtual bool Reference() = 0;

		//
		// Removes a reference, returning true if the reference count has gone
		// to zero.
		//
		virtual bool Dereference() = 0;

	private:

	};

	//
	// Implements a reference count with deletion via ``delete this''.
	// There may be only one reference count implementation in an object tree
	// for a particular object.  The default deletion policy may be overridden
	// via providing an OnObjectDeletion event handler implementation.
	//
	class Ref : virtual public IRef
	{

	public:

		Ref()
		: m_References( 1 )
		{
		}
		virtual ~Ref()
		{
		}

		virtual bool Reference()
		{
			if (!InterlockedAddReference( &m_References ))
			{
				return false;
			}

			return true;
		}

		virtual bool Dereference()
		{
			if (InterlockedDecrementReference( &m_References ))
			{
				OnObjectDeletion();
				return false;
			}

			return true;
		}

	protected:

		//
		// Deletes the object.  The default implementation simply uses the
		// operator delete allocator.
		//
		virtual void OnObjectDeletion()
		{
			delete this;
		}

	private:

		static
		inline
		bool
		InterlockedAddReference(
			 ULONG_PTR volatile * Reference
			)
		{
			ULONG_PTR PrevValue, PrevCopy;

			PrevValue = *Reference;

			do
			{
				//
				// If we would overflow the reference count then we'll fail the
				// request.  Also, if the reference count is zero, then we'll
				// not take a reference, in case cleanup is deferred and we
				// have a window where we have a static list that may own a
				// copy cleared in final cleanup, but final cleanup has not yet
				// been called.
				//

				if (((PrevValue + 1) < PrevValue) || (!PrevValue))
					return false;

				//
				// Otherwise, attempt to exchange the current reference count
				// with an incremented reference count.
				//

				PrevCopy  = PrevValue;

#if defined(_WINDOWS)
				PrevValue = (ULONG_PTR)InterlockedCompareExchangePointer(
					(PVOID volatile * )(Reference),
					(PVOID)(PrevCopy + 1),
					(PVOID)(PrevValue)
					);
#else
				__sync_val_compare_and_swap((PVOID volatile * )(Reference),(PVOID)(PrevValue),(PVOID)(PrevCopy + 1));
#endif
			}
			while (PrevCopy != PrevValue) ;

			return true;
		}

		static
		inline
		bool
		InterlockedDecrementReference(
			 ULONG_PTR volatile * Reference
			)
		{
#ifdef _WINDOWS
			return (InterlockedDecrement(
				(LONG volatile *)Reference )) == 0;
#else
			return (__sync_sub_and_fetch((LONG volatile *)Reference ,1) ) == 0;
#endif
		}

		ULONG_PTR m_References;
	};

	//
	// Scoped reference/dereference object.
	//
	class ScopedRef
	{

	public:

		ScopedRef(  IRef * Ref )
			: m_Ref( Ref )
		{
			if (!m_Ref->Reference())
				throw std::runtime_error( "Failed to acquire reference." );
		}

		~ScopedRef()
		{
			m_Ref->Dereference();
		}

	private:

		IRef * m_Ref;

	};

	//
	// Scoped dereference only object, e.g. to be used in asynchronous
	// completion handlers where the reference is logically transferred to
	// the completion handler.
	//
	class ScopedDeref
	{

	public:

		ScopedDeref(  IRef * Ref )
			: m_Ref( Ref )
		{
		}

		~ScopedDeref()
		{
			m_Ref->Dereference();
		}

	private:

		IRef * m_Ref;

	};

	//
	// Reference counted buffer class, similar to std::tr1::shared_ptr<>, but
	// without the dependency upon boost.
	//


	//
	// Deleter callback type for the shared buffer.
	//
	// template< class T >
	// typedef void (* BufferDeleter)(  T * p );

	//
	// Default deleter implementation for operator delete [].
	//
	template< class T >
	inline
	void
	BufferDeleterDelete(
		 T *p
		)
	{
		delete [] p;

		//
		// Ensure that we never are permitted to delete a partial type.
		//

		enum __force_completed_type_T { __force_completed_type = sizeof( T ) };
	}

	//
	// Define allocator routines for SharedState components of SharedPtr and
	// SharedBuffer.  These allow SharedPtr to be used cross-module (although
	// the user must guarantee to appropriately overload operator new and
	// operator delete on the type in question to a module-insensitive
	// allocator).
	//

	inline
	LONG_PTR *
	SharedPtrAllocSharedState(
		 LONG_PTR Initializer
		)
	{
		LONG_PTR * SharedState;

#if defined(_WINDOWS)
		SharedState = (LONG_PTR *) HeapAlloc(
			GetProcessHeap( ),
			0,
			sizeof( LONG_PTR ));
#else
		SharedState = (LONG_PTR *) malloc(sizeof( LONG_PTR ));
#endif

		if (SharedState == NULL)
			throw std::bad_alloc( );

		*SharedState = Initializer;

		return SharedState;
	}

	inline
	void
	SharedPtrDeleteSharedState(
		 LONG_PTR * SharedState
		)
	{
#if defined(_WINDOWS)
		HeapFree( GetProcessHeap( ), 0, SharedState );
#else
        free(SharedState);
#endif
	}

	//
	// Shared memory buffer.  Defaults to taking a buffer that must be
	// allocated via operator new[], though a different allocator may be used
	// if the deleter is overridden.
	//
	// SharedBuffer objects reference count the internal buffer allocation.
	//
	template<
		class T /*,
		void (* Deleter)(  T * p ) = BufferDeleterDelete< T >  */
		>
	class SharedBuffer
	{

	public:

		SharedBuffer(
			 T *Buf,
			 size_t Size
			)
			: m_Buf( Buf ),
			  m_Size( Size ),
			  m_SharedState( NULL )
		{
			if (m_Buf)
			{
				try
				{
					m_SharedState = SharedPtrAllocSharedState( 1 );
				}
				catch (std::bad_alloc)
				{
					BufferDeleterDelete( m_Buf );
					throw;
				}
			}
		}

		SharedBuffer()
			: m_Buf( NULL ),
			  m_Size( 0 ),
			  m_SharedState( NULL )
		{
		}

		SharedBuffer(  const SharedBuffer & Other )
			: m_Buf( Other.m_Buf ),
			  m_Size( Other.m_Size ),
			  m_SharedState( Other.m_SharedState )
		{
			if (m_SharedState)
			{
				InterlockedIncrementPtr( m_SharedState );
			}
		}

		~SharedBuffer()
		{
			if (m_SharedState)
			{
				if (!InterlockedDecrementPtr( m_SharedState ))
				{
					BufferDeleterDelete( m_Buf );
					delete m_SharedState;
				}
			}
		}

		SharedBuffer& operator=(
			 const SharedBuffer & Other
			)
		{
			if (m_SharedState == Other.m_SharedState)
				return *this;

			if (m_SharedState)
			{
				if (!InterlockedDecrementPtr( m_SharedState ))
				{
					BufferDeleterDelete( m_Buf );
					SharedPtrDeleteSharedState( m_SharedState );
				}
			}

			m_SharedState = Other.m_SharedState;
			m_Buf         = Other.m_Buf;
			m_Size        = Other.m_Size;

			if (m_SharedState)
				InterlockedIncrementPtr( m_SharedState );

			return *this;
		}

		inline T * get() { return m_Buf; }
		inline size_t size() { return m_Size; }

		inline
		void
		release()
		{
			if (m_SharedState)
			{
				if (!InterlockedDecrementPtr( m_SharedState ))
				{
					BufferDeleterDelete( m_Buf );
					SharedPtrDeleteSharedState( m_SharedState );
				}

				m_SharedState = NULL;
				m_Buf         = NULL;
				m_Size        = 0;
			}
		}

		inline
		bool
		unique() const
		{
			if (!m_SharedState)
				return true;

			return *m_SharedState == 1;
		}

//	private: // For internal use only, do not use.

		typedef LONG_PTR SharedState;

		mutable SharedState * m_SharedState;
		T                   * m_Buf;
		size_t                m_Size;

	};



	//
	// Reference counted ponter class, similar to std::tr1::shared_ptr<>, but
	// without the dependency upon boost.
	//

	//
	// Deleter callback type for the shared pointer.
	//
	// template< class T >
	// typedef void (* SharedPtrDeleter)(  T * p );

	//
	// Default deleter implementation for operator delete.
	//
	template< class T >
	inline
	void
	SharedPtrDeleterDelete(
		 T *p
		)
	{
		delete p;

		//
		// Ensure that we never are permitted to delete a partial type.
		//

		enum __force_completed_type_T { __force_completed_type = sizeof( T ) };
	}

	//
	// Shared pointer.  Defaults to taking a pointer that must be allocated
	// via operator new, though a different allocator may be used if the
	// deleter is overridden.
	//
	// SharedPtr objects reference count the internal buffer allocation.
	//
	template<
		class T /*,
		void (* Deleter)(  T * p ) = SharedPtrDeleterDelete< T >  */
		>
	class SharedPtr
	{

	public:

		SharedPtr(
			 T *Ptr
			)
			: m_Ptr( Ptr ),
			  m_SharedState( NULL )
		{
			if (m_Ptr)
			{
				try
				{
					m_SharedState = SharedPtrAllocSharedState( 1 );
				}
				catch (std::bad_alloc)
				{
					//
					// If we failed to allocate the shared state, delete the pointer
					// that had been transferred to us.  This is consistent with the
					// documentation for std::shared_ptr.  However, it might be
					// better to only allocate on first copy instead of right away,
					// which avoids the problem altogether.
					//

					SharedPtrDeleterDelete( Ptr );
					throw;
				}
			}
		}

		SharedPtr()
			: m_Ptr( NULL ),
			  m_SharedState( NULL )
		{
		}

		SharedPtr(  const SharedPtr & Other )
			: m_Ptr( Other.m_Ptr ),
			  m_SharedState( Other.m_SharedState )
		{
			if (m_SharedState)
			{
				InterlockedIncrementPtr( m_SharedState );
			}
		}

		~SharedPtr()
		{
			if (m_SharedState)
			{
				if (!InterlockedDecrementPtr( m_SharedState ))
				{
					SharedPtrDeleterDelete( m_Ptr );
					SharedPtrDeleteSharedState( m_SharedState );
				}
			}
		}

		SharedPtr& operator=(
			 const SharedPtr & Other
			)
		{
			if (m_SharedState == Other.m_SharedState)
				return *this;

			if (m_SharedState)
			{
				if (!InterlockedDecrementPtr( m_SharedState ))
				{
					SharedPtrDeleterDelete( m_Ptr );
					SharedPtrDeleteSharedState( m_SharedState );
				}
			}

			m_SharedState = Other.m_SharedState;
			m_Ptr         = Other.m_Ptr;

			if (m_SharedState)
				InterlockedIncrementPtr( m_SharedState );

			return *this;
		}

		inline T * get() const { return m_Ptr; }
		inline T & operator*() const { return *m_Ptr; }
		inline T * operator->() const { return m_Ptr; }
		inline bool operator==( const T * t) const { return m_Ptr == t; }
		inline bool operator!=( const T * t) const { return m_Ptr != t; }

		inline
		void
		release()
		{
			if (m_SharedState)
			{
				if (!InterlockedDecrementPtr( m_SharedState ))
				{
					SharedPtrDeleterDelete( m_Ptr );
					SharedPtrDeleteSharedState( m_SharedState );
				}

				m_SharedState = NULL;
				m_Ptr         = NULL;
			}
		}

		inline
		bool
		unique() const
		{
			if (!m_SharedState)
				return true;

			return *m_SharedState == 1;
		}

		template< typename T2 >
		inline
		swutil::SharedPtr< T2 >
		cast_to()
		{
			swutil::SharedPtr< T2 > sp;

			sp.m_SharedState = m_SharedState;
			sp.m_Ptr         = (T2 *) m_Ptr;

			if (sp.m_SharedState)
				InterlockedIncrementPtr( sp.m_SharedState );

			return sp;
		}

//	private: // Do not access these members publicly.

		typedef LONG_PTR SharedState;

		mutable SharedState * m_SharedState;
		T                   * m_Ptr;
	};


	
	//
	// Define automatically managed buffer context.
	//

	typedef std::vector< unsigned char > ByteVec;

	typedef swutil::SharedPtr< std::vector< unsigned char > > SharedByteVec;

	//
	// A class definition may use DECLARE_SWUTIL_CROSS_MODULE_NEW in order to
	// create operator new / operator delete overloads which function cross
	// module (by virtue of using the process heap or other platform equivalent
	// mechanism).
	//
	// N.B.  Interior data members of such a class must be similarly declared
	//       to avoid fatal data corruption.
	//

#define DECLARE_SWUTIL_CROSS_MODULE_NEW( )                             \
	                                                                   \
	inline                                                             \
	void *                                                             \
	operator new(                                                      \
	     size_t s                                                  \
	    )                                                              \
	{                                                                  \
	    void * p;                                                      \
	                                                                   \
	    p = (void *) HeapAlloc( GetProcessHeap( ), 0, s );             \
	                                                                   \
	    if (p == NULL)                                                 \
	        throw std::bad_alloc( );                                   \
	                                                                   \
	    return p;                                                      \
	}                                                                  \
	                                                                   \
	inline                                                             \
	void                                                               \
	operator delete(                                                   \
		 void * p                                                  \
		)                                                              \
	{                                                                  \
		if (p == NULL)                                                 \
			return;                                                    \
	                                                                   \
		HeapFree( GetProcessHeap( ), 0, p );                           \
	}                                                                  \
	                                                                   \
	inline                                                             \
	void *                                                             \
	operator new[ ](                                                   \
	     size_t s                                                  \
	    )                                                              \
	{                                                                  \
	    void * p;                                                      \
	                                                                   \
	    p = (void *) HeapAlloc( GetProcessHeap( ), 0, s );             \
	                                                                   \
	    if (p == NULL)                                                 \
	        throw std::bad_alloc( );                                   \
	                                                                   \
	    return p;                                                      \
	}                                                                  \
	                                                                   \
	inline                                                             \
	void                                                               \
	operator delete[ ](                                                \
		 void * p                                                  \
		)                                                              \
	{                                                                  \
		if (p == NULL)                                                 \
			return;                                                    \
	                                                                   \
		HeapFree( GetProcessHeap( ), 0, p );                           \
	}                                                                  \

}

#ifdef _M_CEE
#pragma warning(pop)
#endif

#endif


