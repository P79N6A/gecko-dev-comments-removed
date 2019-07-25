





































#include <stdlib.h>
#include "nsScannerString.h"


  



#define MAX_CAPACITY ((PR_UINT32_MAX / sizeof(PRUnichar)) - \
                      (sizeof(Buffer) + sizeof(PRUnichar)))

nsScannerBufferList::Buffer*
nsScannerBufferList::AllocBufferFromString( const nsAString& aString )
  {
    PRUint32 len = aString.Length();

    if (len > MAX_CAPACITY)
      return nsnull;

    Buffer* buf = (Buffer*) malloc(sizeof(Buffer) + (len + 1) * sizeof(PRUnichar));
    if (buf)
      {
        

        buf->mUsageCount = 0;
        buf->mDataEnd = buf->DataStart() + len;

        nsAString::const_iterator source;
        aString.BeginReading(source);
        nsCharTraits<PRUnichar>::copy(buf->DataStart(), source.get(), len);

        
        
        *buf->mDataEnd = PRUnichar(0);
      }
    return buf;
  }

nsScannerBufferList::Buffer*
nsScannerBufferList::AllocBuffer( PRUint32 capacity )
  {
    if (capacity > MAX_CAPACITY)
      return nsnull;

    Buffer* buf = (Buffer*) malloc(sizeof(Buffer) + (capacity + 1) * sizeof(PRUnichar));
    if (buf)
      {
        

        buf->mUsageCount = 0;
        buf->mDataEnd = buf->DataStart() + capacity;

        
        
        *buf->mDataEnd = PRUnichar(0);
      }
    return buf;
  }

void
nsScannerBufferList::ReleaseAll()
  {
    while (!PR_CLIST_IS_EMPTY(&mBuffers))
      {
        PRCList* node = PR_LIST_HEAD(&mBuffers);
        PR_REMOVE_LINK(node);
        
        free(static_cast<Buffer*>(node));
      }
  }

void
nsScannerBufferList::SplitBuffer( const Position& pos )
  {
    
    

    Buffer* bufferToSplit = pos.mBuffer;
    NS_ASSERTION(bufferToSplit, "null pointer");

    PRUint32 splitOffset = pos.mPosition - bufferToSplit->DataStart();
    NS_ASSERTION(pos.mPosition >= bufferToSplit->DataStart() &&
                 splitOffset <= bufferToSplit->DataLength(),
                 "split offset is outside buffer");
    
    PRUint32 len = bufferToSplit->DataLength() - splitOffset;
    Buffer* new_buffer = AllocBuffer(len);
    if (new_buffer)
      {
        nsCharTraits<PRUnichar>::copy(new_buffer->DataStart(),
                                      bufferToSplit->DataStart() + splitOffset,
                                      len);
        InsertAfter(new_buffer, bufferToSplit);
        bufferToSplit->SetDataLength(splitOffset);
      }
  }

void
nsScannerBufferList::DiscardUnreferencedPrefix( Buffer* aBuf )
  {
    if (aBuf == Head())
      {
        while (!PR_CLIST_IS_EMPTY(&mBuffers) && !Head()->IsInUse())
          {
            Buffer* buffer = Head();
            PR_REMOVE_LINK(buffer);
            free(buffer);
          }
      }
  }

size_t
nsScannerBufferList::Position::Distance( const Position& aStart, const Position& aEnd )
  {
    size_t result = 0;
    if (aStart.mBuffer == aEnd.mBuffer)
      {
        result = aEnd.mPosition - aStart.mPosition;
      }
    else
      {
        result = aStart.mBuffer->DataEnd() - aStart.mPosition;
        for (Buffer* b = aStart.mBuffer->Next(); b != aEnd.mBuffer; b = b->Next())
          result += b->DataLength();
        result += aEnd.mPosition - aEnd.mBuffer->DataStart();
      }
    return result;
  }






nsScannerSubstring::nsScannerSubstring()
  : mStart(nsnull, nsnull)
  , mEnd(nsnull, nsnull)
  , mBufferList(nsnull)
  , mLength(0)
  , mIsDirty(true)
  {
  }

nsScannerSubstring::nsScannerSubstring( const nsAString& s )
  : mBufferList(nsnull)
  , mIsDirty(true)
  {
    Rebind(s);
  }

nsScannerSubstring::~nsScannerSubstring()
  {
    release_ownership_of_buffer_list();
  }

