






































var TESTS = [
  "test.txt",
  "test.png"
];

function run_test()
{
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);

  for (var i = 0; i < TESTS.length; i++) {
    var source = do_get_file(DATA_DIR + TESTS[i]);
    zipW.addEntryFile(TESTS[i], Ci.nsIZipWriter.COMPRESSION_NONE, source,
                      false);
  }

  try {
    var source = do_get_file(DATA_DIR + TESTS[0]);
    zipW.addEntryFile(TESTS[0], Ci.nsIZipWriter.COMPRESSION_NONE, source,
                      false);
    do_throw("Should not be able to add the same file twice");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_FILE_ALREADY_EXISTS);
  }

  
  for (var i = 0; i < TESTS.length; i++) {
    zipW.removeEntry(TESTS[i], false);
  }

  zipW.close();

  
  var newTmpFile = tmpFile.clone();
  do_check_eq(newTmpFile.fileSize, ZIP_EOCDR_HEADER_SIZE);
}
