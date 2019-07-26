








const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;




Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const BackgroundFileSaverOutputStream = Components.Constructor(
      "@mozilla.org/network/background-file-saver;1?mode=outputstream",
      "nsIBackgroundFileSaver");

const BackgroundFileSaverStreamListener = Components.Constructor(
      "@mozilla.org/network/background-file-saver;1?mode=streamlistener",
      "nsIBackgroundFileSaver");

const StringInputStream = Components.Constructor(
      "@mozilla.org/io/string-input-stream;1",
      "nsIStringInputStream",
      "setData");

const REQUEST_SUSPEND_AT = 1024 * 1024 * 4;
const TEST_DATA_SHORT = "This test string is written to the file.";
const TEST_FILE_NAME_1 = "test-backgroundfilesaver-1.txt";
const TEST_FILE_NAME_2 = "test-backgroundfilesaver-2.txt";
const TEST_FILE_NAME_3 = "test-backgroundfilesaver-3.txt";


const EXPECTED_HASHES = {
  
  40 : "f37176b690e8744ee990a206c086cba54d1502aa2456c3b0c84ef6345d72a192",
  
  80 : "780c0e91f50bb7ec922cc11e16859e6d5df283c0d9470f61772e3d79f41eeb58",
  
  16777216 : "03a0db69a30140f307587ee746a539247c181bafd85b85c8516a3533c7d9ea1d"
};

const gTextDecoder = new TextDecoder();


const TEST_256_CHARS = new Array(257).join("-");
const DESIRED_LENGTH = REQUEST_SUSPEND_AT * 2;
const TEST_DATA_LONG = new Array(1 + DESIRED_LENGTH / 256).join(TEST_256_CHARS);
do_check_eq(TEST_DATA_LONG.length, DESIRED_LENGTH);





function getTempFile(aLeafName) {
  let file = FileUtils.getFile("TmpD", [aLeafName]);
  do_register_cleanup(function GTF_cleanup() {
    if (file.exists()) {
      file.remove(false);
    }
  });
  return file;
}








function toHex(str) {
  var hex = '';
  for (var i = 0; i < str.length; i++) {
    hex += ('0' + str.charCodeAt(i).toString(16)).slice(-2);
  }
  return hex;
}













function promiseVerifyContents(aFile, aExpectedContents) {
  let deferred = Promise.defer();
  NetUtil.asyncFetch(aFile, function(aInputStream, aStatus) {
    do_check_true(Components.isSuccessCode(aStatus));
    let contents = NetUtil.readInputStreamToString(aInputStream,
                                                   aInputStream.available());
    if (contents.length <= TEST_DATA_SHORT.length * 2) {
      do_check_eq(contents, aExpectedContents);
    } else {
      
      do_check_eq(contents.length, aExpectedContents.length);
      do_check_true(contents == aExpectedContents);
    }
    deferred.resolve();
  });
  return deferred.promise;
}













function promiseSaverComplete(aSaver, aOnTargetChangeFn) {
  let deferred = Promise.defer();
  aSaver.observer = {
    onTargetChange: function BFSO_onSaveComplete(aSaver, aTarget)
    {
      if (aOnTargetChangeFn) {
        aOnTargetChangeFn(aTarget);
      }
    },
    onSaveComplete: function BFSO_onSaveComplete(aSaver, aStatus)
    {
      if (Components.isSuccessCode(aStatus)) {
        deferred.resolve();
      } else {
        deferred.reject(new Components.Exception("Saver failed.", aStatus));
      }
    },
  };
  return deferred.promise;
}















function promiseCopyToSaver(aSourceString, aSaverOutputStream, aCloseWhenDone) {
  let deferred = Promise.defer();
  let inputStream = new StringInputStream(aSourceString, aSourceString.length);
  let copier = Cc["@mozilla.org/network/async-stream-copier;1"]
               .createInstance(Ci.nsIAsyncStreamCopier);
  copier.init(inputStream, aSaverOutputStream, null, false, true, 0x8000, true,
              aCloseWhenDone);
  copier.asyncCopy({
    onStartRequest: function () { },
    onStopRequest: function (aRequest, aContext, aStatusCode)
    {
      if (Components.isSuccessCode(aStatusCode)) {
        deferred.resolve();
      } else {
        deferred.reject(new Components.Exception(aResult));
      }
    },
  }, null);
  return deferred.promise;
}















