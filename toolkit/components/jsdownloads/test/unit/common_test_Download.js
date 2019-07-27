










"use strict";




const kDeleteTempFileOnExit = "browser.helperApps.deleteTempFileOnExit";











function promiseStartDownload(aSourceUrl) {
  if (gUseLegacySaver) {
    return promiseStartLegacyDownload(aSourceUrl);
  }

  return promiseNewDownload(aSourceUrl).then(download => {
    download.start();
    return download;
  });
}














function promiseStartDownload_tryToKeepPartialData() {
  return Task.spawn(function () {
    mustInterruptResponses();

    
    let download;
    if (!gUseLegacySaver) {
      let targetFilePath = getTempFile(TEST_TARGET_FILE_NAME).path;
      download = yield Downloads.createDownload({
        source: httpUrl("interruptible_resumable.txt"),
        target: { path: targetFilePath,
                  partFilePath: targetFilePath + ".part" },
      });
      download.tryToKeepPartialData = true;
      download.start();
    } else {
      
      
      download = yield promiseStartExternalHelperAppServiceDownload();
    }

    yield promiseDownloadMidway(download);
    yield promisePartFileReady(download);

    throw new Task.Result(download);
  });
}










function promisePartFileReady(aDownload) {
  return Task.spawn(function () {
    
    
    
    
    try {
      do {
        yield promiseTimeout(50);
      } while (!(yield OS.File.exists(aDownload.target.partFilePath)));
    } catch (ex if ex instanceof OS.File.Error) {
      
      
      do_print("Expected exception while checking existence: " + ex.toString());
      
      yield promiseTimeout(100);
    }
  });
}














let promiseVerifyTarget = Task.async(function* (downloadTarget,
                                                expectedContents) {
  yield promiseVerifyContents(downloadTarget.path, expectedContents);
  do_check_true(downloadTarget.exists);
  do_check_eq(downloadTarget.size, expectedContents.length);
});









add_task(function test_basic()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);

  let download;
  if (!gUseLegacySaver) {
    
    
    download = yield Downloads.createDownload({
      source: { url: httpUrl("source.txt") },
      target: { path: targetFile.path },
      saver: { type: "copy" },
    });

    do_check_eq(download.source.url, httpUrl("source.txt"));
    do_check_eq(download.target.path, targetFile.path);

    yield download.start();
  } else {
    
    
    download = yield promiseStartLegacyDownload(null,
                                                { targetFile: targetFile });

    do_check_eq(download.source.url, httpUrl("source.txt"));
    do_check_eq(download.target.path, targetFile.path);

    yield promiseDownloadStopped(download);
  }

  
  do_check_true(download.source.referrer === null);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);
});






add_task(function test_basic_tryToKeepPartialData()
{
  let download = yield promiseStartDownload_tryToKeepPartialData();
  continueResponses();
  yield promiseDownloadStopped(download);

  
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
  do_check_eq(32, download.saver.getSha256Hash().length);
});




add_task(function test_unix_permissions()
{
  
  if (Services.appinfo.OS != "Darwin" && Services.appinfo.OS != "Linux" &&
      Services.appinfo.OS != "WINNT") {
    do_print("Skipping test.");
    return;
  }

  let launcherPath = getTempFile("app-launcher").path;

  for (let autoDelete of [false, true]) {
    for (let isPrivate of [false, true]) {
      for (let launchWhenSucceeded of [false, true]) {
        do_print("Checking " + JSON.stringify({ autoDelete,
                                                isPrivate,
                                                launchWhenSucceeded }));

        Services.prefs.setBoolPref(kDeleteTempFileOnExit, autoDelete);

        let download;
        if (!gUseLegacySaver) {
          download = yield Downloads.createDownload({
            source: { url: httpUrl("source.txt"), isPrivate },
            target: getTempFile(TEST_TARGET_FILE_NAME).path,
            launchWhenSucceeded,
            launcherPath,
          });
          yield download.start();
        } else {
          download = yield promiseStartLegacyDownload(httpUrl("source.txt"), {
            isPrivate,
            launchWhenSucceeded,
            launcherPath: launchWhenSucceeded && launcherPath,
          });
          yield promiseDownloadStopped(download);
        }

        let isTemporary = launchWhenSucceeded && (autoDelete || isPrivate);
        let stat = yield OS.File.stat(download.target.path);
        if (Services.appinfo.OS == "WINNT") {
          
          
          do_check_eq(stat.winAttributes.readOnly, isTemporary ? true : false);
        } else {
          
          
          
          
          do_check_eq(stat.unixMode,
                      isTemporary ? 0o400 : (0o666 & ~OS.Constants.Sys.umask));
        }
      }
    }
  }

  
  Services.prefs.clearUserPref(kDeleteTempFileOnExit);
});




add_task(function test_referrer()
{
  let sourcePath = "/test_referrer.txt";
  let sourceUrl = httpUrl("test_referrer.txt");
  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;

  function cleanup() {
    gHttpServer.registerPathHandler(sourcePath, null);
  }
  do_register_cleanup(cleanup);

  gHttpServer.registerPathHandler(sourcePath, function (aRequest, aResponse) {
    aResponse.setHeader("Content-Type", "text/plain", false);

    do_check_true(aRequest.hasHeader("Referer"));
    do_check_eq(aRequest.getHeader("Referer"), TEST_REFERRER_URL);
  });
  let download = yield Downloads.createDownload({
    source: { url: sourceUrl, referrer: TEST_REFERRER_URL },
    target: targetPath,
  });
  do_check_eq(download.source.referrer, TEST_REFERRER_URL);
  yield download.start();

  download = yield Downloads.createDownload({
    source: { url: sourceUrl, referrer: TEST_REFERRER_URL,
              isPrivate: true },
    target: targetPath,
  });
  do_check_eq(download.source.referrer, TEST_REFERRER_URL);
  yield download.start();

  
  sourceUrl = "data:text/html,<html><body></body></html>";
  download = yield Downloads.createDownload({
    source: { url: sourceUrl, referrer: TEST_REFERRER_URL },
    target: targetPath,
  });
  do_check_eq(download.source.referrer, TEST_REFERRER_URL);
  yield download.start();

  cleanup();
});




