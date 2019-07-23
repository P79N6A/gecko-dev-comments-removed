










































Components.utils.import("resource://gre/modules/NetUtil.jsm");
const Cc = Components.classes;
const Ci = Components.interfaces;











function getFileContents(aFile)
{
  let fstream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  fstream.init(aFile, -1, 0, 0);

  let cstream = Cc["@mozilla.org/intl/converter-input-stream;1"].
                createInstance(Ci.nsIConverterInputStream);
  cstream.init(fstream, "UTF-8", 0, 0);

  let string  = {};
  cstream.readString(-1, string);
  cstream.close();
  return string.value;
}




function test_async_write_file()
{
  do_test_pending();

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("TmpD", Ci.nsIFile);
  file.append("NetUtil-async-test-file.tmp");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let ostream = Cc["@mozilla.org/network/file-output-stream;1"].
                createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, -1, -1, 0);

  
  const TEST_DATA = "this is a test string";
  let istream = Cc["@mozilla.org/io/string-input-stream;1"].
                createInstance(Ci.nsIStringInputStream);
  istream.setData(TEST_DATA, TEST_DATA.length);

  NetUtil.asyncCopy(istream, ostream, function(aResult) {
    
    do_check_true(Components.isSuccessCode(aResult));

    
    do_check_eq(TEST_DATA, getFileContents(file));

    
    file.remove(false);
    do_test_finished();
    run_next_test();
  });
}




let tests = [
  test_async_write_file,
];
let index = 0;

function run_next_test()
{
  if (index < tests.length) {
    do_test_pending();
    print("Running the next test: " + tests[index].name);
    tests[index++]();
  }

  do_test_finished();
}

function run_test()
{
  do_test_pending();
  run_next_test();
}

