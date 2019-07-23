





































const FILENAME = "missing.txt";

var observer = {
  onStartRequest: function(request, context)
  {
  },

  onStopRequest: function(request, context, status)
  {
    do_check_eq(status, Components.results.NS_ERROR_FILE_NOT_FOUND);
    zipW.close();
    do_check_eq(ZIP_EOCDR_HEADER_SIZE, tmpFile.fileSize);
    do_test_finished();
  }
};

function run_test()
{
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  zipW.removeEntry(FILENAME, true);
  do_test_pending();
  zipW.processQueue(observer, null);

  
  do_check_false(zipW.inQueue);
}
