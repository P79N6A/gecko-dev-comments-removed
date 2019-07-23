





































const DATA = "";
const FILENAME = "test.txt";
const CRC = 0x00000000;
const time = Date.now();

function testpass(source)
{
  
  do_check_true(source.hasEntry(FILENAME));

  var entry = source.getEntry(FILENAME);
  do_check_neq(entry, null);

  do_check_false(entry.isDirectory);

  
  do_check_eq(entry.compression, ZIP_METHOD_DEFLATE);

  
  do_check_eq(entry.realSize, DATA.length);

  
  do_check_eq(entry.CRC32, CRC);
}

function run_test()
{
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);

  
  do_check_false(zipW.hasEntry(FILENAME));

  do_check_false(zipW.inQueue);

  var file = do_get_file(DATA_DIR + "emptyfile.txt");
  zipW.addEntryFile(FILENAME, Ci.nsIZipWriter.COMPRESSION_BEST, file, false);

  
  testpass(zipW);
  zipW.close();

  
  zipW.open(tmpFile, PR_RDWR);
  testpass(zipW);
  zipW.close();

  
  var zipR = new ZipReader(tmpFile);
  testpass(zipR);
  zipR.test(FILENAME);
  var stream = Cc["@mozilla.org/scriptableinputstream;1"]
                .createInstance(Ci.nsIScriptableInputStream);
  stream.init(zipR.getInputStream(FILENAME));
  var result = stream.read(DATA.length);
  stream.close();
  zipR.close();

  do_check_eq(result, DATA);
}