add_task(function test_initial_final_state()
{
  let download;
  if (!gUseLegacySaver) {
    
    
    download = yield promiseNewDownload();

    do_check_true(download.stopped);
    do_check_false(download.succeeded);
    do_check_false(download.canceled);
    do_check_true(download.error === null);
    do_check_eq(download.progress, 0);
    do_check_true(download.startTime === null);
    do_check_false(download.target.exists);
    do_check_eq(download.target.size, 0);

    yield download.start();
  } else {
    
    
    download = yield promiseStartLegacyDownload();
    yield promiseDownloadStopped(download);
  }

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);
  do_check_eq(download.progress, 100);
  do_check_true(isValidDate(download.startTime));
  do_check_true(download.target.exists);
  do_check_eq(download.target.size, TEST_DATA_SHORT.length);
});




add_task(function test_final_state_notified()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));

  let onchangeNotified = false;
  let lastNotifiedStopped;
  let lastNotifiedProgress;
  download.onchange = function () {
    onchangeNotified = true;
    lastNotifiedStopped = download.stopped;
    lastNotifiedProgress = download.progress;
  };

  
  let promiseAttempt = download.start();
  continueResponses();
  yield promiseAttempt;

  
  do_check_true(onchangeNotified);
  do_check_true(lastNotifiedStopped);
  do_check_eq(lastNotifiedProgress, 100);
});




add_task(function test_intermediate_progress()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));

  yield promiseDownloadMidway(download);

  do_check_true(download.hasProgress);
  do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);
  do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);

  
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);

  
  continueResponses();
  yield promiseDownloadStopped(download);

  do_check_true(download.stopped);
  do_check_eq(download.progress, 100);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_empty_progress()
{
  let download = yield promiseStartDownload(httpUrl("empty.txt"));
  yield promiseDownloadStopped(download);

  do_check_true(download.stopped);
  do_check_true(download.hasProgress);
  do_check_eq(download.progress, 100);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  
  do_check_eq(download.contentType, "text/plain");

  do_check_eq((yield OS.File.stat(download.target.path)).size, 0);
  do_check_true(download.target.exists);
  do_check_eq(download.target.size, 0);
});





add_task(function test_empty_progress_tryToKeepPartialData()
{
  
  let download;
  if (!gUseLegacySaver) {
    let targetFilePath = getTempFile(TEST_TARGET_FILE_NAME).path;
    download = yield Downloads.createDownload({
      source: httpUrl("empty.txt"),
      target: { path: targetFilePath,
                partFilePath: targetFilePath + ".part" },
    });
    download.tryToKeepPartialData = true;
    download.start();
  } else {
    
    
    download = yield promiseStartExternalHelperAppServiceDownload(
                                                         httpUrl("empty.txt"));
  }
  yield promiseDownloadStopped(download);

  
  do_check_eq((yield OS.File.stat(download.target.path)).size, 0);
  do_check_true(download.target.exists);
  do_check_eq(download.target.size, 0);

  do_check_false(yield OS.File.exists(download.target.partFilePath));
  do_check_eq(32, download.saver.getSha256Hash().length);
});




add_task(function test_empty_noprogress()
{
  let sourcePath = "/test_empty_noprogress.txt";
  let sourceUrl = httpUrl("test_empty_noprogress.txt");
  let deferRequestReceived = Promise.defer();

  
  function cleanup() {
    gHttpServer.registerPathHandler(sourcePath, null);
  }
  do_register_cleanup(cleanup);

  registerInterruptibleHandler(sourcePath,
    function firstPart(aRequest, aResponse) {
      aResponse.setHeader("Content-Type", "text/plain", false);
      deferRequestReceived.resolve();
    }, function secondPart(aRequest, aResponse) { });

  
  mustInterruptResponses();
  let download;
  if (!gUseLegacySaver) {
    
    
    
    download = yield promiseNewDownload(sourceUrl);

    download.onchange = function () {
      if (!download.stopped) {
        do_check_false(download.hasProgress);
        do_check_eq(download.currentBytes, 0);
        do_check_eq(download.totalBytes, 0);
      }
    };

    download.start();
  } else {
    
    
    
    download = yield promiseStartLegacyDownload(sourceUrl);
  }

  
  
  
  yield deferRequestReceived.promise;
  yield promiseExecuteSoon();

  
  do_check_false(download.stopped);
  do_check_false(download.hasProgress);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);

  
  continueResponses();
  yield promiseDownloadStopped(download);

  
  do_check_eq(download.contentType, "text/plain");

  
  do_check_true(download.stopped);
  do_check_false(download.hasProgress);
  do_check_eq(download.progress, 100);
  do_check_eq(download.currentBytes, 0);
  do_check_eq(download.totalBytes, 0);
  do_check_true(download.target.exists);
  do_check_eq(download.target.size, 0);

  do_check_eq((yield OS.File.stat(download.target.path)).size, 0);
});




