








"use strict";







add_task(function test_download_construction()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);

  let download = yield Downloads.createDownload({
    source: { uri: TEST_SOURCE_URI },
    target: { file: targetFile },
    saver: { type: "copy" },
  });

  
  do_check_true(download.source.uri.equals(TEST_SOURCE_URI));
  do_check_eq(download.target.file, targetFile);
  do_check_true(download.source.referrer === null);

  
  yield download.start();

  yield promiseVerifyContents(targetFile, TEST_DATA_SHORT);
});




add_task(function test_download_referrer()
{
  let source_path = "/test_download_referrer.txt";
  let source_uri = NetUtil.newURI(HTTP_BASE + source_path);
  let target_uri = getTempFile(TEST_TARGET_FILE_NAME);

  function cleanup() {
    gHttpServer.registerPathHandler(source_path, null);
  }

  do_register_cleanup(cleanup);

  gHttpServer.registerPathHandler(source_path, function (aRequest, aResponse) {
    aResponse.setHeader("Content-Type", "text/plain", false);

    do_check_true(aRequest.hasHeader("Referer"));
    do_check_eq(aRequest.getHeader("Referer"), TEST_REFERRER_URI.spec);
  });
  let download = yield Downloads.createDownload({
    source: { uri: source_uri, referrer: TEST_REFERRER_URI },
    target: { file: target_uri },
    saver: { type: "copy" },
  });
  do_check_true(download.source.referrer.equals(TEST_REFERRER_URI));
  yield download.start();

  download = yield Downloads.createDownload({
    source: { uri: source_uri, referrer: TEST_REFERRER_URI, isPrivate: true },
    target: { file: target_uri },
    saver: { type: "copy" },
  });
  do_check_true(download.source.referrer.equals(TEST_REFERRER_URI));
  yield download.start();

  
  source_uri = NetUtil.newURI("data:text/html,<html><body></body></html>");
  download = yield Downloads.createDownload({
    source: { uri: source_uri, referrer: TEST_REFERRER_URI },
    target: { file: target_uri },
    saver: { type: "copy" },
  });
  do_check_true(download.source.referrer.equals(TEST_REFERRER_URI));
  yield download.start();

  cleanup();
});




add_task(function test_download_initial_final_state()
{
  let download = yield promiseSimpleDownload();

  do_check_true(download.stopped);
  do_check_false(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);
  do_check_eq(download.progress, 0);
  do_check_true(download.startTime === null);

  
  yield download.start();

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);
  do_check_eq(download.progress, 100);
  do_check_true(isValidDate(download.startTime));
});




add_task(function test_download_final_state_notified()
{
  let download = yield promiseSimpleDownload();

  let onchangeNotified = false;
  let lastNotifiedStopped;
  let lastNotifiedProgress;
  download.onchange = function () {
    onchangeNotified = true;
    lastNotifiedStopped = download.stopped;
    lastNotifiedProgress = download.progress;
  };

  
  yield download.start();

  
  do_check_true(onchangeNotified);
  do_check_true(lastNotifiedStopped);
  do_check_eq(lastNotifiedProgress, 100);
});




add_task(function test_download_intermediate_progress()
{
  let deferResponse = deferNextResponse();

  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  download.onchange = function () {
    if (download.progress == 50) {
      do_check_true(download.hasProgress);
      do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);
      do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);

      
      deferResponse.resolve();
    }
  };

  
  yield download.start();

  do_check_true(download.stopped);
  do_check_eq(download.progress, 100);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_empty_progress()
{
  let download = yield promiseSimpleDownload(TEST_EMPTY_URI);

  yield download.start();

  do_check_true(download.stopped);
  do_check_true(download.hasProgress);
  do_check_eq(download.progress, 100);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  do_check_eq(download.target.file.fileSize, 0);
});




