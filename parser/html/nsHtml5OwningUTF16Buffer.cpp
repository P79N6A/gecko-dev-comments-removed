



#include "nsHtml5OwningUTF16Buffer.h"

nsHtml5OwningUTF16Buffer::nsHtml5OwningUTF16Buffer(PRUnichar* aBuffer)
  : nsHtml5UTF16Buffer(aBuffer, 0),
    next(nullptr),
    key(nullptr)
{
  MOZ_COUNT_CTOR(nsHtml5OwningUTF16Buffer);
}

nsHtml5OwningUTF16Buffer::nsHtml5OwningUTF16Buffer(void* aKey)
  : nsHtml5UTF16Buffer(nullptr, 0),
    next(nullptr),
    key(aKey)
{
  MOZ_COUNT_CTOR(nsHtml5OwningUTF16Buffer);
}

nsHtml5OwningUTF16Buffer::~nsHtml5OwningUTF16Buffer()
{
  MOZ_COUNT_DTOR(nsHtml5OwningUTF16Buffer);
  DeleteBuffer();

  
  nsRefPtr<nsHtml5OwningUTF16Buffer> tail;
  tail.swap(next);
  while (tail && tail->mRefCnt == 1) {
    nsRefPtr<nsHtml5OwningUTF16Buffer> tmp;
    tmp.swap(tail->next);
    tail.swap(tmp);
  }
}


already_AddRefed<nsHtml5OwningUTF16Buffer>
nsHtml5OwningUTF16Buffer::FalliblyCreate(int32_t aLength)
{
  const mozilla::fallible_t fallible = mozilla::fallible_t();
  PRUnichar* newBuf = new (fallible) PRUnichar[aLength];
  if (!newBuf) {
    return nullptr;
  }
  nsRefPtr<nsHtml5OwningUTF16Buffer> newObj =
    new (fallible) nsHtml5OwningUTF16Buffer(newBuf);
  if (!newObj) {
    delete[] newBuf;
    return nullptr;
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
  NS_PRECONDITION(int32_t(mRefCnt) >= 0, "Illegal refcount.");
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