PRInt32
nsScannerSubstring::CountChar( PRUnichar c ) const
  {
      



    size_type result = 0;
    size_type lengthToExamine = Length();

    nsScannerIterator iter;
    for ( BeginReading(iter); ; )
      {
        PRInt32 lengthToExamineInThisFragment = iter.size_forward();
        const PRUnichar* fromBegin = iter.get();
        result += size_type(NS_COUNT(fromBegin, fromBegin+lengthToExamineInThisFragment, c));
        if ( !(lengthToExamine -= lengthToExamineInThisFragment) )
          return result;
        iter.advance(lengthToExamineInThisFragment);
      }
      
    return 0;
  }

void
nsScannerSubstring::Rebind( const nsScannerSubstring& aString,
                            const nsScannerIterator& aStart, 
                            const nsScannerIterator& aEnd )
  {
    

    aString.acquire_ownership_of_buffer_list();
    release_ownership_of_buffer_list();

    mStart      = aStart;
    mEnd        = aEnd;
    mBufferList = aString.mBufferList;
    mLength     = Distance(aStart, aEnd);
    mIsDirty    = true;
  }

void
nsScannerSubstring::Rebind( const nsAString& aString )
  {
    release_ownership_of_buffer_list();

    mBufferList = new nsScannerBufferList(AllocBufferFromString(aString));
    mIsDirty    = true;

    init_range_from_buffer_list();
    acquire_ownership_of_buffer_list();
  }

const nsSubstring&
nsScannerSubstring::AsString() const
  {
    if (mIsDirty)
      {
        nsScannerSubstring* mutable_this = const_cast<nsScannerSubstring*>(this);

        if (mStart.mBuffer == mEnd.mBuffer) {
          
          
          mutable_this->mFlattenedRep.Rebind(mStart.mPosition, mEnd.mPosition);
        } else {
          
          nsScannerIterator start, end;
          CopyUnicodeTo(BeginReading(start), EndReading(end), mutable_this->mFlattenedRep);
        }

        mutable_this->mIsDirty = false;
      }

    return mFlattenedRep;
  }

nsScannerIterator&
nsScannerSubstring::BeginReading( nsScannerIterator& iter ) const
  {
    iter.mOwner = this;

    iter.mFragment.mBuffer = mStart.mBuffer;
    iter.mFragment.mFragmentStart = mStart.mPosition;
    if (mStart.mBuffer == mEnd.mBuffer)
      iter.mFragment.mFragmentEnd = mEnd.mPosition;
    else
      iter.mFragment.mFragmentEnd = mStart.mBuffer->DataEnd();

    iter.mPosition = mStart.mPosition;
    iter.normalize_forward();
    return iter;
  }

nsScannerIterator&
nsScannerSubstring::EndReading( nsScannerIterator& iter ) const
  {
    iter.mOwner = this;

    iter.mFragment.mBuffer = mEnd.mBuffer;
    iter.mFragment.mFragmentEnd = mEnd.mPosition;
    if (mStart.mBuffer == mEnd.mBuffer)
      iter.mFragment.mFragmentStart = mStart.mPosition;
    else
      iter.mFragment.mFragmentStart = mEnd.mBuffer->DataStart();

    iter.mPosition = mEnd.mPosition;
    
    return iter;
  }

bool
nsScannerSubstring::GetNextFragment( nsScannerFragment& frag ) const
  {
    
    if (frag.mBuffer == mEnd.mBuffer)
      return false;

    frag.mBuffer = static_cast<const Buffer*>(PR_NEXT_LINK(frag.mBuffer));

    if (frag.mBuffer == mStart.mBuffer)
      frag.mFragmentStart = mStart.mPosition;
    else
      frag.mFragmentStart = frag.mBuffer->DataStart();

    if (frag.mBuffer == mEnd.mBuffer)
      frag.mFragmentEnd = mEnd.mPosition;
    else
      frag.mFragmentEnd = frag.mBuffer->DataEnd();

    return true;
  }

bool
nsScannerSubstring::GetPrevFragment( nsScannerFragment& frag ) const
  {
    
    if (frag.mBuffer == mStart.mBuffer)
      return false;

    frag.mBuffer = static_cast<const Buffer*>(PR_PREV_LINK(frag.mBuffer));

    if (frag.mBuffer == mStart.mBuffer)
      frag.mFragmentStart = mStart.mPosition;
    else
      frag.mFragmentStart = frag.mBuffer->DataStart();

    if (frag.mBuffer == mEnd.mBuffer)
      frag.mFragmentEnd = mEnd.mPosition;
    else
      frag.mFragmentEnd = frag.mBuffer->DataEnd();

    return true;
  }


  



