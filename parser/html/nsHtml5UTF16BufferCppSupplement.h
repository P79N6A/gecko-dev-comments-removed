




































nsHtml5UTF16Buffer::nsHtml5UTF16Buffer(PRUnichar* aBuffer, PRInt32 aEnd)
  : buffer(aBuffer)
  , start(0)
  , end(aEnd)
{
  MOZ_COUNT_CTOR(nsHtml5UTF16Buffer);
}

nsHtml5UTF16Buffer::~nsHtml5UTF16Buffer()
{
  MOZ_COUNT_DTOR(nsHtml5UTF16Buffer);
}

void
nsHtml5UTF16Buffer::DeleteBuffer()
{
  delete[] buffer;
}