add_task(function test_start_twice()
{
  mustInterruptResponses();

  let download;
  if (!gUseLegacySaver) {
    
    
    download = yield promiseNewDownload(httpUrl("interruptible.txt"));
  } else {
    
    
    download = yield promiseStartLegacyDownload(httpUrl("interruptible.txt"));
  }

  
  let promiseAttempt1 = download.start();
  let promiseAttempt2 = download.start();

  
  continueResponses();

  
  yield promiseAttempt1;
  yield promiseAttempt2;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_cancel_midway()
{
  mustInterruptResponses();

  
  
  let download;
  let options = {};
  if (!gUseLegacySaver) {
    download = yield promiseNewDownload(httpUrl("interruptible.txt"));
  } else {
    download = yield promiseStartLegacyDownload(httpUrl("interruptible.txt"),
                                                options);
  }

  
  let deferCancel = Promise.defer();
  let onchange = function () {
    if (!download.stopped && !download.canceled && download.progress == 50) {
      
      deferCancel.resolve(download.cancel());

      
      
      do_check_true(download.canceled);
    }
  };

  
  
  
  download.onchange = onchange;
  onchange();

  let promiseAttempt;
  if (!gUseLegacySaver) {
    promiseAttempt = download.start();
  }

  
  
  yield deferCancel.promise;

  if (gUseLegacySaver) {
    
    do_check_eq(options.outPersist.result, Cr.NS_ERROR_ABORT);
  }

  do_check_true(download.stopped);
  do_check_true(download.canceled);
  do_check_true(download.error === null);
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);

  do_check_false(yield OS.File.exists(download.target.path));

  
  do_check_eq(download.progress, 50);
  do_check_eq(download.totalBytes, TEST_DATA_SHORT.length * 2);
  do_check_eq(download.currentBytes, TEST_DATA_SHORT.length);

  if (!gUseLegacySaver) {
    
    try {
      yield promiseAttempt;
      do_throw("The download should have been canceled.");
    } catch (ex if ex instanceof Downloads.Error) {
      do_check_false(ex.becauseSourceFailed);
      do_check_false(ex.becauseTargetFailed);
    }
  }
});





add_task(function test_cancel_midway_tryToKeepPartialData()
{
  let download = yield promiseStartDownload_tryToKeepPartialData();

  do_check_true(yield OS.File.exists(download.target.path));
  do_check_true(yield OS.File.exists(download.target.partFilePath));

  yield download.cancel();
  yield download.removePartialData();

  do_check_true(download.stopped);
  do_check_true(download.canceled);
  do_check_true(download.error === null);

  do_check_false(yield OS.File.exists(download.target.path));
  do_check_false(yield OS.File.exists(download.target.partFilePath));
});




add_task(function test_cancel_immediately()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));

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

  do_check_false(yield OS.File.exists(download.target.path));

  
  yield promiseCancel;
});




add_task(function test_cancel_midway_restart()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));

  
  yield promiseDownloadMidway(download);
  yield download.cancel();

  do_check_true(download.stopped);

  
  continueResponses();
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

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_cancel_midway_restart_tryToKeepPartialData()
{
  let download = yield promiseStartDownload_tryToKeepPartialData();
  yield download.cancel();

  do_check_true(download.stopped);
  do_check_true(download.hasPartialData);

  
  do_check_false(yield OS.File.exists(download.target.path));
  yield promiseVerifyContents(download.target.partFilePath, TEST_DATA_SHORT);
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);

  
  do_check_eq(gMostRecentFirstBytePos, 0);

  
  
  let deferMidway = Promise.defer();
  download.onchange = function () {
    if (!download.stopped && !download.canceled &&
        download.currentBytes == Math.floor(TEST_DATA_SHORT.length * 3 / 2)) {
      download.onchange = null;
      deferMidway.resolve();
    }
  };

  mustInterruptResponses();
  let promiseAttempt = download.start();

  
  
  
  
  yield deferMidway.promise;
  do_check_true(download.progress > 72 && download.progress < 78);

  
  continueResponses();
  yield promiseAttempt;

  
  do_check_eq(gMostRecentFirstBytePos, TEST_DATA_SHORT.length);

  
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
});





add_task(function test_cancel_midway_restart_removePartialData()
{
  let download = yield promiseStartDownload_tryToKeepPartialData();
  yield download.cancel();

  do_check_true(download.hasPartialData);
  yield promiseVerifyContents(download.target.partFilePath, TEST_DATA_SHORT);
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);

  yield download.removePartialData();

  do_check_false(download.hasPartialData);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);

  
  continueResponses();
  yield download.start();

  
  do_check_eq(gMostRecentFirstBytePos, 0);

  
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
});






add_task(function test_cancel_midway_restart_tryToKeepPartialData_false()
{
  let download = yield promiseStartDownload_tryToKeepPartialData();
  yield download.cancel();

  download.tryToKeepPartialData = false;

  
  do_check_true(download.hasPartialData);
  yield promiseVerifyContents(download.target.partFilePath, TEST_DATA_SHORT);

  yield download.removePartialData();
  do_check_false(yield OS.File.exists(download.target.partFilePath));

  
  mustInterruptResponses();
  download.start();

  yield promiseDownloadMidway(download);
  yield promisePartFileReady(download);

  
  do_check_false(download.hasPartialData);
  do_check_true(yield OS.File.exists(download.target.partFilePath));

  
  
  if (Services.appinfo.OS == "Darwin" || Services.appinfo.OS == "Linux") {
    do_check_eq((yield OS.File.stat(download.target.partFilePath)).unixMode,
                0o600);
  }

  yield download.cancel();

  
  do_check_false(download.hasPartialData);
  do_check_false(yield OS.File.exists(download.target.partFilePath));

  
  continueResponses();
  yield download.start();

  
  do_check_eq(gMostRecentFirstBytePos, 0);

  
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
});




add_task(function test_cancel_immediately_restart_immediately()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));
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

  
  
  continueResponses();
  try {
    yield promiseAttempt;
    
    
    
    do_print("The download should have been canceled.");
  } catch (ex if ex instanceof Downloads.Error) {
    do_check_false(ex.becauseSourceFailed);
    do_check_false(ex.becauseTargetFailed);
  }

  yield promiseRestarted;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_cancel_midway_restart_immediately()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));
  let promiseAttempt = download.start();

  
  yield promiseDownloadMidway(download);
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

  
  continueResponses();
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

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_cancel_successful()
{
  let download = yield promiseStartDownload();
  yield promiseDownloadStopped(download);

  
  yield download.cancel();

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);
});




