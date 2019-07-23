




































#include "nsTraceRefcnt.h"

nsHtml5UTF16Buffer::nsHtml5UTF16Buffer(PRInt32 size)
  : buffer(new PRUnichar[size]),
    start(0),
    end(0),
    next(nsnull),
    key(nsnull)
{
  MOZ_COUNT_CTOR(nsHtml5UTF16Buffer);
}

nsHtml5UTF16Buffer::nsHtml5UTF16Buffer(void* key)
  : buffer(nsnull),
    start(0),
    end(0),
    next(nsnull),
    key(key)
{
  MOZ_COUNT_CTOR(nsHtml5UTF16Buffer);
}

nsHtml5UTF16Buffer::~nsHtml5UTF16Buffer()
{
  MOZ_COUNT_DTOR(nsHtml5UTF16Buffer);
  delete[] buffer;
}




nsrefcnt
nsHtml5UTF16Buffer::AddRef()
{
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "Illegal refcount.");
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "nsHtml5UTF16Buffer", sizeof(*this));
  return mRefCnt;
}

nsrefcnt
nsHtml5UTF16Buffer::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "Release without AddRef.");
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsHtml5UTF16Buffer");
  if (mRefCnt == 0) {
    mRefCnt = 1; 
    delete this;
    return 0;
  }
  return mRefCnt;                              
}