add_task(function test_download_empty_noprogress()
{
  let deferResponse = deferNextResponse();
  let promiseEmptyRequestReceived = promiseNextRequestReceived();

  let download = yield promiseSimpleDownload(TEST_EMPTY_NOPROGRESS_URI);

  download.onchange = function () {
    if (!download.stopped) {
      do_check_false(download.hasProgress);
      do_check_eq(download.currentBytes, 0);
      do_check_eq(download.totalBytes, 0);
    }
  };

  
  let promiseAttempt = download.start();

  
  
  
  yield promiseEmptyRequestReceived;
  yield promiseExecuteSoon();

  
  do_check_false(download.stopped);
  do_check_false(download.hasProgress);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  
  deferResponse.resolve();
  yield promiseAttempt;

  
  do_check_true(download.stopped);
  do_check_false(download.hasProgress);
  do_check_eq(download.progress, 100);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  do_check_eq(download.target.file.fileSize, 0);
});




add_task(function test_download_start_twice()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  
  let deferResponse = deferNextResponse();

  
  let promiseAttempt1 = download.start();
  let promiseAttempt2 = download.start();

  
  deferResponse.resolve();

  
  yield promiseAttempt1;
  yield promiseAttempt2;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_cancel_midway()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  let deferResponse = deferNextResponse();
  try {
    
    let deferCancel = Promise.defer();
    download.onchange = function () {
      if (!download.stopped && !download.canceled && download.progress == 50) {
        deferCancel.resolve(download.cancel());

        
        
        do_check_true(download.canceled);
      }
    };

    let promiseAttempt = download.start();

    
    
    yield deferCancel.promise;

    do_check_true(download.stopped);
    do_check_true(download.canceled);
    do_check_true(download.error === null);

    do_check_false(download.target.file.exists());

    
    do_check_eq(download.progress, 50);
    do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);
    do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);

    
    try {
      yield promiseAttempt;
      do_throw("The download should have been canceled.");
    } catch (ex if ex instanceof Downloads.Error) {
      do_check_false(ex.becauseSourceFailed);
      do_check_false(ex.becauseTargetFailed);
    }
  } finally {
    deferResponse.resolve();
  }
});




add_task(function test_download_cancel_immediately()
{
  
  let deferResponse = deferNextResponse();
  try {
    let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

    let promiseAttempt = download.start();
    do_check_false(download.stopped);

    let promiseCancel = download.cancel();
    do_check_true(download.canceled);

    
    
    
    try {
      yield promiseAttempt;
      do_throw("The download should have been canceled.");
    } catch (ex if ex instanceof Downloads.Error) {
      do_check_false(ex.becauseSourceFailed);
      do_check_false(ex.becauseTargetFailed);
    }

    do_check_true(download.stopped);
    do_check_true(download.canceled);
    do_check_true(download.error === null);

    do_check_false(download.target.file.exists());

    
    yield promiseCancel;
  } finally {
    deferResponse.resolve();
  }

  
  
  
  
  for (let i = 0; i < 5; i++) {
    yield promiseExecuteSoon();
  }
});