nsScannerString::nsScannerString( Buffer* aBuf )
  {
    mBufferList = new nsScannerBufferList(aBuf);

    init_range_from_buffer_list();
    acquire_ownership_of_buffer_list();
  }

void
nsScannerString::AppendBuffer( Buffer* aBuf )
  {
    mBufferList->Append(aBuf);
    mLength += aBuf->DataLength();

    mEnd.mBuffer = aBuf;
    mEnd.mPosition = aBuf->DataEnd();

    mIsDirty = true;
  }

void
nsScannerString::DiscardPrefix( const nsScannerIterator& aIter )
  {
    Position old_start(mStart);
    mStart = aIter;
    mLength -= Position::Distance(old_start, mStart);
    
    mStart.mBuffer->IncrementUsageCount();
    old_start.mBuffer->DecrementUsageCount();

    mBufferList->DiscardUnreferencedPrefix(old_start.mBuffer);

    mIsDirty = true;
  }

void
nsScannerString::UngetReadable( const nsAString& aReadable, const nsScannerIterator& aInsertPoint )
    







  {
    Position insertPos(aInsertPoint);

    mBufferList->SplitBuffer(insertPos);
      
      

    Buffer* new_buffer = AllocBufferFromString(aReadable);
      
      
      

    Buffer* buffer_to_split = insertPos.mBuffer;
    mBufferList->InsertAfter(new_buffer, buffer_to_split);
    mLength += aReadable.Length();

    mEnd.mBuffer = mBufferList->Tail();
    mEnd.mPosition = mEnd.mBuffer->DataEnd();

    mIsDirty = true;
  }

void
nsScannerString::ReplaceCharacter(nsScannerIterator& aPosition, PRUnichar aChar)
  {
    
    
    
    PRUnichar* pos = const_cast<PRUnichar*>(aPosition.get());
    *pos = aChar;

    mIsDirty = true;
  }


  



void
nsScannerSharedSubstring::Rebind(const nsScannerIterator &aStart,
                              const nsScannerIterator &aEnd)
{
  
  
  

  Buffer *buffer = const_cast<Buffer*>(aStart.buffer());
  bool sameBuffer = buffer == aEnd.buffer();

  nsScannerBufferList *bufferList;

  if (sameBuffer) {
    bufferList = aStart.mOwner->mBufferList;
    bufferList->AddRef();
    buffer->IncrementUsageCount();
  }

  if (mBufferList)
    ReleaseBuffer();

  if (sameBuffer) {
    mBuffer = buffer;
    mBufferList = bufferList;
    mString.Rebind(aStart.mPosition, aEnd.mPosition);
  } else {
    mBuffer = nsnull;
    mBufferList = nsnull;
    CopyUnicodeTo(aStart, aEnd, mString);
  }
}

void
nsScannerSharedSubstring::ReleaseBuffer()
{
  NS_ASSERTION(mBufferList, "Should only be called with non-null mBufferList");
  mBuffer->DecrementUsageCount();
  mBufferList->DiscardUnreferencedPrefix(mBuffer);
  mBufferList->Release();
}

void
nsScannerSharedSubstring::MakeMutable()
{
  nsString temp(mString); 
  mString.Assign(temp);   

  ReleaseBuffer();

  mBuffer = nsnull;
  mBufferList = nsnull;
}

  




static inline
nsAString::iterator&
copy_multifragment_string( nsScannerIterator& first, const nsScannerIterator& last, nsAString::iterator& result )
  {
    typedef nsCharSourceTraits<nsScannerIterator> source_traits;
    typedef nsCharSinkTraits<nsAString::iterator> sink_traits;

    while ( first != last )
      {
        PRUint32 distance = source_traits::readable_distance(first, last);
        sink_traits::write(result, source_traits::read(first), distance);
        NS_ASSERTION(distance > 0, "|copy_multifragment_string| will never terminate");
        source_traits::advance(first, distance);
      }

    return result;
  }