add_task(function test_cancel_twice()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));

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

  do_check_false(yield OS.File.exists(download.target.path));
});




add_task(function test_refresh_succeeded()
{
  let download = yield promiseStartDownload();
  yield promiseDownloadStopped(download);

  
  yield download.refresh();
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);

  
  
  yield OS.File.move(download.target.path, download.target.path + ".old");
  yield download.refresh();
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, TEST_DATA_SHORT.length);

  
  yield OS.File.move(download.target.path + ".old", download.target.path);
  yield download.refresh();
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);
});




add_task(function test_finalize()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));

  let promiseFinalized = download.finalize();

  try {
    yield download.start();
    do_throw("It should not be possible to restart after finalization.");
  } catch (ex) { }

  yield promiseFinalized;

  do_check_true(download.stopped);
  do_check_false(download.succeeded);
  do_check_true(download.canceled);
  do_check_true(download.error === null);

  do_check_false(yield OS.File.exists(download.target.path));
});




add_task(function test_finalize_tryToKeepPartialData()
{
  
  let download = yield promiseStartDownload_tryToKeepPartialData();
  yield download.finalize();

  do_check_true(download.hasPartialData);
  do_check_true(yield OS.File.exists(download.target.partFilePath));

  
  yield download.removePartialData();

  
  download = yield promiseStartDownload_tryToKeepPartialData();
  yield download.finalize(true);

  do_check_false(download.hasPartialData);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
});




add_task(function test_whenSucceeded_after_restart()
{
  mustInterruptResponses();

  let promiseSucceeded;

  let download;
  if (!gUseLegacySaver) {
    
    
    download = yield promiseNewDownload(httpUrl("interruptible.txt"));
    promiseSucceeded = download.whenSucceeded();
    download.start();
  } else {
    
    
    download = yield promiseStartLegacyDownload(httpUrl("interruptible.txt"));
    promiseSucceeded = download.whenSucceeded();
  }

  
  yield download.cancel();

  
  continueResponses();
  download.start();

  
  yield promiseSucceeded;

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);
});




add_task(function test_error_source()
{
  let serverSocket = startFakeServer();
  try {
    let sourceUrl = "http://localhost:" + serverSocket.port + "/source.txt";

    let download;
    try {
      if (!gUseLegacySaver) {
        
        
        download = yield promiseNewDownload(sourceUrl);

        do_check_true(download.error === null);

        yield download.start();
      } else {
        
        
        
        
        download = yield promiseStartLegacyDownload(sourceUrl);
        yield promiseDownloadStopped(download);
      }
      do_throw("The download should have failed.");
    } catch (ex if ex instanceof Downloads.Error && ex.becauseSourceFailed) {
      
    }

    
    do_check_true(download.stopped);
    do_check_false(download.canceled);
    do_check_true(download.error !== null);
    do_check_true(download.error.becauseSourceFailed);
    do_check_false(download.error.becauseTargetFailed);

    do_check_false(yield OS.File.exists(download.target.path));
    do_check_false(download.target.exists);
    do_check_eq(download.target.size, 0);
  } finally {
    serverSocket.close();
  }
});





add_task(function test_error_source_partial()
{
  let sourceUrl = httpUrl("shorter-than-content-length-http-1-1.txt");

  let enforcePref = Services.prefs.getBoolPref("network.http.enforce-framing.http1");
  Services.prefs.setBoolPref("network.http.enforce-framing.http1", true);

  function cleanup() {
    Services.prefs.setBoolPref("network.http.enforce-framing.http1", enforcePref);
  }
  do_register_cleanup(cleanup);

  let download;
  try {
    if (!gUseLegacySaver) {
      
      
      download = yield promiseNewDownload(sourceUrl);

      do_check_true(download.error === null);

      yield download.start();
    } else {
      
      
      
      
      download = yield promiseStartLegacyDownload(sourceUrl);
      yield promiseDownloadStopped(download);
    }
    do_throw("The download should have failed.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseSourceFailed) {
    
  }

  
  do_check_true(download.stopped);
  do_check_false(download.canceled);
  do_check_true(download.error !== null);
  do_check_true(download.error.becauseSourceFailed);
  do_check_false(download.error.becauseTargetFailed);
  do_check_eq(download.error.result, Cr.NS_ERROR_NET_PARTIAL_TRANSFER);

  do_check_false(yield OS.File.exists(download.target.path));
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);
});




add_task(function test_error_target()
{
  
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  targetFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0);
  try {
    let download;
    try {
      if (!gUseLegacySaver) {
        
        
        download = yield Downloads.createDownload({
          source: httpUrl("source.txt"),
          target: targetFile,
        });
        yield download.start();
      } else {
        
        
        
        
        download = yield promiseStartLegacyDownload(null,
                                                    { targetFile: targetFile });
        yield promiseDownloadStopped(download);
      }
      do_throw("The download should have failed.");
    } catch (ex if ex instanceof Downloads.Error && ex.becauseTargetFailed) {
      
    }

    
    do_check_true(download.stopped);
    do_check_false(download.canceled);
    do_check_true(download.error !== null);
    do_check_true(download.error.becauseTargetFailed);
    do_check_false(download.error.becauseSourceFailed);
  } finally {
    
    if (targetFile.exists()) {
      targetFile.permissions = FileUtils.PERMS_FILE;
      targetFile.remove(false);
    }
  }
});




