




const DATA = "ZIP WRITER TEST DATA";
const FILENAME = "test_data.txt";
const CRC = 0xe6164331;
const time = 1199145600000; 

var TESTS = [
  {
    name: "test.txt",
    compression: Ci.nsIZipWriter.COMPRESSION_DEFAULT
  },
  {
    name: "test.png",
    compression: Ci.nsIZipWriter.COMPRESSION_NONE
  }
];

function swap16(n)
{
  return (((n >> 8) & 0xFF) <<  0) |
         (((n >>  0) & 0xFF) << 8);
}

function swap32(n)
{
  return (((n >> 24) & 0xFF) <<  0) |
         (((n >> 16) & 0xFF) <<  8) |
         (((n >>  8) & 0xFF) << 16) |
         (((n >>  0) & 0xFF) << 24);
}

function move_to_data(bis, offset)
{
  bis.readBytes(18); 
  var size = swap32(bis.read32());
  bis.readBytes(4);
  var file_len = swap16(bis.read16());
  var extra_len = swap16(bis.read16());
  var file = bis.readBytes(file_len);
  bis.readBytes(extra_len);
  offset += ZIP_FILE_HEADER_SIZE + file_len + extra_len;

  return {offset: offset, size: size};
}

function test_alignment(align_size)
{
  
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  for (var i = 0; i < TESTS.length; i++) {
    var source = do_get_file(DATA_DIR + TESTS[i].name);
    zipW.addEntryFile(TESTS[i].name, TESTS[i].compression, source, false);
  }
  var stream = Cc["@mozilla.org/io/string-input-stream;1"]
                .createInstance(Ci.nsIStringInputStream);
  stream.setData(DATA, DATA.length);
  zipW.addEntryStream(FILENAME, time * PR_USEC_PER_MSEC,
                      Ci.nsIZipWriter.COMPRESSION_NONE, stream, false);
  zipW.alignStoredFiles(align_size);
  zipW.close();

  
  var zipR = new ZipReader(tmpFile);
  var stream = Cc["@mozilla.org/scriptableinputstream;1"]
                .createInstance(Ci.nsIScriptableInputStream);
  stream.init(zipR.getInputStream(FILENAME));
  var result = stream.read(DATA.length);
  do_check_eq(result, DATA);
  stream.close();
  zipR.close();

  
  var fis = Cc["@mozilla.org/network/file-input-stream;1"]
                 .createInstance(Ci.nsIFileInputStream);
  fis.init(tmpFile, -1, -1, null);
  let bis = Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(fis);
  var offset = 0;

  var ret = move_to_data(bis, offset); 
  offset = ret.offset;
  bis.readBytes(ret.size);
  offset += ret.size;

  ret = move_to_data(bis, offset); 
  offset = ret.offset;
  do_check_eq(offset % align_size, 0);
  bis.readBytes(ret.size);
  offset += ret.size;

  ret = move_to_data(bis, offset); 
  offset = ret.offset;
  var result = bis.readBytes(DATA.length);
  do_check_eq(result, DATA);
  do_check_eq(offset % align_size, 0);

  fis.close();
  bis.close();
}

function run_test()
{
  test_alignment(2);
  test_alignment(4);
  test_alignment(16);
  test_alignment(4096);
  test_alignment(32768);
}
