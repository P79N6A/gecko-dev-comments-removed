


function run_test() {
  var StorageStream = Components.Constructor("@mozilla.org/storagestream;1",
                                             "nsIStorageStream",
                                             "init");
  var ConverterInputStream = Components.Constructor("@mozilla.org/intl/converter-input-stream;1",
                                                    "nsIConverterInputStream",
                                                    "init");
  var ConverterOutputStream = Components.Constructor("@mozilla.org/intl/converter-output-stream;1",
                                                     "nsIConverterOutputStream",
                                                     "init");



  var storage = new StorageStream(1024, -1, null);

  
  var outStr = storage.getOutputStream(0);
  var out = new ConverterOutputStream(outStr, "UTF-8", 1024, 0xFFFD);
  out.writeString("Foo.");
  out.close();
  out.close(); 

  
  var inStr = storage.newInputStream(0);
  var inp = new ConverterInputStream(inStr, "UTF-8", 1024, 0xFFFD);
  inp.close();
  inp.close(); 
}