add_task(function test_error_restart()
{
  let download;

  
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  targetFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0);
  try {
    
    
    if (!gUseLegacySaver) {
      download = yield Downloads.createDownload({
        source: httpUrl("source.txt"),
        target: targetFile,
      });
      download.start();
    } else {
      download = yield promiseStartLegacyDownload(null,
                                                  { targetFile: targetFile });
    }
    yield promiseDownloadStopped(download);
    do_throw("The download should have failed.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseTargetFailed) {
    
  } finally {
    
    if (targetFile.exists()) {
      targetFile.permissions = FileUtils.PERMS_FILE;

      
      
      
      targetFile.moveTo(null, targetFile.leafName + ".delete.tmp");
      targetFile.remove(false);
    }
  }

  
  yield download.start();

  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.canceled);
  do_check_true(download.error === null);
  do_check_eq(download.progress, 100);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);
});




add_task(function test_public_and_private()
{
  let sourcePath = "/test_public_and_private.txt";
  let sourceUrl = httpUrl("test_public_and_private.txt");
  let testCount = 0;

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  function cleanup() {
    Services.prefs.clearUserPref("network.cookie.cookieBehavior");
    Services.cookies.removeAll();
    gHttpServer.registerPathHandler(sourcePath, null);
  }
  do_register_cleanup(cleanup);

  gHttpServer.registerPathHandler(sourcePath, function (aRequest, aResponse) {
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
  yield Downloads.fetch(sourceUrl, targetFile);
  yield Downloads.fetch(sourceUrl, targetFile);

  if (!gUseLegacySaver) {
    let download = yield Downloads.createDownload({
      source: { url: sourceUrl, isPrivate: true },
      target: targetFile,
    });
    yield download.start();
  } else {
    let download = yield promiseStartLegacyDownload(sourceUrl,
                                                    { isPrivate: true });
    yield promiseDownloadStopped(download);
  }

  cleanup();
});




add_task(function test_cancel_immediately_restart_and_check_startTime()
{
  let download = yield promiseStartDownload();

  let startTime = download.startTime;
  do_check_true(isValidDate(download.startTime));

  yield download.cancel();
  do_check_eq(download.startTime.getTime(), startTime.getTime());

  
  yield promiseTimeout(10);

  yield download.start();
  do_check_true(download.startTime.getTime() > startTime.getTime());
});




add_task(function test_with_content_encoding()
{
  let sourcePath = "/test_with_content_encoding.txt";
  let sourceUrl = httpUrl("test_with_content_encoding.txt");

  function cleanup() {
    gHttpServer.registerPathHandler(sourcePath, null);
  }
  do_register_cleanup(cleanup);

  gHttpServer.registerPathHandler(sourcePath, function (aRequest, aResponse) {
    aResponse.setHeader("Content-Type", "text/plain", false);
    aResponse.setHeader("Content-Encoding", "gzip", false);
    aResponse.setHeader("Content-Length",
                        "" + TEST_DATA_SHORT_GZIP_ENCODED.length, false);

    let bos = new BinaryOutputStream(aResponse.bodyOutputStream);
    bos.writeByteArray(TEST_DATA_SHORT_GZIP_ENCODED,
                       TEST_DATA_SHORT_GZIP_ENCODED.length);
  });

  let download = yield promiseStartDownload(sourceUrl);
  yield promiseDownloadStopped(download);

  do_check_eq(download.progress, 100);
  do_check_eq(download.totalBytes, TEST_DATA_SHORT_GZIP_ENCODED.length);

  
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);

  cleanup();
});




add_task(function test_with_content_encoding_ignore_extension()
{
  let sourcePath = "/test_with_content_encoding_ignore_extension.gz";
  let sourceUrl = httpUrl("test_with_content_encoding_ignore_extension.gz");

  function cleanup() {
    gHttpServer.registerPathHandler(sourcePath, null);
  }
  do_register_cleanup(cleanup);

  gHttpServer.registerPathHandler(sourcePath, function (aRequest, aResponse) {
    aResponse.setHeader("Content-Type", "text/plain", false);
    aResponse.setHeader("Content-Encoding", "gzip", false);
    aResponse.setHeader("Content-Length",
                        "" + TEST_DATA_SHORT_GZIP_ENCODED.length, false);

    let bos = new BinaryOutputStream(aResponse.bodyOutputStream);
    bos.writeByteArray(TEST_DATA_SHORT_GZIP_ENCODED,
                       TEST_DATA_SHORT_GZIP_ENCODED.length);
  });

  let download = yield promiseStartDownload(sourceUrl);
  yield promiseDownloadStopped(download);

  do_check_eq(download.progress, 100);
  do_check_eq(download.totalBytes, TEST_DATA_SHORT_GZIP_ENCODED.length);
  do_check_eq(download.target.size, TEST_DATA_SHORT_GZIP_ENCODED.length);

  
  
  yield promiseVerifyTarget(download.target,
        String.fromCharCode.apply(String, TEST_DATA_SHORT_GZIP_ENCODED));

  cleanup();
});




add_task(function test_cancel_midway_restart_with_content_encoding()
{
  mustInterruptResponses();

  let download = yield promiseStartDownload(httpUrl("interruptible_gzip.txt"));

  
  let deferCancel = Promise.defer();
  let onchange = function () {
    if (!download.stopped && !download.canceled &&
        download.currentBytes == TEST_DATA_SHORT_GZIP_ENCODED_FIRST.length) {
      deferCancel.resolve(download.cancel());
    }
  };

  
  
  download.onchange = onchange;
  onchange();

  yield deferCancel.promise;

  do_check_true(download.stopped);

  
  continueResponses();
  download.onchange = null;
  yield download.start();

  do_check_eq(download.progress, 100);
  do_check_eq(download.totalBytes, TEST_DATA_SHORT_GZIP_ENCODED.length);

  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);
});




