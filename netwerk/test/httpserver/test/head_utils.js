





































do_import_script("netwerk/test/httpserver/httpd.js");








function createServer()
{
  return new nsHttpServer();
}







function makeChannel(url)
{
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null)
                .QueryInterface(Ci.nsIHttpChannel);

  return chan;
}







function makeBIS(stream)
{
  var bis = Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(stream);
  return bis;
}
