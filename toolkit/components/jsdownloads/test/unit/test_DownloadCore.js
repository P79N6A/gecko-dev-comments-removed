








"use strict";




function promiseSimpleDownload() {
  return Downloads.createDownload({
    source: { uri: TEST_SOURCE_URI },
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

  
  yield download.start();

  yield promiseVerifyContents(targetFile, TEST_DATA_SHORT);
});




add_task(function test_download_initial_final_state()
{
  let download = yield promiseSimpleDownload();

  do_check_false(download.done);
  do_check_eq(download.progress, 0);

  
  yield download.start();

  do_check_true(download.done);
  do_check_eq(download.progress, 100);
});




add_task(function test_download_view_final_notified()
{
  let download = yield promiseSimpleDownload();

  let onchangeNotified = false;
  let lastNotifiedDone;
  let lastNotifiedProgress;
  download.onchange = function () {
    onchangeNotified = true;
    lastNotifiedDone = download.done;
    lastNotifiedProgress = download.progress;
  };

  
  yield download.start();

  
  do_check_true(onchangeNotified);
  do_check_true(lastNotifiedDone);
  do_check_eq(lastNotifiedProgress, 100);
});




add_task(function test_download_intermediate_progress()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);

  let deferUntilHalfProgress = Promise.defer();
  gHttpServer.registerPathHandler("/test_download_intermediate_progress",
    function (aRequest, aResponse)
    {
      aResponse.processAsync();
      aResponse.setHeader("Content-Type", "text/plain", false);
      aResponse.setHeader("Content-Length", "" + (TEST_DATA_SHORT.length * 2),
                          false);
      aResponse.write(TEST_DATA_SHORT);

      deferUntilHalfProgress.promise.then(function () {
        aResponse.write(TEST_DATA_SHORT);
        aResponse.finish();
      });
    });

  let download = yield Downloads.createDownload({
    source: { uri: NetUtil.newURI(HTTP_BASE +
                                  "/test_download_intermediate_progress") },
    target: { file: targetFile },
    saver: { type: "copy" },
  });

  download.onchange = function () {
    if (download.progress == 50) {
      do_check_true(download.hasProgress);
      do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);
      do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);

      
      deferUntilHalfProgress.resolve();
    }
  };

  
  yield download.start();

  do_check_true(download.done);
  do_check_eq(download.progress, 100);

  yield promiseVerifyContents(targetFile, TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_download_error_source()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);

  let serverSocket = startFakeServer();
  try {
    let download = yield Downloads.createDownload({
      source: { uri: TEST_FAKE_SOURCE_URI },
      target: { file: targetFile },
      saver: { type: "copy" },
    });

    do_check_true(download.error === null);

    try {
      yield download.start();
      do_throw("The download should have failed.");
    } catch (ex if ex instanceof Downloads.Error && ex.becauseSourceFailed) {
      
    }

    do_check_true(download.done);
    do_check_true(download.error !== null);
    do_check_true(download.error.becauseSourceFailed);
    do_check_false(download.error.becauseTargetFailed);
  } finally {
    serverSocket.close();
  }
});




add_task(function test_download_error_target()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);

  
  targetFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0);

  let download = yield Downloads.createDownload({
    source: { uri: TEST_SOURCE_URI },
    target: { file: targetFile },
    saver: { type: "copy" },
  });

  do_check_true(download.error === null);

  try {
    yield download.start();
    do_throw("The download should have failed.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseTargetFailed) {
    
  }

  do_check_true(download.done);
  do_check_true(download.error !== null);
  do_check_true(download.error.becauseTargetFailed);
  do_check_false(download.error.becauseSourceFailed);
});
