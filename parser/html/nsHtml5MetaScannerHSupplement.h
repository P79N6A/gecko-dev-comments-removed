


 
private:
  nsCString mCharset;
  inline int32_t read()
  {
    return readable->read();
  }
public:
  void sniff(nsHtml5ByteReadable* bytes, nsACString& charset);
