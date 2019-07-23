






































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
      do_check_true(zipR.hasEntry(TESTS[i].name));

      var source = do_get_file(DATA_DIR + TESTS[i].name);
      var entry = zipR.getEntry(TESTS[i].name);
      do_check_eq(entry.realSize, TESTS[i].size);
      do_check_eq(entry.size, TESTS[i].size);
      do_check_eq(entry.CRC32, TESTS[i].crc);
      do_check_eq(Math.floor(entry.lastModifiedTime / PR_USEC_PER_SEC),
                  Math.floor(source.lastModifiedTime / PR_MSEC_PER_SEC));

      zipR.test(TESTS[i].name);
    }

    zipR.close();
    do_test_finished();
  }
};

function run_test()
{
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);

  for (var i = 0; i < TESTS.length; i++) {
    var source = do_get_file(DATA_DIR+TESTS[i].name);
    zipW.addEntryFile(TESTS[i].name, Ci.nsIZipWriter.COMPRESSION_NONE, source,
                      true);
    size += ZIP_FILE_HEADER_SIZE + ZIP_CDS_HEADER_SIZE +
            (ZIP_EXTENDED_TIMESTAMP_SIZE * 2) +
            (TESTS[i].name.length*2) + TESTS[i].size;
  }
  do_test_pending();
  zipW.processQueue(observer, null);
  do_check_true(zipW.inQueue);
}