add_task(function test_download_cancel_midway_restart()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  
  let deferResponse = deferNextResponse();
  try {
    let deferCancel = Promise.defer();
    download.onchange = function () {
      if (!download.stopped && !download.canceled && download.progress == 50) {
        deferCancel.resolve(download.cancel());
      }
    };
    download.start();
    yield deferCancel.promise;
  } finally {
    deferResponse.resolve();
  }

  do_check_true(download.stopped);

  
  download.onchange = null;
  let promiseAttempt = download.start();

  
  do_check_false(download.stopped);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  
  
  
  do_check_eq(download.progress, 0);
  do_check_eq(download.totalBytes, 0);
  do_check_eq(download.currentBytes, 0);

  yield promiseAttempt;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_cancel_immediately_restart_immediately()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  
  let deferResponse = deferNextResponse();

  let promiseAttempt = download.start();
  do_check_false(download.stopped);

  download.cancel();
  do_check_true(download.canceled);

  let promiseRestarted = download.start();
  do_check_false(download.stopped);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  
  
  
  do_check_eq(download.hasProgress, false);
  do_check_eq(download.progress, 0);
  do_check_eq(download.totalBytes, 0);
  do_check_eq(download.currentBytes, 0);

  
  
  
  
  for (let i = 0; i < 5; i++) {
    yield promiseExecuteSoon();
  }

  
  
  deferResponse.resolve();

  try {
    yield promiseAttempt;
    do_throw("The download should have been canceled.");
  } catch (ex if ex instanceof Downloads.Error) {
    do_check_false(ex.becauseSourceFailed);
    do_check_false(ex.becauseTargetFailed);
  }

  yield promiseRestarted;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_cancel_midway_restart_immediately()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  
  let deferResponse = deferNextResponse();

  let deferMidway = Promise.defer();
  download.onchange = function () {
    if (!download.stopped && !download.canceled && download.progress == 50) {
      do_check_eq(download.progress, 50);
      deferMidway.resolve();
    }
  };
  let promiseAttempt = download.start();
  yield deferMidway.promise;

  download.cancel();
  do_check_true(download.canceled);

  let promiseRestarted = download.start();
  do_check_false(download.stopped);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  
  
  
  do_check_eq(download.hasProgress, false);
  do_check_eq(download.progress, 0);
  do_check_eq(download.totalBytes, 0);
  do_check_eq(download.currentBytes, 0);

  deferResponse.resolve();

  
  try {
    yield promiseAttempt;
    do_throw("The download should have been canceled.");
  } catch (ex if ex instanceof Downloads.Error) {
    do_check_false(ex.becauseSourceFailed);
    do_check_false(ex.becauseTargetFailed);
  }

  yield promiseRestarted;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_cancel_successful()
{
  let download = yield promiseSimpleDownload();

  
  yield download.start();

  
  yield download.cancel();

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyContents(download.target.file, TEST_DATA_SHORT);
});




add_task(function test_download_cancel_twice()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  
  let deferResponse = deferNextResponse();
  try {
    let promiseAttempt = download.start();
    do_check_false(download.stopped);

    let promiseCancel1 = download.cancel();
    do_check_true(download.canceled);
    let promiseCancel2 = download.cancel();

    try {
      yield promiseAttempt;
      do_throw("The download should have been canceled.");
    } catch (ex if ex instanceof Downloads.Error) {
      do_check_false(ex.becauseSourceFailed);
      do_check_false(ex.becauseTargetFailed);
    }

    
    yield promiseCancel1;
    yield promiseCancel2;

    do_check_true(download.stopped);
    do_check_false(download.succeeded);
    do_check_true(download.canceled);
    do_check_true(download.error === null);

    do_check_false(download.target.file.exists());
  } finally {
    deferResponse.resolve();
  }
});




add_task(function test_download_whenSucceeded()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  
  let deferResponse = deferNextResponse();

  
  let promiseSucceeded = download.whenSucceeded();

  
  download.start();
  yield download.cancel();

  deferResponse.resolve();

  
  download.start();

  
  yield promiseSucceeded;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_error_source()
{
  let serverSocket = startFakeServer();
  try {
    let download = yield promiseSimpleDownload(TEST_FAKE_SOURCE_URI);

    do_check_true(download.error === null);

    try {
      yield download.start();
      do_throw("The download should have failed.");
    } catch (ex if ex instanceof Downloads.Error && ex.becauseSourceFailed) {
      
    }

    do_check_true(download.stopped);
    do_check_false(download.canceled);
    do_check_true(download.error !== null);
    do_check_true(download.error.becauseSourceFailed);
    do_check_false(download.error.becauseTargetFailed);
  } finally {
    serverSocket.close();
  }
});




add_task(function test_download_error_target()
{
  let download = yield promiseSimpleDownload();

  do_check_true(download.error === null);

  
  download.target.file.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0);
  try {
    try {
      yield download.start();
      do_throw("The download should have failed.");
    } catch (ex if ex instanceof Downloads.Error && ex.becauseTargetFailed) {
      
    }

    do_check_true(download.stopped);
    do_check_false(download.canceled);
    do_check_true(download.error !== null);
    do_check_true(download.error.becauseTargetFailed);
    do_check_false(download.error.becauseSourceFailed);
  } finally {
    
    if (download.target.file.exists()) {
      download.target.file.permissions = FileUtils.PERMS_FILE;
      download.target.file.remove(false);
    }
  }
});




