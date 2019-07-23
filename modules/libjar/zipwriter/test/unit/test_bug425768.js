





































const DIRNAME = "test/";
const time = Date.now();

function run_test()
{
  
  var source = do_get_file("data/test.zip");
  source.copyTo(tmpFile.parent, tmpFile.leafName);

  
  zipW.open(tmpFile, PR_RDWR | PR_APPEND);
  zipW.addEntryDirectory(DIRNAME, time * PR_USEC_PER_MSEC, false);
  do_check_true(zipW.hasEntry(DIRNAME));
  zipW.close();

  var zipR = new ZipReader(tmpFile);
  do_check_true(zipR.hasEntry(DIRNAME));
  zipR.close();

  
  
  var extra = ZIP_FILE_HEADER_SIZE + ZIP_CDS_HEADER_SIZE + (DIRNAME.length * 2);
  do_check_eq(source.fileSize + extra, tmpFile.fileSize);
}