function promisePumpToSaver(aSourceString, aSaverStreamListener,
                            aCloseWhenDone) {
  let deferred = Promise.defer();
  aSaverStreamListener.QueryInterface(Ci.nsIStreamListener);
  let inputStream = new StringInputStream(aSourceString, aSourceString.length);
  let pump = Cc["@mozilla.org/network/input-stream-pump;1"]
             .createInstance(Ci.nsIInputStreamPump);
  pump.init(inputStream, -1, -1, 0, 0, true);
  pump.asyncRead({
    onStartRequest: function PPTS_onStartRequest(aRequest, aContext)
    {
      aSaverStreamListener.onStartRequest(aRequest, aContext);
    },
    onStopRequest: function PPTS_onStopRequest(aRequest, aContext, aStatusCode)
    {
      aSaverStreamListener.onStopRequest(aRequest, aContext, aStatusCode);
      if (Components.isSuccessCode(aStatusCode)) {
        deferred.resolve();
      } else {
        deferred.reject(new Components.Exception(aResult));
      }
    },
    onDataAvailable: function PPTS_onDataAvailable(aRequest, aContext,
                                                   aInputStream, aOffset,
                                                   aCount)
    {
      aSaverStreamListener.onDataAvailable(aRequest, aContext, aInputStream,
                                           aOffset, aCount);
    },
  }, null);
  return deferred.promise;
}

let gStillRunning = true;




function run_test()
{
  run_next_test();
}

add_task(function test_setup()
{
  
  do_timeout(10 * 60 * 1000, function() {
    if (gStillRunning) {
      do_throw("Test timed out.");
    }
  })
});

add_task(function test_normal()
{
  
  let destFile = getTempFile(TEST_FILE_NAME_1);

  
  let saver = new BackgroundFileSaverOutputStream();

  
  let receivedOnTargetChange = false;
  function onTargetChange(aTarget) {
    do_check_true(destFile.equals(aTarget));
    receivedOnTargetChange = true;
  }
  let completionPromise = promiseSaverComplete(saver, onTargetChange);

  
  saver.setTarget(destFile, false);

  
  yield promiseCopyToSaver(TEST_DATA_SHORT, saver, true);

  
  saver.finish(Cr.NS_OK);
  yield completionPromise;

  
  
  do_check_true(receivedOnTargetChange);

  
  destFile.remove(false);
});

add_task(function test_combinations()
{
  let initialFile = getTempFile(TEST_FILE_NAME_1);
  let renamedFile = getTempFile(TEST_FILE_NAME_2);

  
  
  for (let testFlags = 0; testFlags < 32; testFlags++) {
    let keepPartialOnFailure = !!(testFlags & 1);
    let renameAtSomePoint = !!(testFlags & 2);
    let cancelAtSomePoint = !!(testFlags & 4);
    let useStreamListener = !!(testFlags & 8);
    let useLongData = !!(testFlags & 16);

    let startTime = Date.now();
    do_print("Starting keepPartialOnFailure = " + keepPartialOnFailure +
                    ", renameAtSomePoint = " + renameAtSomePoint +
                    ", cancelAtSomePoint = " + cancelAtSomePoint +
                    ", useStreamListener = " + useStreamListener +
                    ", useLongData = " + useLongData);

    
    let currentFile = null;
    function onTargetChange(aTarget) {
      do_print("Target file changed to: " + aTarget.leafName);
      currentFile = aTarget;
    }

    
    let saver = useStreamListener
                ? new BackgroundFileSaverStreamListener()
                : new BackgroundFileSaverOutputStream();
    saver.enableSha256();
    let completionPromise = promiseSaverComplete(saver, onTargetChange);

    
    
    let testData = useLongData ? TEST_DATA_LONG : TEST_DATA_SHORT;
    let feedPromise = useStreamListener
                      ? promisePumpToSaver(testData + testData, saver)
                      : promiseCopyToSaver(testData, saver, false);

    
    saver.setTarget(initialFile, keepPartialOnFailure);

    
    yield feedPromise;

    if (renameAtSomePoint) {
      saver.setTarget(renamedFile, keepPartialOnFailure);
    }

    if (cancelAtSomePoint) {
      saver.finish(Cr.NS_ERROR_FAILURE);
    }

    
    if (!useStreamListener) {
      yield promiseCopyToSaver(testData, saver, true);
    }

    
    if (!cancelAtSomePoint) {
      saver.finish(Cr.NS_OK);
    }
    try {
      yield completionPromise;
      if (cancelAtSomePoint) {
        do_throw("Failure expected.");
      }
    } catch (ex if cancelAtSomePoint && ex.result == Cr.NS_ERROR_FAILURE) { }

    if (!cancelAtSomePoint) {
      
      do_check_true(currentFile.exists());
      expectedContents = testData + testData;
      yield promiseVerifyContents(currentFile, expectedContents);
      do_check_eq(EXPECTED_HASHES[expectedContents.length],
                  toHex(saver.sha256Hash));
      currentFile.remove(false);

      
      if (renamedFile.equals(currentFile)) {
        do_check_false(initialFile.exists());
      }
    } else if (!keepPartialOnFailure) {
      
      do_check_false(initialFile.exists());
      do_check_false(renamedFile.exists());
    } else {
      
      
      
      if (initialFile.exists()) {
        initialFile.remove(false);
      }
      if (renamedFile.exists()) {
        renamedFile.remove(false);
      }
    }

    do_print("Test case completed in " + (Date.now() - startTime) + " ms.");
  }
});

