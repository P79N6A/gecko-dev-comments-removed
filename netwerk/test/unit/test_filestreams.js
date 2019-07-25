


let Cc = Components.classes;
let Ci = Components.interfaces;



do_get_profile();

const OUTPUT_STREAM_CONTRACT_ID = "@mozilla.org/network/file-output-stream;1";
const SAFE_OUTPUT_STREAM_CONTRACT_ID = "@mozilla.org/network/safe-file-output-stream;1";












function ensure_unique(aFile)
{
  ensure_unique.fileIndex = ensure_unique.fileIndex || 0;

  var leafName = aFile.leafName;
  while (aFile.clone().exists()) {
    aFile.leafName = leafName + "_" + (ensure_unique.fileIndex++);
  }
}



















function check_access(aContractId, aDeferOpen, aTrickDeferredOpen)
{
  const LEAF_NAME = "filestreams-test-file.tmp";
  const TRICKY_LEAF_NAME = "BetYouDidNotExpectThat.tmp";
  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("ProfD", Ci.nsIFile);
  file.append(LEAF_NAME);

  

  ensure_unique(file);
  let ostream = Cc[aContractId].createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, -1, -1, aDeferOpen ? Ci.nsIFileOutputStream.DEFER_OPEN : 0);
  do_check_eq(aDeferOpen, !file.clone().exists()); 
  if (aDeferOpen) {
    
    if (aTrickDeferredOpen) {
      
      file.leafName = TRICKY_LEAF_NAME;
    }
    ostream.write("data", 4);
    if (aTrickDeferredOpen) {
      file.leafName = LEAF_NAME;
    }
    
    do_check_true(file.clone().exists());
  }
  ostream.close();

  

  ensure_unique(file);
  let istream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  var initOk, getOk;
  try {
    istream.init(file, -1, 0, aDeferOpen ? Ci.nsIFileInputStream.DEFER_OPEN : 0);
    initOk = true;
  }
  catch(e) {
    initOk = false;
  }
  try {
    let fstream = Cc["@mozilla.org/network/file-input-stream;1"].
                  createInstance(Ci.nsIFileInputStream);
    fstream.init(aFile, -1, 0, 0);
    getOk = true;
  }
  catch(e) {
    getOk = false;
  }

  
  
  
  
  do_check_true( (aDeferOpen && initOk && !getOk) ||
                 (!aDeferOpen && !initOk && !getOk) );
  istream.close();
}








function sync_operations(aDeferOpen)
{
  const TEST_DATA = "this is a test string";
  const LEAF_NAME = "filestreams-test-file.tmp";

  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("ProfD", Ci.nsIFile);
  file.append(LEAF_NAME);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  let ostream = Cc[OUTPUT_STREAM_CONTRACT_ID].
                createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, -1, -1, aDeferOpen ? Ci.nsIFileOutputStream.DEFER_OPEN : 0);

  ostream.write(TEST_DATA, TEST_DATA.length);
  ostream.close();

  let fstream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  fstream.init(file, -1, 0, aDeferOpen ? Ci.nsIFileInputStream.DEFER_OPEN : 0);

  let cstream = Cc["@mozilla.org/intl/converter-input-stream;1"].
                createInstance(Ci.nsIConverterInputStream);
  cstream.init(fstream, "UTF-8", 0, 0);

  let string  = {};
  cstream.readString(-1, string);
  cstream.close();
  fstream.close();

  do_check_eq(string.value, TEST_DATA);
}




function test_access()
{
  check_access(OUTPUT_STREAM_CONTRACT_ID, false, false);
}

function test_access_trick()
{
  check_access(OUTPUT_STREAM_CONTRACT_ID, false, true);
}

function test_access_defer()
{
  check_access(OUTPUT_STREAM_CONTRACT_ID, true, false);
}

function test_access_defer_trick()
{
  check_access(OUTPUT_STREAM_CONTRACT_ID, true, true);
}

function test_access_safe()
{
  check_access(SAFE_OUTPUT_STREAM_CONTRACT_ID, false, false);
}

function test_access_safe_trick()
{
  check_access(SAFE_OUTPUT_STREAM_CONTRACT_ID, false, true);
}

function test_access_safe_defer()
{
  check_access(SAFE_OUTPUT_STREAM_CONTRACT_ID, true, false);
}

function test_access_safe_defer_trick()
{
  check_access(SAFE_OUTPUT_STREAM_CONTRACT_ID, true, true);
}

function test_sync_operations()
{
  sync_operations();
}

function test_sync_operations_deferred()
{
  sync_operations(true);
}

function do_test_zero_size_buffered(disableBuffering)
{
  const LEAF_NAME = "filestreams-test-file.tmp";
  const BUFFERSIZE = 4096;

  let file = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get("ProfD", Ci.nsIFile);
  file.append(LEAF_NAME);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  let fstream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  fstream.init(file, -1, 0,
               Ci.nsIFileInputStream.CLOSE_ON_EOF |
               Ci.nsIFileInputStream.REOPEN_ON_REWIND);

  var buffered = Cc["@mozilla.org/network/buffered-input-stream;1"].
                   createInstance(Ci.nsIBufferedInputStream);
  buffered.init(fstream, BUFFERSIZE);

  if (disableBuffering) {
      buffered.QueryInterface(Ci.nsIStreamBufferAccess).disableBuffering();
  }

  
  
  let cstream = Cc["@mozilla.org/intl/converter-input-stream;1"].
                createInstance(Ci.nsIConverterInputStream);
  cstream.init(buffered, "UTF-8", 0, 0);

  do_check_eq(buffered.available(), 0);

  
  let string = {};
  do_check_eq(cstream.readString(BUFFERSIZE, string), 0);
  do_check_eq(string.value, "");

  
  var exceptionThrown = false;
  try {
    do_check_eq(buffered.available(), 0);
  } catch (e) {
    exceptionThrown = true;
  }
  do_check_true(exceptionThrown);

  
  buffered.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);

  
  exceptionThrown = false;
  try {
    do_check_eq(buffered.available(), 0);
  } catch (e) {
    exceptionThrown = true;
  }
  do_check_false(exceptionThrown);
}

function test_zero_size_buffered()
{
    do_test_zero_size_buffered(false);
    do_test_zero_size_buffered(true);
}




let tests = [
  test_access,
  test_access_trick,
  test_access_defer,
  test_access_defer_trick,
  test_access_safe,
  test_access_safe_trick,
  test_access_safe_defer,
  test_access_safe_defer_trick,
  test_sync_operations,
  test_sync_operations_deferred,
  test_zero_size_buffered,
];

function run_test()
{
  tests.forEach(function(test) {
    test();
  });
}