add_task(function test_blocked_parental_controls()
{
  function cleanup() {
    DownloadIntegration.shouldBlockInTest = false;
  }
  do_register_cleanup(cleanup);
  DownloadIntegration.shouldBlockInTest = true;

  let download;
  try {
    if (!gUseLegacySaver) {
      
      
      download = yield promiseNewDownload();
      yield download.start();
    } else {
      
      
      
      
      download = yield promiseStartLegacyDownload();
      yield promiseDownloadStopped(download);
    }
    do_throw("The download should have blocked.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseBlocked) {
    do_check_true(ex.becauseBlockedByParentalControls);
    do_check_true(download.error.becauseBlockedByParentalControls);
  }

  
  do_check_false(yield OS.File.exists(download.target.path));

  cleanup();
});





add_task(function test_blocked_parental_controls_httpstatus450()
{
  let download;
  try {
    if (!gUseLegacySaver) {
      download = yield promiseNewDownload(httpUrl("parentalblocked.zip"));
      yield download.start();
    }
    else {
      download = yield promiseStartLegacyDownload(httpUrl("parentalblocked.zip"));
      yield promiseDownloadStopped(download);
    }
    do_throw("The download should have blocked.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseBlocked) {
    do_check_true(ex.becauseBlockedByParentalControls);
    do_check_true(download.error.becauseBlockedByParentalControls);
    do_check_true(download.stopped);
  }

  do_check_false(yield OS.File.exists(download.target.path));
});






add_task(function test_getSha256Hash()
{
  if (!gUseLegacySaver) {
    let download = yield promiseStartDownload(httpUrl("source.txt"));
    yield promiseDownloadStopped(download);
    do_check_true(download.stopped);
    do_check_eq(32, download.saver.getSha256Hash().length);
  }
});













let promiseBlockedDownload = Task.async(function* (options) {
  function cleanup() {
    DownloadIntegration.shouldBlockInTestForApplicationReputation = false;
    DownloadIntegration.shouldKeepBlockedDataInTest = false;
  }
  do_register_cleanup(cleanup);

  let {keepPartialData, keepBlockedData} = options;
  DownloadIntegration.shouldBlockInTestForApplicationReputation = true;
  DownloadIntegration.shouldKeepBlockedDataInTest = keepBlockedData;

  let download;

  try {
    if (keepPartialData) {
      download = yield promiseStartDownload_tryToKeepPartialData();
      continueResponses();
    } else if (gUseLegacySaver) {
      download = yield promiseStartLegacyDownload();
    } else {
      download = yield promiseNewDownload();
      yield download.start();
      do_throw("The download should have blocked.");
    }

    yield promiseDownloadStopped(download);
    do_throw("The download should have blocked.");
  } catch (ex if ex instanceof Downloads.Error && ex.becauseBlocked) {
    do_check_true(ex.becauseBlockedByReputationCheck);
    do_check_true(download.error.becauseBlockedByReputationCheck);
  }

  do_check_true(download.stopped);
  do_check_false(download.succeeded);
  do_check_false(yield OS.File.exists(download.target.path));

  cleanup();
  return download;
});





add_task(function test_blocked_applicationReputation()
{
  let download = yield promiseBlockedDownload({
    keepPartialData: false,
    keepBlockedData: false,
  });

  
  do_check_false(yield OS.File.exists(download.target.path));
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);

  
  do_check_false(download.hasBlockedData);
});





add_task(function test_blocked_applicationReputation_confirmBlock()
{
  let download = yield promiseBlockedDownload({
    keepPartialData: true,
    keepBlockedData: true,
  });

  do_check_true(download.hasBlockedData);
  do_check_true(yield OS.File.exists(download.target.partFilePath));

  yield download.confirmBlock();

  
  
  do_check_true(download.stopped);
  do_check_false(download.succeeded);
  do_check_false(download.hasBlockedData);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
  do_check_false(yield OS.File.exists(download.target.path));
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);
});





add_task(function test_blocked_applicationReputation_unblock()
{
  let download = yield promiseBlockedDownload({
    keepPartialData: true,
    keepBlockedData: true,
  });

  do_check_true(download.hasBlockedData);
  do_check_true(yield OS.File.exists(download.target.partFilePath));

  yield download.unblock();

  
  
  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.hasBlockedData);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
  yield promiseVerifyTarget(download.target, TEST_DATA_SHORT + TEST_DATA_SHORT);

  
  
  do_check_true(download.error instanceof Downloads.Error);
  do_check_true(download.error.becauseBlocked);
  do_check_true(download.error.becauseBlockedByReputationCheck);
});




add_task(function test_blocked_applicationReputation_cancel()
{
  let download = yield promiseBlockedDownload({
    keepPartialData: true,
    keepBlockedData: true,
  });

  
  yield download.cancel();

  
  
  do_check_true(download.error.becauseBlockedByReputationCheck);
  do_check_true(download.stopped);
  do_check_false(download.succeeded);
  do_check_true(download.hasBlockedData);
});




add_task(function test_blocked_applicationReputation_decisionRace()
{
  let download = yield promiseBlockedDownload({
    keepPartialData: true,
    keepBlockedData: true,
  });

  let unblockPromise = download.unblock();
  let confirmBlockPromise = download.confirmBlock();

  yield confirmBlockPromise.then(() => {
    do_throw("confirmBlock should have failed.");
  }, () => {});

  yield unblockPromise;

  
  
  do_check_true(download.stopped);
  do_check_true(download.succeeded);
  do_check_false(download.hasBlockedData);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
  do_check_true(yield OS.File.exists(download.target.path));

  download = yield promiseBlockedDownload({
    keepPartialData: true,
    keepBlockedData: true,
  });

  confirmBlockPromise = download.confirmBlock();
  unblockPromise = download.unblock();

  yield unblockPromise.then(() => {
    do_throw("unblock should have failed.");
  }, () => {});

  yield confirmBlockPromise;

  
  
  do_check_true(download.stopped);
  do_check_false(download.succeeded);
  do_check_false(download.hasBlockedData);
  do_check_false(yield OS.File.exists(download.target.partFilePath));
  do_check_false(yield OS.File.exists(download.target.path));
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);
});





