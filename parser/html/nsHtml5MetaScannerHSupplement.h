


 
private:
  nsCOMPtr<nsIUnicodeDecoder>  mUnicodeDecoder;
  nsCString mCharset;
  inline int32_t read() {
    return readable->read();
  }
public:
  void sniff(nsHtml5ByteReadable* bytes, nsIUnicodeDecoder** decoder, nsACString& charset);