void
CopyUnicodeTo( const nsScannerIterator& aSrcStart,
               const nsScannerIterator& aSrcEnd,
               nsAString& aDest )
  {
    nsAString::iterator writer;
    if (!EnsureStringLength(aDest, Distance(aSrcStart, aSrcEnd))) {
      aDest.Truncate();
      return; 
    }
    aDest.BeginWriting(writer);
    nsScannerIterator fromBegin(aSrcStart);
    
    copy_multifragment_string(fromBegin, aSrcEnd, writer);
  }

void
AppendUnicodeTo( const nsScannerIterator& aSrcStart,
                 const nsScannerIterator& aSrcEnd,
                 nsScannerSharedSubstring& aDest )
  {
    
    if (aDest.str().IsEmpty()) {
      
      
      aDest.Rebind(aSrcStart, aSrcEnd);
    } else {
      
      AppendUnicodeTo(aSrcStart, aSrcEnd, aDest.writable());
    }
  }

void
AppendUnicodeTo( const nsScannerIterator& aSrcStart,
                 const nsScannerIterator& aSrcEnd,
                 nsAString& aDest )
  {
    nsAString::iterator writer;
    PRUint32 oldLength = aDest.Length();
    if (!EnsureStringLength(aDest, oldLength + Distance(aSrcStart, aSrcEnd)))
      return; 
    aDest.BeginWriting(writer).advance(oldLength);
    nsScannerIterator fromBegin(aSrcStart);
    
    copy_multifragment_string(fromBegin, aSrcEnd, writer);
  }

bool
FindCharInReadable( PRUnichar aChar,
                    nsScannerIterator& aSearchStart,
                    const nsScannerIterator& aSearchEnd )
  {
    while ( aSearchStart != aSearchEnd )
      {
        PRInt32 fragmentLength;
        if ( SameFragment(aSearchStart, aSearchEnd) ) 
          fragmentLength = aSearchEnd.get() - aSearchStart.get();
        else
          fragmentLength = aSearchStart.size_forward();

        const PRUnichar* charFoundAt = nsCharTraits<PRUnichar>::find(aSearchStart.get(), fragmentLength, aChar);
        if ( charFoundAt ) {
          aSearchStart.advance( charFoundAt - aSearchStart.get() );
          return true;
        }

        aSearchStart.advance(fragmentLength);
      }

    return false;
  }

bool
FindInReadable( const nsAString& aPattern,
                nsScannerIterator& aSearchStart,
                nsScannerIterator& aSearchEnd,
                const nsStringComparator& compare )
  {
    bool found_it = false;

      
    if ( aSearchStart != aSearchEnd )
      {
        nsAString::const_iterator aPatternStart, aPatternEnd;
        aPattern.BeginReading(aPatternStart);
        aPattern.EndReading(aPatternEnd);

          
        while ( !found_it )
          {
              
            while ( aSearchStart != aSearchEnd &&
                    compare(aPatternStart.get(), aSearchStart.get(), 1, 1) )
              ++aSearchStart;

              
            if ( aSearchStart == aSearchEnd )
              break;

              
            nsAString::const_iterator testPattern(aPatternStart);
            nsScannerIterator testSearch(aSearchStart);

              
            for(;;)
              {
                  
                  
                ++testPattern;
                ++testSearch;

                  
                if ( testPattern == aPatternEnd )
                  {
                    found_it = true;
                    aSearchEnd = testSearch; 
                    break;
                  }

                  
                  
                if ( testSearch == aSearchEnd )
                  {
                    aSearchStart = aSearchEnd;
                    break;
                  }

                  
                  
                if ( compare(testPattern.get(), testSearch.get(), 1, 1) )
                  {
                    ++aSearchStart;
                    break;
                  }
              }
          }
      }

    return found_it;
  }

  




bool
RFindInReadable( const nsAString& aPattern,
                 nsScannerIterator& aSearchStart,
                 nsScannerIterator& aSearchEnd,
                 const nsStringComparator& aComparator )
  {
    bool found_it = false;

    nsScannerIterator savedSearchEnd(aSearchEnd);
    nsScannerIterator searchStart(aSearchStart), searchEnd(aSearchEnd);

    while ( searchStart != searchEnd )
      {
        if ( FindInReadable(aPattern, searchStart, searchEnd, aComparator) )
          {
            found_it = true;

              
            aSearchStart = searchStart;
            aSearchEnd = searchEnd;

              
              
            ++searchStart;
            searchEnd = savedSearchEnd;
          }
      }

      
    if ( !found_it )
      aSearchStart = aSearchEnd;

    return found_it;
  }