add_task(function test_blocked_applicationReputation_unblock()
{
  let download = yield promiseBlockedDownload({
    keepPartialData: true,
    keepBlockedData: true,
  });

  do_check_true(download.hasBlockedData);
  do_check_true(yield OS.File.exists(download.target.partFilePath));

  
  yield OS.File.remove(download.target.partFilePath);

  let unblockPromise = download.unblock();
  yield unblockPromise.then(() => {
    do_throw("unblock should have failed.");
  }, () => {});

  
  
  do_check_false(download.hasBlockedData);
  do_check_true(download.stopped);
  do_check_false(download.succeeded);
  do_check_false(download.target.exists);
  do_check_eq(download.target.size, 0);
});




add_task(function test_showContainingDirectory() {
  DownloadIntegration._deferTestShowDir = Promise.defer();

  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;

  let download = yield Downloads.createDownload({
    source: { url: httpUrl("source.txt") },
    target: ""
  });

  try {
    yield download.showContainingDirectory();
    do_throw("Should have failed because of an invalid path.");
  } catch (ex if ex instanceof Components.Exception) {
    
    
    let validResult = ex.result == Cr.NS_ERROR_FILE_UNRECOGNIZED_PATH ||
                      ex.result == Cr.NS_ERROR_FAILURE;
    do_check_true(validResult);
  }

  download = yield Downloads.createDownload({
    source: { url: httpUrl("source.txt") },
    target: targetPath
  });


  DownloadIntegration._deferTestShowDir = Promise.defer();
  download.showContainingDirectory();
  let result = yield DownloadIntegration._deferTestShowDir.promise;
  do_check_eq(result, "success");
});




add_task(function test_launch() {
  let customLauncher = getTempFile("app-launcher");

  
  for (let launcherPath of [null, customLauncher.path]) {
    let download;
    if (!gUseLegacySaver) {
      
      
      download = yield Downloads.createDownload({
        source: httpUrl("source.txt"),
        target: getTempFile(TEST_TARGET_FILE_NAME).path,
        launcherPath: launcherPath,
        launchWhenSucceeded: true
      });

      try {
        yield download.launch();
        do_throw("Can't launch download file as it has not completed yet");
      } catch (ex) {
        do_check_eq(ex.message,
                    "launch can only be called if the download succeeded");
      }

      yield download.start();
    } else {
      
      
      download = yield promiseStartLegacyDownload(
                                         httpUrl("source.txt"),
                                         { launcherPath: launcherPath,
                                           launchWhenSucceeded: true });
      yield promiseDownloadStopped(download);
    }

    do_check_true(download.launchWhenSucceeded);

    DownloadIntegration._deferTestOpenFile = Promise.defer();
    download.launch();
    let result = yield DownloadIntegration._deferTestOpenFile.promise;

    
    if (!launcherPath) {
      
      do_check_true(result === null);
    } else {
      
      do_check_eq(result.preferredAction, Ci.nsIMIMEInfo.useHelperApp);
      do_check_true(result.preferredApplicationHandler
                          .QueryInterface(Ci.nsILocalHandlerApp)
                          .executable.equals(customLauncher));
    }
  }
});




add_task(function test_launcherPath_invalid() {
  let download = yield Downloads.createDownload({
    source: { url: httpUrl("source.txt") },
    target: { path: getTempFile(TEST_TARGET_FILE_NAME).path },
    launcherPath: " "
  });

  DownloadIntegration._deferTestOpenFile = Promise.defer();
  yield download.start();
  try {
    download.launch();
    result = yield DownloadIntegration._deferTestOpenFile.promise;
    do_throw("Can't launch file with invalid custom launcher")
  } catch (ex if ex instanceof Components.Exception) {
    
    
    let validResult = ex.result == Cr.NS_ERROR_FILE_UNRECOGNIZED_PATH ||
                      ex.result == Cr.NS_ERROR_FAILURE;
    do_check_true(validResult);
  }
});





add_task(function test_launchWhenSucceeded() {
  let customLauncher = getTempFile("app-launcher");

  
  for (let launcherPath of [null, customLauncher.path]) {
    DownloadIntegration._deferTestOpenFile = Promise.defer();

    if (!gUseLegacySaver) {
      let download = yield Downloads.createDownload({
        source: httpUrl("source.txt"),
        target: getTempFile(TEST_TARGET_FILE_NAME).path,
        launchWhenSucceeded: true,
        launcherPath: launcherPath,
      });
      yield download.start();
    } else {
      let download = yield promiseStartLegacyDownload(
                                             httpUrl("source.txt"),
                                             { launcherPath: launcherPath,
                                               launchWhenSucceeded: true });
      yield promiseDownloadStopped(download);
    }

    let result = yield DownloadIntegration._deferTestOpenFile.promise;

    
    if (!launcherPath) {
      
      do_check_true(result === null);
    } else {
      
      do_check_eq(result.preferredAction, Ci.nsIMIMEInfo.useHelperApp);
      do_check_true(result.preferredApplicationHandler
                          .QueryInterface(Ci.nsILocalHandlerApp)
                          .executable.equals(customLauncher));
    }
  }
});




add_task(function test_contentType() {
  let download = yield promiseStartDownload(httpUrl("source.txt"));
  yield promiseDownloadStopped(download);

  do_check_eq("text/plain", download.contentType);
});





add_task(function test_toSerializable_startTime()
{
  let download1 = yield promiseStartDownload(httpUrl("source.txt"));
  yield promiseDownloadStopped(download1);

  let serializable = download1.toSerializable();
  let reserialized = JSON.parse(JSON.stringify(serializable));

  let download2 = yield Downloads.createDownload(reserialized);

  do_check_eq(download1.startTime.constructor.name, "Date");
  do_check_eq(download2.startTime.constructor.name, "Date");
  do_check_eq(download1.startTime.toJSON(), download2.startTime.toJSON());
});






