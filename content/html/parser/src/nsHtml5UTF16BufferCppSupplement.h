




































nsHtml5UTF16Buffer::nsHtml5UTF16Buffer(PRInt32 size)
  : buffer(new PRUnichar[size]),
    start(0),
    end(0),
    next(nsnull),
    key(nsnull)
{
}

nsHtml5UTF16Buffer::nsHtml5UTF16Buffer(void* key)
  : buffer(nsnull),
    start(0),
    end(0),
    next(nsnull),
    key(key)
{
}

nsHtml5UTF16Buffer::~nsHtml5UTF16Buffer()
{
  delete[] buffer;
}
