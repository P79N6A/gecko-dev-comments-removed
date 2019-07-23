




































  public:
    nsHtml5UTF16Buffer(PRInt32 size);
    nsHtml5UTF16Buffer(void* key);
    ~nsHtml5UTF16Buffer();
    nsRefPtr<nsHtml5UTF16Buffer> next;
    void* key;
    nsrefcnt AddRef();
    nsrefcnt Release();
  private:
    nsAutoRefCnt mRefCnt;