add_task(function test_platform_integration()
{
  let downloadFiles = [];
  let oldDeviceStorageEnabled = false;
  try {
     oldDeviceStorageEnabled = Services.prefs.getBoolPref("device.storage.enabled");
  } catch (e) {
    
  }
  let downloadWatcherNotified = false;
  let observer = {
    observe: function(subject, topic, data) {
      do_check_eq(topic, "download-watcher-notify");
      do_check_eq(data, "modified");
      downloadWatcherNotified = true;
    }
  }
  Services.obs.addObserver(observer, "download-watcher-notify", false);
  Services.prefs.setBoolPref("device.storage.enabled", true);
  function cleanup() {
    for (let file of downloadFiles) {
      file.remove(true);
    }
    Services.obs.removeObserver(observer, "download-watcher-notify");
    Services.prefs.setBoolPref("device.storage.enabled", oldDeviceStorageEnabled);
  }
  do_register_cleanup(cleanup);

  for (let isPrivate of [false, true]) {
    DownloadIntegration.downloadDoneCalled = false;

    
    
    
    
    let targetFilePath = yield DownloadIntegration.getSystemDownloadsDirectory();
    targetFilePath = OS.Path.join(targetFilePath,
                                  "test" + (Math.floor(Math.random() * 1000000)));
    let targetFile = new FileUtils.File(targetFilePath);
    downloadFiles.push(targetFile);

    let download;
    if (gUseLegacySaver) {
      download = yield promiseStartLegacyDownload(httpUrl("source.txt"),
                                                  { targetFile: targetFile });
    }
    else {
      download = yield Downloads.createDownload({
        source: httpUrl("source.txt"),
        target: targetFile,
      });
      download.start();
    }

    
    
    yield download.whenSucceeded().then(function () {
      do_check_true(DownloadIntegration.downloadDoneCalled);
      do_check_true(downloadWatcherNotified);
    });

    
    yield promiseDownloadStopped(download);

    yield promiseVerifyTarget(download.target, TEST_DATA_SHORT);
  }
});




add_task(function test_history()
{
  mustInterruptResponses();

  
  yield PlacesTestUtils.clearHistory();
  let promiseVisit = promiseWaitForVisit(httpUrl("interruptible.txt"));

  
  let download = yield promiseStartDownload(httpUrl("interruptible.txt"));

  
  let [time, transitionType] = yield promiseVisit;
  do_check_eq(time, download.startTime.getTime() * 1000);
  do_check_eq(transitionType, Ci.nsINavHistoryService.TRANSITION_DOWNLOAD);

  
  yield PlacesTestUtils.clearHistory();
  download.cancel();
  continueResponses();
  yield download.start();

  
  do_check_false(yield promiseIsURIVisited(httpUrl("interruptible.txt")));
});





add_task(function test_history_tryToKeepPartialData()
{
  
  yield PlacesTestUtils.clearHistory();
  let promiseVisit =
      promiseWaitForVisit(httpUrl("interruptible_resumable.txt"));

  
  let beforeStartTimeMs = Date.now();
  let download = yield promiseStartDownload_tryToKeepPartialData();

  
  let [time, transitionType] = yield promiseVisit;
  do_check_eq(transitionType, Ci.nsINavHistoryService.TRANSITION_DOWNLOAD);

  
  
  
  do_check_true(time >= beforeStartTimeMs * 1000 - 1000000);

  
  continueResponses();
  yield promiseDownloadStopped(download);
});





add_task(function test_launchWhenSucceeded_deleteTempFileOnExit() {
  const kDeleteTempFileOnExit = "browser.helperApps.deleteTempFileOnExit";

  let customLauncherPath = getTempFile("app-launcher").path;
  let autoDeleteTargetPathOne = getTempFile(TEST_TARGET_FILE_NAME).path;
  let autoDeleteTargetPathTwo = getTempFile(TEST_TARGET_FILE_NAME).path;
  let noAutoDeleteTargetPath = getTempFile(TEST_TARGET_FILE_NAME).path;

  let autoDeleteDownloadOne = yield Downloads.createDownload({
    source: { url: httpUrl("source.txt"), isPrivate: true },
    target: autoDeleteTargetPathOne,
    launchWhenSucceeded: true,
    launcherPath: customLauncherPath,
  });
  yield autoDeleteDownloadOne.start();

  Services.prefs.setBoolPref(kDeleteTempFileOnExit, true);
  let autoDeleteDownloadTwo = yield Downloads.createDownload({
    source: httpUrl("source.txt"),
    target: autoDeleteTargetPathTwo,
    launchWhenSucceeded: true,
    launcherPath: customLauncherPath,
  });
  yield autoDeleteDownloadTwo.start();

  Services.prefs.setBoolPref(kDeleteTempFileOnExit, false);
  let noAutoDeleteDownload = yield Downloads.createDownload({
    source: httpUrl("source.txt"),
    target: noAutoDeleteTargetPath,
    launchWhenSucceeded: true,
    launcherPath: customLauncherPath,
  });
  yield noAutoDeleteDownload.start();

  Services.prefs.clearUserPref(kDeleteTempFileOnExit);

  do_check_true(yield OS.File.exists(autoDeleteTargetPathOne));
  do_check_true(yield OS.File.exists(autoDeleteTargetPathTwo));
  do_check_true(yield OS.File.exists(noAutoDeleteTargetPath));

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  do_check_false(yield OS.File.exists(autoDeleteTargetPathOne));

  
  let expire = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                 .getService(Ci.nsIObserver);
  expire.observe(null, "profile-before-change", null);
  do_check_false(yield OS.File.exists(autoDeleteTargetPathTwo));
  do_check_true(yield OS.File.exists(noAutoDeleteTargetPath));
});
