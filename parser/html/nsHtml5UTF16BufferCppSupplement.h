




































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
