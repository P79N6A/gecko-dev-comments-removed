

const PR_RDONLY = 0x1;  

function getDir(key) {
  var dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties);
  return dirSvc.get(key, Components.interfaces.nsILocalFile);
}



function test_stream(stream) {
  
  
  do_check_eq(stream.isNonBlocking(), false);
  
  
  
  var binstream = Components.classes['@mozilla.org/binaryinputstream;1']
                            .createInstance(Components.interfaces.nsIBinaryInputStream);
  binstream.setInputStream(stream);

  var numread = 0;
  for (;;) {
    do_check_eq(stream.available(), binstream.available());
    var avail = stream.available();
    do_check_neq(avail, -1);

    
    
    do_check_neq(avail, Math.pow(2, 32) - 1);
    do_check_neq(avail, Math.pow(2, 31) - 1);

    if (!avail) {
      
      
      var could_read = false;
      try {
        binstream.readByteArray(1);
        could_read = true;
      } catch (e) {
        
      }
      if (could_read)
        do_throw("Data readable when available indicated EOF!");
      return numread;
    }

    dump("Trying to read " + avail + " bytes\n");
    
    
    var data = binstream.readByteArray(avail);

    numread += avail;
  }
  return numread;
}

function stream_for_file(file) {
  var str = Components.classes["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Components.interfaces.nsIFileInputStream);
  str.init(file, PR_RDONLY, 0, 0);
  return str;
}

function stream_from_channel(file) {
  var ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
  var uri = ios.newFileURI(file);
  return ios.newChannelFromURI(uri).open();
}

function run_test() {
  
  var file = getDir("XpcomLib");
  var len = file.fileSize;
  do_check_eq(test_stream(stream_for_file(file)), len);
  do_check_eq(test_stream(stream_from_channel(file)), len);
  var dir = file.parent;
  test_stream(stream_from_channel(dir)); 
}

