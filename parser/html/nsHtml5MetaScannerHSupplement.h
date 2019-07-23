



































 
private:
  nsCOMPtr<nsIUnicodeDecoder>  mUnicodeDecoder;
  nsCString mCharset;
  inline PRInt32 read() {
    return readable->read();
  }
public:
  void sniff(nsHtml5ByteReadable* bytes, nsIUnicodeDecoder** decoder, nsACString& charset);
  nsHtml5MetaScanner();
  ~nsHtml5MetaScanner();
