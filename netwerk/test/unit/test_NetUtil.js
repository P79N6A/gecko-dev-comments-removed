










































do_load_httpd_js();

Components.utils.import("resource://gre/modules/NetUtil.jsm");



do_get_profile();

const OUTPUT_STREAM_CONTRACT_ID = "@mozilla.org/network/file-output-stream;1";
const SAFE_OUTPUT_STREAM_CONTRACT_ID = "@mozilla.org/network/safe-file-output-stream;1";











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









function async_write_file(aContractId, aDeferOpen)
{
  do_test_pending();

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("ProfD", Ci.nsIFile);
  file.append("NetUtil-async-test-file.tmp");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let ostream = Cc[aContractId].createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, -1, -1, aDeferOpen ? Ci.nsIFileOutputStream.DEFER_OPEN : 0);

  
  const TEST_DATA = "this is a test string";
  let istream = Cc["@mozilla.org/io/string-input-stream;1"].
                createInstance(Ci.nsIStringInputStream);
  istream.setData(TEST_DATA, TEST_DATA.length);

  NetUtil.asyncCopy(istream, ostream, function(aResult) {
    
    do_check_true(Components.isSuccessCode(aResult));

    
    do_check_eq(TEST_DATA, getFileContents(file));

    
    do_test_finished();
    run_next_test();
  });
}




function test_async_write_file() {
  async_write_file(OUTPUT_STREAM_CONTRACT_ID);
}

function test_async_write_file_deferred() {
  async_write_file(OUTPUT_STREAM_CONTRACT_ID, true);
}

function test_async_write_file_safe() {
  async_write_file(SAFE_OUTPUT_STREAM_CONTRACT_ID);
}

