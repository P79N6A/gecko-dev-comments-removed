




const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");


var TESTS = [
  {
    name: "test.txt",
    size: 232,
    crc: 0x0373ac26
  },
  {
    name: "test.png",
    size: 3402,
    crc: 0x504a5c30
  }
];

var size = 0;

var observer = {
  onStartRequest: function(request, context)
  {
  },

  onStopRequest: function(request, context, status)
  {
    do_check_eq(status, Components.results.NS_OK);

    zipW.close();
    size += ZIP_EOCDR_HEADER_SIZE;

    do_check_eq(size, tmpFile.fileSize);

    
    var zipR = new ZipReader(tmpFile);

    for (var i = 0; i < TESTS.length; i++) {
      var source = do_get_file(DATA_DIR + TESTS[i].name);
      for (let method in methods) {
        var entryName = method + "/" + TESTS[i].name;
        do_check_true(zipR.hasEntry(entryName));

        var entry = zipR.getEntry(entryName);
        do_check_eq(entry.realSize, TESTS[i].size);
        do_check_eq(entry.size, TESTS[i].size);
        do_check_eq(entry.CRC32, TESTS[i].crc);
        do_check_eq(Math.floor(entry.lastModifiedTime / PR_USEC_PER_SEC),
                    Math.floor(source.lastModifiedTime / PR_MSEC_PER_SEC));

        zipR.test(entryName);
      }
    }

    zipR.close();
    do_test_finished();
  }
};

var methods = {
  file: function method_file(entry, source)
  {
    zipW.addEntryFile(entry, Ci.nsIZipWriter.COMPRESSION_NONE, source,
                      true);
  },
  channel: function method_channel(entry, source)
  {
    zipW.addEntryChannel(entry, source.lastModifiedTime * PR_MSEC_PER_SEC,
                         Ci.nsIZipWriter.COMPRESSION_NONE,
                         ioSvc.newChannelFromURI2(ioSvc.newFileURI(source),
                                                  null,      
                                                  Services.scriptSecurityManager.getSystemPrincipal(),
                                                  null,      
                                                  Ci.nsILoadInfo.SEC_NORMAL,
                                                  Ci.nsIContentPolicy.TYPE_OTHER), true);
  },
  stream: function method_stream(entry, source)
  {
    zipW.addEntryStream(entry, source.lastModifiedTime * PR_MSEC_PER_SEC,
                        Ci.nsIZipWriter.COMPRESSION_NONE,
                        ioSvc.newChannelFromURI2(ioSvc.newFileURI(source),
                                                 null,      
                                                 Services.scriptSecurityManager.getSystemPrincipal(),
                                                 null,      
                                                 Ci.nsILoadInfo.SEC_NORMAL,
                                                 Ci.nsIContentPolicy.TYPE_OTHER).open(), true);
  }
}

function run_test()
{
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);

  for (var i = 0; i < TESTS.length; i++) {
    var source = do_get_file(DATA_DIR+TESTS[i].name);
    for (let method in methods) {
      var entry = method + "/" + TESTS[i].name;
      methods[method](entry, source);
      size += ZIP_FILE_HEADER_SIZE + ZIP_CDS_HEADER_SIZE +
              (ZIP_EXTENDED_TIMESTAMP_SIZE * 2) +
              (entry.length*2) + TESTS[i].size;
    }
  }
  do_test_pending();
  zipW.processQueue(observer, null);
  do_check_true(zipW.inQueue);
}
