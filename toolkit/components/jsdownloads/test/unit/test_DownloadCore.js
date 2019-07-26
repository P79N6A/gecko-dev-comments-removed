








"use strict";















function promiseSimpleDownload(aSourceURI) {
  return Downloads.createDownload({
    source: { uri: aSourceURI || TEST_SOURCE_URI },
    target: { file: getTempFile(TEST_TARGET_FILE_NAME) },
    saver: { type: "copy" },
  });
}







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

  
  yield download.start();

  yield promiseVerifyContents(targetFile, TEST_DATA_SHORT);
});




add_task(function test_download_initial_final_state()
{
  let download = yield promiseSimpleDownload();

  do_check_false(download.stopped);
  do_check_eq(download.progress, 0);

  
  yield download.start();

  do_check_true(download.stopped);
  do_check_eq(download.progress, 100);
});




add_task(function test_download_view_final_notified()
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
  let interruptible = startInterruptibleResponseHandler();

  let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

  download.onchange = function () {
    if (download.progress == 50) {
      do_check_true(download.hasProgress);
      do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);
      do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);

      
      interruptible.resolve();
    }
  };

  
  yield download.start();

  do_check_true(download.stopped);
  do_check_eq(download.progress, 100);

  yield promiseVerifyContents(download.target.file,
                              TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_cancel()
{
  let interruptible = startInterruptibleResponseHandler();
  try {
    let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

    
    download.onchange = function () {
      if (!download.stopped && download.progress == 50) {
        download.cancel();
      }
    };

    try {
      yield download.start();
      do_throw("The download should have been canceled.");
    } catch (ex if ex instanceof Downloads.Error) {
      do_check_false(ex.becauseSourceFailed);
      do_check_false(ex.becauseTargetFailed);
    }

    do_check_true(download.stopped);
    do_check_true(download.canceled);
    do_check_true(download.error === null);

    do_check_false(download.target.file.exists());
  } finally {
    interruptible.reject();
  }
});




add_task(function test_download_cancel_immediately()
{
  let interruptible = startInterruptibleResponseHandler();
  try {
    let download = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);

    let promiseStopped = download.start();
    download.cancel();

    try {
      yield promiseStopped;
      do_throw("The download should have been canceled.");
    } catch (ex if ex instanceof Downloads.Error) {
      do_check_false(ex.becauseSourceFailed);
      do_check_false(ex.becauseTargetFailed);
    }

    do_check_true(download.stopped);
    do_check_true(download.canceled);
    do_check_true(download.error === null);

    do_check_false(download.target.file.exists());
  } finally {
    interruptible.reject();
  }
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
    yield download.start();
    do_throw("The download should have failed.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseTargetFailed) {
    
  }

  do_check_true(download.stopped);
  do_check_false(download.canceled);
  do_check_true(download.error !== null);
  do_check_true(download.error.becauseTargetFailed);
  do_check_false(download.error.becauseSourceFailed);
});