function test_async_write_file_safe_deferred() {
  async_write_file(SAFE_OUTPUT_STREAM_CONTRACT_ID, true);
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

function test_newURI_takes_nsIFile()
{
  let ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("ProfD", Ci.nsIFile);
  file.append("NetUtil-test-file.tmp");

  
  
  let iosURI = ios.newFileURI(file);
  let NetUtilURI = NetUtil.newURI(file);
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

function test_asyncFetch_with_nsIChannel()
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

function test_asyncFetch_with_nsIURI()
{
  const TEST_DATA = "this is a test string";

  
  let server = new nsHttpServer();
  server.registerPathHandler("/test", function(aRequest, aResponse) {
    aResponse.setStatusLine(aRequest.httpVersion, 200, "OK");
    aResponse.setHeader("Content-Type", "text/plain", false);
    aResponse.write(TEST_DATA);
  });
  server.start(4444);

  
  let uri = NetUtil.newURI("http://localhost:4444/test");

  
  NetUtil.asyncFetch(uri, function(aInputStream, aResult) {
    
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

function test_asyncFetch_with_string()
{
  const TEST_DATA = "this is a test string";

  
  let server = new nsHttpServer();
  server.registerPathHandler("/test", function(aRequest, aResponse) {
    aResponse.setStatusLine(aRequest.httpVersion, 200, "OK");
    aResponse.setHeader("Content-Type", "text/plain", false);
    aResponse.write(TEST_DATA);
  });
  server.start(4444);

  
  NetUtil.asyncFetch("http://localhost:4444/test", function(aInputStream,
                                                            aResult) {
    
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

function test_asyncFetch_with_nsIFile()
{
  const TEST_DATA = "this is a test string";

  
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("ProfD", Ci.nsIFile);
  file.append("NetUtil-asyncFetch-test-file.tmp");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let ostream = Cc["@mozilla.org/network/file-output-stream;1"].
                createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, -1, -1, 0);
  ostream.write(TEST_DATA, TEST_DATA.length);

  
  do_check_eq(TEST_DATA, getFileContents(file));

  
  NetUtil.asyncFetch(file, function(aInputStream, aResult) {
    
    do_check_true(Components.isSuccessCode(aResult));

    
    do_check_eq(aInputStream.available(), TEST_DATA.length);
    let is = Cc["@mozilla.org/scriptableinputstream;1"].
             createInstance(Ci.nsIScriptableInputStream);
    is.init(aInputStream);
    let result = is.read(TEST_DATA.length);
    do_check_eq(TEST_DATA, result);

    run_next_test();
  });
}

function test_asyncFetch_with_nsIInputString()
{
  const TEST_DATA = "this is a test string";
  let istream = Cc["@mozilla.org/io/string-input-stream;1"].
                createInstance(Ci.nsIStringInputStream);
  istream.setData(TEST_DATA, TEST_DATA.length);

  
  NetUtil.asyncFetch(istream, function(aInputStream, aResult) {
    
    do_check_true(Components.isSuccessCode(aResult));

    
    do_check_eq(aInputStream.available(), TEST_DATA.length);
    do_check_eq(NetUtil.readInputStreamToString(aInputStream, TEST_DATA.length),
                TEST_DATA);

    run_next_test();
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

function test_newChannel_no_specifier()
{
  try {
    NetUtil.newChannel();
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_newChannel_with_string()
{
  const TEST_SPEC = "http://mozilla.org";

  
  
  let ios = NetUtil.ioService;
  let iosChannel = ios.newChannel(TEST_SPEC, null, null);
  let NetUtilChannel = NetUtil.newChannel(TEST_SPEC);
  do_check_true(iosChannel.URI.equals(NetUtilChannel.URI));

  run_next_test();
}

function test_newChannel_with_nsIURI()
{
  const TEST_SPEC = "http://mozilla.org";

  
  
  let uri = NetUtil.newURI(TEST_SPEC);
  let iosChannel = NetUtil.ioService.newChannelFromURI(uri);
  let NetUtilChannel = NetUtil.newChannel(uri);
  do_check_true(iosChannel.URI.equals(NetUtilChannel.URI));

  run_next_test();
}

function test_newChannel_with_nsIFile()
{
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("ProfD", Ci.nsIFile);
  file.append("NetUtil-test-file.tmp");

  
  
  let uri = NetUtil.newURI(file);
  let iosChannel = NetUtil.ioService.newChannelFromURI(uri);
  let NetUtilChannel = NetUtil.newChannel(uri);
  do_check_true(iosChannel.URI.equals(NetUtilChannel.URI));

  run_next_test();
}

function test_readInputStreamToString()
{
  const TEST_DATA = "this is a test string\0 with an embedded null";
  let istream = Cc["@mozilla.org/io/string-input-stream;1"].
                createInstance(Ci.nsISupportsCString);
  istream.data = TEST_DATA;

  do_check_eq(NetUtil.readInputStreamToString(istream, TEST_DATA.length),
              TEST_DATA);

  run_next_test();
}

function test_readInputStreamToString_no_input_stream()
{
  try {
    NetUtil.readInputStreamToString("hi", 2);
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_readInputStreamToString_no_bytes_arg()
{
  const TEST_DATA = "this is a test string";
  let istream = Cc["@mozilla.org/io/string-input-stream;1"].
                createInstance(Ci.nsIStringInputStream);
  istream.setData(TEST_DATA, TEST_DATA.length);

  try {
    NetUtil.readInputStreamToString(istream);
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_readInputStreamToString_blocking_stream()
{
  let pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
  pipe.init(true, true, 0, 0, null);

  try {
    NetUtil.readInputStreamToString(pipe.inputStream, 10);
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_BASE_STREAM_WOULD_BLOCK);
  }
  run_next_test();
}

function test_readInputStreamToString_too_many_bytes()
{
  const TEST_DATA = "this is a test string";
  let istream = Cc["@mozilla.org/io/string-input-stream;1"].
                createInstance(Ci.nsIStringInputStream);
  istream.setData(TEST_DATA, TEST_DATA.length);

  try {
    NetUtil.readInputStreamToString(istream, TEST_DATA.length + 10);
    do_throw("should throw!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_FAILURE);
  }

  run_next_test();
}




[
  test_async_write_file,
  test_async_write_file_deferred,
  test_async_write_file_safe,
  test_async_write_file_safe_deferred,
  test_newURI_no_spec_throws,
  test_newURI,
  test_newURI_takes_nsIFile,
  test_ioService,
  test_asyncFetch_no_channel,
  test_asyncFetch_no_callback,
  test_asyncFetch_with_nsIChannel,
  test_asyncFetch_with_nsIURI,
  test_asyncFetch_with_string,
  test_asyncFetch_with_nsIFile,
  test_asyncFetch_with_nsIInputString,
  test_asyncFetch_does_not_block,
  test_newChannel_no_specifier,
  test_newChannel_with_string,
  test_newChannel_with_nsIURI,
  test_newChannel_with_nsIFile,
  test_readInputStreamToString,
  test_readInputStreamToString_no_input_stream,
  test_readInputStreamToString_no_bytes_arg,
  test_readInputStreamToString_blocking_stream,
  test_readInputStreamToString_too_many_bytes,
].forEach(add_test);
let index = 0;

function run_test()
{
  run_next_test();
}

