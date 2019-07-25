




































#include "nsHtml5OwningUTF16Buffer.h"

nsHtml5OwningUTF16Buffer::nsHtml5OwningUTF16Buffer(PRUnichar* aBuffer)
  : nsHtml5UTF16Buffer(aBuffer, 0),
    next(nsnull),
    key(nsnull)
{
  MOZ_COUNT_CTOR(nsHtml5OwningUTF16Buffer);
}

nsHtml5OwningUTF16Buffer::nsHtml5OwningUTF16Buffer(void* aKey)
  : nsHtml5UTF16Buffer(nsnull, 0),
    next(nsnull),
    key(aKey)
{
  MOZ_COUNT_CTOR(nsHtml5OwningUTF16Buffer);
}

nsHtml5OwningUTF16Buffer::~nsHtml5OwningUTF16Buffer()
{
  MOZ_COUNT_DTOR(nsHtml5OwningUTF16Buffer);
  DeleteBuffer();
}


already_AddRefed<nsHtml5OwningUTF16Buffer>
nsHtml5OwningUTF16Buffer::FalliblyCreate(PRInt32 aLength)
{
  const mozilla::fallible_t fallible = mozilla::fallible_t();
  PRUnichar* newBuf = new (fallible) PRUnichar[aLength];
  if (!newBuf) {
    return nsnull;
  }
  nsRefPtr<nsHtml5OwningUTF16Buffer> newObj =
    new (fallible) nsHtml5OwningUTF16Buffer(newBuf);
  if (!newObj) {
    delete[] newBuf;
    return nsnull;
  }
  return newObj.forget();
}

void
nsHtml5OwningUTF16Buffer::Swap(nsHtml5OwningUTF16Buffer* aOther)
{
  nsHtml5UTF16Buffer::Swap(aOther);
}





nsrefcnt
nsHtml5OwningUTF16Buffer::AddRef()
{
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "Illegal refcount.");
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "nsHtml5OwningUTF16Buffer", sizeof(*this));
  return mRefCnt;
}

nsrefcnt
nsHtml5OwningUTF16Buffer::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "Release without AddRef.");
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsHtml5OwningUTF16Buffer");
  if (mRefCnt == 0) {
    mRefCnt = 1; 
    delete this;
    return 0;
  }
  return mRefCnt;
}
