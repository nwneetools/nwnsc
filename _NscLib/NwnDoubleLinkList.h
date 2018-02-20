#ifndef ETS_NWNDOUBLELINKLIST_H
#define ETS_NWNDOUBLELINKLIST_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NwnDoubleLinkList.h - Absolute double link list |
//
// This module contains the definition of an absolute double link list.
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
// $History: NwnDoubleLinkList.h $
//      
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Required include files
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Forward definitions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNwnDoubleLinkList
{

// @access Construction and destruction
public:

	// @cmember General constructor

	CNwnDoubleLinkList ()
	{
		Initialize ();
	}

	// @cmember General destructor

	~CNwnDoubleLinkList ()
	{
		Remove ();
	}

// @access Public methods
public:

	// @cmember Insert a new item after given one

	void InsertAfter (CNwnDoubleLinkList *pLink);

	// @cmember Remove an item from a list

	void Remove ();

// @access Public inline methods
public:

	// @cmember Initialize

	void Initialize ()
	{
		m_pNext = this;
		m_pPrev = this;
	}

	// @cmember Insert a new item at tail of list

	void InsertTail (CNwnDoubleLinkList *pLink) 
	{
		InsertAfter (pLink ->m_pPrev); 
	}

	// @cmember Insert a new item at head of list

	void InsertHead (CNwnDoubleLinkList *pLink) 
	{
		InsertAfter (pLink);
	}

	// @cmember Test to see if empty
					
	bool IsEmpty () const
	{ 
		return m_pNext == this; 
	}
	
	// @cmember Return the next item

	CNwnDoubleLinkList *GetNext ()  
	{ 
		return m_pNext;
	}

	// @cmember Return the next item

	const CNwnDoubleLinkList *GetNext () const
	{ 
		return m_pNext;
	}

	// @cmember Return prev item in a list
		
	CNwnDoubleLinkList *GetPrev ()  
	{ 
		return m_pPrev;
	}

	// @cmember Return prev item in a list
		
	const CNwnDoubleLinkList *GetPrev () const
	{ 
		return m_pPrev;
	}

// @cmember Private methods
private:

	// @cmember Copy operator

	CNwnDoubleLinkList &operator = (CNwnDoubleLinkList &src);

// @access Protected members
protected:

	// @cmember Next link in the list

	CNwnDoubleLinkList			*m_pNext;

	// @cmember Previous link in the list

	CNwnDoubleLinkList			*m_pPrev;
};

//-----------------------------------------------------------------------------
//
// @mfunc Insert an item after current list entry
//
// @parm CNwnDoubleLinkList * | pLink | Link to insert after
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

inline void CNwnDoubleLinkList::InsertAfter (CNwnDoubleLinkList *pLink)
{

	//
	// If the list is the same as the chain being deleted, move backwards
	// to avoid attaching to a detached chain (don't used the Prev function
	// since it will not return the root)
	//

	if (pLink == this) 
		pLink = pLink ->m_pPrev;
	    
	
	// 
	// If the object is currently linked to another chain,
	// remove the link from the chain 
	// 

	if (m_pNext != this) 
		Remove ();
	
	// 
	// Get the current next and prev pointer of the list
	// 

	CNwnDoubleLinkList *pNext = pLink ->m_pNext;
	//CNwnDoubleLinkList *pPrev = pLink ->m_pPrev;
	
	// 
	// Chain the entries together
	// 

	m_pNext = pNext;
	m_pPrev = pLink;
	pLink ->m_pNext = this;
	pNext ->m_pPrev = this;	
	return;	
}

//-----------------------------------------------------------------------------
//
// @mfunc Remove a link from the list
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

inline void CNwnDoubleLinkList::Remove ()
{

	//
	// Return if we currently aren't in a link
	//

	if (m_pNext == this)
		return;

	// 
	// Get the current next and prev pointer of the list
	// 

	CNwnDoubleLinkList *pNext = m_pNext;
	CNwnDoubleLinkList *pPrev = m_pPrev;
	
	// 
	// Unchain the list
	// 

	pNext ->m_pPrev = pPrev;
	pPrev ->m_pNext = pNext;
	
	// 
	// Initialize the unlinked item
	// 

	m_pNext = this;
	m_pPrev = this;
	return;		
}

#endif // ETS_NWNDOUBLELINKLIST_H
