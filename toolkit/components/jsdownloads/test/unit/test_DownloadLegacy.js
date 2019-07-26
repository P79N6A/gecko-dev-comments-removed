








"use strict";






















function promiseStartLegacyDownload(aSourceURI, aIsPrivate, aOutPersist) {
  let sourceURI = aSourceURI || TEST_SOURCE_URI;
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);

  let persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                  .createInstance(Ci.nsIWebBrowserPersist);

  
  
  
  
  let transfer =
      Components.classesByID["{1b4c85df-cbdd-4bb6-b04e-613caece083c}"]
                .createInstance(Ci.nsITransfer);

  if (aOutPersist) {
    aOutPersist.value = persist;
  }

  let deferred = Promise.defer();
  let promise = aIsPrivate ? Downloads.getPrivateDownloadList() :
                Downloads.getPublicDownloadList();
  promise.then(function (aList) {
    
    
    aList.addView({
      onDownloadAdded: function (aDownload) {
        aList.removeView(this);

        
        
        aList.remove(aDownload);

        
        deferred.resolve(aDownload);
      },
    });

    
    
    transfer.init(sourceURI, NetUtil.newURI(targetFile), null, null, null, null,
                  persist, aIsPrivate);
    persist.progressListener = transfer;

    
    persist.savePrivacyAwareURI(sourceURI, null, null, null, null, targetFile, aIsPrivate);
  }.bind(this)).then(null, do_report_unexpected_exception);

  return deferred.promise;
}







add_task(function test_basic()
{
  let tempDirectory = FileUtils.getDir("TmpD", []);

  let download = yield promiseStartLegacyDownload();

  
  do_check_true(download.source.uri.equals(TEST_SOURCE_URI));
  do_check_true(download.target.file.parent.equals(tempDirectory));

  
  if (!download.stopped) {
    yield download.start();
  }

  yield promiseVerifyContents(download.target.file, TEST_DATA_SHORT);
});




add_task(function test_final_state()
{
  let download = yield promiseStartLegacyDownload();

  
  if (!download.stopped) {
    yield download.start();
  }

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);
  do_check_eq(download.progress, 100);
});




add_task(function test_intermediate_progress()
{
  let deferResponse = deferNextResponse();

  let download = yield promiseStartLegacyDownload(TEST_INTERRUPTIBLE_URI);

  let onchange = function () {
    if (download.progress == 50) {
      do_check_true(download.hasProgress);
      do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);
      do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);

      
      deferResponse.resolve();
    }
  };

  
  
  download.onchange = onchange;
  onchange();

  
  if (!download.stopped) {
    yield download.start();
  }

  do_check_true(download.stopped);
  do_check_eq(download.progress, 100);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_empty_progress()
{
  let download = yield promiseStartLegacyDownload(TEST_EMPTY_URI);

  
  if (!download.stopped) {
    yield download.start();
  }

  do_check_true(download.stopped);
  do_check_true(download.hasProgress);
  do_check_eq(download.progress, 100);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  do_check_eq(download.target.file.fileSize, 0);
});




add_task(function test_empty_noprogress()
{
  let deferResponse = deferNextResponse();
  let promiseEmptyRequestReceived = promiseNextRequestReceived();

  let download = yield promiseStartLegacyDownload(TEST_EMPTY_NOPROGRESS_URI);

  
  
  
  yield promiseEmptyRequestReceived;
  yield promiseExecuteSoon();

  
  do_check_false(download.stopped);
  do_check_false(download.hasProgress);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  
  
  deferResponse.resolve();
  if (!download.stopped) {
    yield download.start();
  }

  
  do_check_true(download.stopped);
  do_check_false(download.hasProgress);
  do_check_eq(download.progress, 100);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  do_check_eq(download.target.file.fileSize, 0);
});




add_task(function test_cancel_midway()
{
  let deferResponse = deferNextResponse();
  let outPersist = {};
  let download = yield promiseStartLegacyDownload(TEST_INTERRUPTIBLE_URI, false,
                                                  outPersist);

  try {
    
    let deferCancel = Promise.defer();
    let onchange = function () {
      if (!download.stopped && !download.canceled && download.progress == 50) {
        deferCancel.resolve(download.cancel());

        
        
        do_check_true(download.canceled);
      }
    };

    
    
    download.onchange = onchange;
    onchange();

    
    
    yield deferCancel.promise;

    
    do_check_eq(outPersist.value.result, Cr.NS_ERROR_ABORT);

    do_check_true(download.stopped);
    do_check_true(download.canceled);
    do_check_true(download.error === null);

    do_check_false(download.target.file.exists());

    
    do_check_eq(download.progress, 50);
    do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);
    do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);
  } finally {
    deferResponse.resolve();
  }
});




add_task(function test_error()
{
  let serverSocket = startFakeServer();
  try {
    let download = yield promiseStartLegacyDownload(TEST_FAKE_SOURCE_URI);

    
    
    let deferStopped = Promise.defer();
    let onchange = function () {
      if (download.stopped) {
        deferStopped.resolve();
      }
    };
    download.onchange = onchange;
    onchange();
    yield deferStopped.promise;

    
    do_check_false(download.canceled);
    do_check_true(download.error !== null);
    do_check_true(download.error.becauseSourceFailed);
    do_check_false(download.error.becauseTargetFailed);
  } finally {
    serverSocket.close();
  }
});




add_task(function test_download_public_and_private()
{
  let source_path = "/test_download_public_and_private.txt";
  let source_uri = NetUtil.newURI(HTTP_BASE + source_path);
  let testCount = 0;

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  function cleanup() {
    Services.prefs.clearUserPref("network.cookie.cookieBehavior");
    Services.cookies.removeAll();
    gHttpServer.registerPathHandler(source_path, null);
  }

  do_register_cleanup(cleanup);

  gHttpServer.registerPathHandler(source_path, function (aRequest, aResponse) {
    aResponse.setHeader("Content-Type", "text/plain", false);

    if (testCount == 0) {
      
      do_check_false(aRequest.hasHeader("Cookie"));
      aResponse.setHeader("Set-Cookie", "foobar=1", false);
      testCount++;
    } else if (testCount == 1) {
      
      do_check_true(aRequest.hasHeader("Cookie"));
      do_check_eq(aRequest.getHeader("Cookie"), "foobar=1");
      testCount++;
    } else if (testCount == 2)  {
      
      do_check_false(aRequest.hasHeader("Cookie"));
    }
  });

  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  yield Downloads.simpleDownload(source_uri, targetFile);
  yield Downloads.simpleDownload(source_uri, targetFile);
  let download = yield promiseStartLegacyDownload(source_uri, true);
  
  if (!download.stopped) {
    yield download.start();
  }

  cleanup();
});




add_task(function test_download_blocked_parental_controls()
{
  function cleanup() {
    DownloadIntegration.shouldBlockInTest = false;
  }
  do_register_cleanup(cleanup);
  DownloadIntegration.shouldBlockInTest = true;

  let download = yield promiseStartLegacyDownload();

  try {
    yield download.start();
    do_throw("The download should have blocked.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseBlocked) {
    do_check_true(ex.becauseBlockedByParentalControls);
  }

  do_check_false(download.target.file.exists());

  cleanup();
});
