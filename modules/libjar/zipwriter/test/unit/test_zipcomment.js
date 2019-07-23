





































const DATA = "ZIP WRITER TEST COMMENT";
const DATA2 = "ANOTHER ONE";

function run_test()
{
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  zipW.comment = DATA;
  zipW.close();

  
  do_check_true(tmpFile.exists());

  
  
  do_check_eq(tmpFile.fileSize, ZIP_EOCDR_HEADER_SIZE + DATA.length);

  zipW.open(tmpFile, PR_RDWR);
  
  do_check_eq(zipW.comment, DATA);
  zipW.comment = DATA2;
  zipW.close();

  
  tmpFile = tmpFile.clone();

  
  
  do_check_eq(tmpFile.fileSize, ZIP_EOCDR_HEADER_SIZE + DATA2.length);
}