add_task(function test_setTarget_after_close_stream()
{
  
  
  let destFile = getTempFile(TEST_FILE_NAME_1);

  
  
  for (let i = 0; i < 2; i++) {
    let saver = new BackgroundFileSaverOutputStream();
    saver.enableSha256();
    let completionPromise = promiseSaverComplete(saver);
  
    
    
    
    
    yield promiseCopyToSaver(TEST_DATA_SHORT, saver, true);
  
    
    saver.setTarget(destFile, false);
    saver.finish(Cr.NS_OK);
    yield completionPromise;
  
    
    yield promiseVerifyContents(destFile, TEST_DATA_SHORT);
    do_check_eq(EXPECTED_HASHES[TEST_DATA_SHORT.length],
                toHex(saver.sha256Hash));
  }

  
  destFile.remove(false);
});

add_task(function test_setTarget_multiple()
{
  
  let destFile = getTempFile(TEST_FILE_NAME_1);
  let saver = new BackgroundFileSaverOutputStream();
  let completionPromise = promiseSaverComplete(saver);

  
  saver.setTarget(getTempFile(TEST_FILE_NAME_2), false);
  saver.setTarget(getTempFile(TEST_FILE_NAME_3), false);
  yield promiseCopyToSaver(TEST_DATA_SHORT, saver, true);
  saver.setTarget(getTempFile(TEST_FILE_NAME_2), false);
  saver.setTarget(destFile, false);

  
  saver.finish(Cr.NS_OK);
  yield completionPromise;

  
  do_check_false(getTempFile(TEST_FILE_NAME_2).exists());
  do_check_false(getTempFile(TEST_FILE_NAME_3).exists());
  yield promiseVerifyContents(destFile, TEST_DATA_SHORT);
  destFile.remove(false);
});

add_task(function test_finish_only()
{
  
  let destFile = getTempFile(TEST_FILE_NAME_1);
  let saver = new BackgroundFileSaverOutputStream();
  function onTargetChange(aTarget) {
    do_throw("Should not receive the onTargetChange notification.");
  }
  let completionPromise = promiseSaverComplete(saver, onTargetChange);
  saver.finish(Cr.NS_OK);
  yield completionPromise;
});

add_task(function test_invalid_hash()
{
  let saver = new BackgroundFileSaverStreamListener();
  
  try {
    let hash = saver.sha256Hash;
    throw "Shouldn't be able to get hash if hashing not enabled";
  } catch (ex if ex.result == Cr.NS_ERROR_NOT_AVAILABLE) { }
  
  saver.enableSha256();
  let destFile = getTempFile(TEST_FILE_NAME_1);
  saver.setTarget(destFile, false);
  
  
  
  
  saver.finish(Cr.NS_ERROR_FAILURE);
  try {
    let hash = saver.sha256Hash;
    throw "Shouldn't be able to get hash if save did not succeed";
  } catch (ex if ex.result == Cr.NS_ERROR_NOT_AVAILABLE) { }
});

add_task(function test_teardown()
{
  gStillRunning = false;
});
