










































do_load_httpd_js();

Components.utils.import("resource://gre/modules/NetUtil.jsm");











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

function test_async_write_file_nsISafeOutputStream()
{
  do_test_pending();

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("TmpD", Ci.nsIFile);
  file.append("NetUtil-async-test-file.tmp");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let ostream = Cc["@mozilla.org/network/safe-file-output-stream;1"].
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

function test_newURI_no_spec_throws()
{
  try {
    NetUtil.newURI();
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_newURI()
{
  let ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  
  const TEST_URI = "http://mozilla.org";
  let iosURI = ios.newURI(TEST_URI, null, null);
  let NetUtilURI = NetUtil.newURI(TEST_URI);
  do_check_true(iosURI.equals(NetUtilURI));

  run_next_test();
}

function test_ioService()
{
  do_check_true(NetUtil.ioService instanceof Ci.nsIIOService);
  run_next_test();
}

function test_asyncFetch_no_channel()
{
  try {
    NetUtil.asyncFetch(null, function() { });
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_asyncFetch_no_callback()
{
  try {
    NetUtil.asyncFetch({ });
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_asyncFetch()
{
  const TEST_DATA = "this is a test string";

  
  let server = new nsHttpServer();
  server.registerPathHandler("/test", function(aRequest, aResponse) {
    aResponse.setStatusLine(aRequest.httpVersion, 200, "OK");
    aResponse.setHeader("Content-Type", "text/plain", false);
    aResponse.write(TEST_DATA);
  });
  server.start(4444);

  
  let channel = NetUtil.ioService.
                newChannel("http://localhost:4444/test", null, null);

  
  NetUtil.asyncFetch(channel, function(aInputStream, aResult) {
    
    do_check_true(Components.isSuccessCode(aResult));

    
    do_check_eq(aInputStream.available(), TEST_DATA.length);
    let is = Cc["@mozilla.org/scriptableinputstream;1"].
             createInstance(Ci.nsIScriptableInputStream);
    is.init(aInputStream);
    let result = is.read(TEST_DATA.length);
    do_check_eq(TEST_DATA, result);

    server.stop(run_next_test);
  });
}

function test_asyncFetch_does_not_block()
{
  
  let channel = NetUtil.ioService.
                newChannel("data:text/plain,", null, null);

  
  NetUtil.asyncFetch(channel, function(aInputStream, aResult) {
    
    do_check_true(Components.isSuccessCode(aResult));

    
    
    let is = Cc["@mozilla.org/scriptableinputstream;1"].
             createInstance(Ci.nsIScriptableInputStream);
    is.init(aInputStream);
    try {
      is.read(1);
      do_throw("should throw!");
    }
    catch (e) {
      do_check_eq(e.result, Cr.NS_BASE_STREAM_CLOSED);
    }

    run_next_test();
  });
}




let tests = [
  test_async_write_file,
  test_async_write_file_nsISafeOutputStream,
  test_newURI_no_spec_throws,
  test_newURI,
  test_ioService,
  test_asyncFetch_no_channel,
  test_asyncFetch_no_callback,
  test_asyncFetch,
  test_asyncFetch_does_not_block,
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