add_task(function test_download_error_restart()
{
  let download = yield promiseSimpleDownload();

  do_check_true(download.error === null);

  
  download.target.file.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0);

  try {
    yield download.start();
    do_throw("The download should have failed.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseTargetFailed) {
    
  } finally {
    
    if (download.target.file.exists()) {
      download.target.file.permissions = FileUtils.PERMS_FILE;

      
      
      
      let fileToRemove = download.target.file.clone();
      fileToRemove.moveTo(null, fileToRemove.leafName + ".delete.tmp");
      fileToRemove.remove(false);
    }
  }

  
  yield download.start();

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);
  do_check_eq(download.progress, 100);

  yield promiseVerifyContents(download.target.file, TEST_DATA_SHORT);
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
  let download = yield Downloads.createDownload({
    source: { uri: source_uri, isPrivate: true },
    target: { file: targetFile },
    saver: { type: "copy" },
  });
  yield download.start();

  cleanup();
});




add_task(function test_download_cancel_immediately_restart_and_check_startTime()
{
  let download = yield promiseSimpleDownload();

  download.start();
  let startTime = download.startTime;
  do_check_true(isValidDate(download.startTime));

  yield download.cancel();
  do_check_eq(download.startTime.getTime(), startTime.getTime());

  
  yield promiseTimeout(10);

  yield download.start();
  do_check_true(download.startTime.getTime() > startTime.getTime());
});




add_task(function test_download_with_content_encoding()
{
  let source_path = "/test_download_with_content_encoding.txt";
  let source_uri = NetUtil.newURI(HTTP_BASE + source_path);

  function cleanup() {
    gHttpServer.registerPathHandler(source_path, null);
  }
  do_register_cleanup(cleanup);

  gHttpServer.registerPathHandler(source_path, function (aRequest, aResponse) {
    aResponse.setHeader("Content-Type", "text/plain", false);
    aResponse.setHeader("Content-Encoding", "gzip", false);
    aResponse.setHeader("Content-Length",
                        "" + TEST_DATA_SHORT_GZIP_ENCODED.length, false);

    let bos =  new BinaryOutputStream(aResponse.bodyOutputStream);
    bos.writeByteArray(TEST_DATA_SHORT_GZIP_ENCODED,
                       TEST_DATA_SHORT_GZIP_ENCODED.length);
  });

  let download = yield Downloads.createDownload({
    source: { uri: source_uri },
    target: { file: getTempFile(TEST_TARGET_FILE_NAME) },
    saver: { type: "copy" },
  });
  yield download.start();

  do_check_eq(download.progress, 100);
  do_check_eq(download.totalBytes, TEST_DATA_SHORT_GZIP_ENCODED.length);

  
  yield promiseVerifyContents(download.target.file, TEST_DATA_SHORT);
});




add_task(function test_download_cancel_midway_restart_with_content_encoding()
{
  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_GZIP_URI);

  
  let deferResponse = deferNextResponse();
  try {
    let deferCancel = Promise.defer();
    download.onchange = function () {
      if (!download.stopped && !download.canceled &&
          download.currentBytes == TEST_DATA_SHORT_GZIP_ENCODED_FIRST.length) {
        deferCancel.resolve(download.cancel());
      }
    };
    download.start();
    yield deferCancel.promise;
  } finally {
    deferResponse.resolve();
  }

  do_check_true(download.stopped);

  
  download.onchange = null;
  yield download.start()

  do_check_eq(download.progress, 100);
  do_check_eq(download.totalBytes, TEST_DATA_SHORT_GZIP_ENCODED.length);

  yield promiseVerifyContents(download.target.file, TEST_DATA_SHORT);
});




add_task(function test_download_blocked_parental_controls()
{
  function cleanup() {
    DownloadIntegration.shouldBlockInTest = false;
  }
  do_register_cleanup(cleanup);
  DownloadIntegration.shouldBlockInTest = true;

  let download = yield promiseSimpleDownload();

  try {
    yield download.start();
    do_throw("The download should have blocked.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseBlocked) {
    do_check_true(ex.becauseBlockedByParentalControls);
  }
  cleanup();
});

