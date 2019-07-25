




function run_test()
{
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  zipW.close();

  
  do_check_true(tmpFile.exists());

  
  do_check_eq(tmpFile.fileSize, ZIP_EOCDR_HEADER_SIZE);
}
