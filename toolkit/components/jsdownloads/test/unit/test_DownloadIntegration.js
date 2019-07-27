






"use strict";








function enableObserversTestMode() {
  DownloadIntegration.testMode = true;
  DownloadIntegration.dontLoadObservers = false;
  function cleanup() {
    DownloadIntegration.testMode = false;
    DownloadIntegration.dontLoadObservers = true;
  }
  do_register_cleanup(cleanup);
}











function notifyPromptObservers(aIsPrivate, aExpectedCount, aExpectedPBCount) {
  let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].
                   createInstance(Ci.nsISupportsPRBool);

  
  DownloadIntegration.testPromptDownloads = -1;
  Services.obs.notifyObservers(cancelQuit, "quit-application-requested", null);
  do_check_eq(DownloadIntegration.testPromptDownloads, aExpectedCount);

  
  DownloadIntegration.testPromptDownloads = -1;
  Services.obs.notifyObservers(cancelQuit, "offline-requested", null);
  do_check_eq(DownloadIntegration.testPromptDownloads, aExpectedCount);

  if (aIsPrivate) {
    
    DownloadIntegration.testPromptDownloads = -1;
    Services.obs.notifyObservers(cancelQuit, "last-pb-context-exiting", null);
    do_check_eq(DownloadIntegration.testPromptDownloads, aExpectedPBCount);
  }
}




XPCOMUtils.defineLazyGetter(this, "gStringBundle", function() {
  return Services.strings.
    createBundle("chrome://mozapps/locale/downloads/downloads.properties");
});





add_task(function test_getSystemDownloadsDirectory()
{
  
  
  
  DownloadIntegration.testMode = true;
  function cleanup() {
    DownloadIntegration.testMode = false;
  }
  do_register_cleanup(cleanup);

  let tempDir = Services.dirsvc.get("TmpD", Ci.nsIFile);
  let downloadDir;

  
  if (Services.appinfo.OS == "Darwin" ||
      Services.appinfo.OS == "Linux" ||
      (Services.appinfo.OS == "WINNT" &&
       parseFloat(Services.sysinfo.getProperty("version")) >= 6)) {
    downloadDir = yield DownloadIntegration.getSystemDownloadsDirectory();
    do_check_eq(downloadDir, tempDir.path);
    do_check_true(yield OS.File.exists(downloadDir));

    let info = yield OS.File.stat(downloadDir);
    do_check_true(info.isDir);
  } else {
    let targetPath = OS.Path.join(tempDir.path,
                       gStringBundle.GetStringFromName("downloadsFolder"));
    try {
      yield OS.File.removeEmptyDir(targetPath);
    } catch(e) {}
    downloadDir = yield DownloadIntegration.getSystemDownloadsDirectory();
    do_check_eq(downloadDir, targetPath);
    do_check_true(yield OS.File.exists(downloadDir));

    let info = yield OS.File.stat(downloadDir);
    do_check_true(info.isDir);
    yield OS.File.removeEmptyDir(targetPath);
  }

  let downloadDirBefore = yield DownloadIntegration.getSystemDownloadsDirectory();
  cleanup();
  let downloadDirAfter = yield DownloadIntegration.getSystemDownloadsDirectory();
  do_check_neq(downloadDirBefore, downloadDirAfter);
});





add_task(function test_getPreferredDownloadsDirectory()
{
  let folderListPrefName = "browser.download.folderList";
  let dirPrefName = "browser.download.dir";
  function cleanup() {
    Services.prefs.clearUserPref(folderListPrefName);
    Services.prefs.clearUserPref(dirPrefName);
  }
  do_register_cleanup(cleanup);

  
  Services.prefs.setIntPref(folderListPrefName, 1);
  let systemDir = yield DownloadIntegration.getSystemDownloadsDirectory();
  let downloadDir = yield DownloadIntegration.getPreferredDownloadsDirectory();
  do_check_neq(downloadDir, "");
  do_check_eq(downloadDir, systemDir);

  
  Services.prefs.setIntPref(folderListPrefName, 0);
  downloadDir = yield DownloadIntegration.getPreferredDownloadsDirectory();
  do_check_neq(downloadDir, "");
  do_check_eq(downloadDir, Services.dirsvc.get("Desk", Ci.nsIFile).path);

  
  
  Services.prefs.setIntPref(folderListPrefName, 2);
  downloadDir = yield DownloadIntegration.getPreferredDownloadsDirectory();
  do_check_neq(downloadDir, "");
  do_check_eq(downloadDir, systemDir);

  
  let time = (new Date()).getTime();
  let tempDir = Services.dirsvc.get("TmpD", Ci.nsIFile);
  tempDir.append(time);
  Services.prefs.setComplexValue("browser.download.dir", Ci.nsIFile, tempDir);
  downloadDir = yield DownloadIntegration.getPreferredDownloadsDirectory();
  do_check_neq(downloadDir, "");
  do_check_eq(downloadDir,  tempDir.path);
  do_check_true(yield OS.File.exists(downloadDir));
  yield OS.File.removeEmptyDir(tempDir.path);

  
  
  tempDir = Services.dirsvc.get("TmpD", Ci.nsIFile);
  tempDir.append("dir_not_exist");
  tempDir.append(time);
  Services.prefs.setComplexValue("browser.download.dir", Ci.nsIFile, tempDir);
  downloadDir = yield DownloadIntegration.getPreferredDownloadsDirectory();
  do_check_eq(downloadDir, systemDir);

  
  
  Services.prefs.setIntPref(folderListPrefName, 999);
  downloadDir = yield DownloadIntegration.getPreferredDownloadsDirectory();
  do_check_eq(downloadDir, systemDir);

  cleanup();
});





add_task(function test_getTemporaryDownloadsDirectory()
{
  let downloadDir = yield DownloadIntegration.getTemporaryDownloadsDirectory();
  do_check_neq(downloadDir, "");

  if ("nsILocalFileMac" in Ci) {
    let preferredDownloadDir = yield DownloadIntegration.getPreferredDownloadsDirectory();
    do_check_eq(downloadDir, preferredDownloadDir);
  } else {
    let tempDir = Services.dirsvc.get("TmpD", Ci.nsIFile);
    do_check_eq(downloadDir, tempDir.path);
  }
});








add_task(function test_notifications()
{
  enableObserversTestMode();

  for (let isPrivate of [false, true]) {
    mustInterruptResponses();

    let list = yield promiseNewList(isPrivate);
    let download1 = yield promiseNewDownload(httpUrl("interruptible.txt"));
    let download2 = yield promiseNewDownload(httpUrl("interruptible.txt"));
    let download3 = yield promiseNewDownload(httpUrl("interruptible.txt"));
    let promiseAttempt1 = download1.start();
    let promiseAttempt2 = download2.start();
    download3.start();

    
    yield list.add(download1);
    yield list.add(download2);
    yield list.add(download3);
    
    yield download3.cancel();

    notifyPromptObservers(isPrivate, 2, 2);

    
    continueResponses();
    yield promiseAttempt1;
    yield promiseAttempt2;

    
    yield list.remove(download1);
    yield list.remove(download2);
    yield list.remove(download3);
  }
});





add_task(function test_no_notifications()
{
  enableObserversTestMode();

  for (let isPrivate of [false, true]) {
    let list = yield promiseNewList(isPrivate);
    let download1 = yield promiseNewDownload(httpUrl("interruptible.txt"));
    let download2 = yield promiseNewDownload(httpUrl("interruptible.txt"));
    download1.start();
    download2.start();

    
    yield list.add(download1);
    yield list.add(download2);

    yield download1.cancel();
    yield download2.cancel();

    notifyPromptObservers(isPrivate, 0, 0);

    
    yield list.remove(download1);
    yield list.remove(download2);
  }
});





add_task(function test_mix_notifications()
{
  enableObserversTestMode();
  mustInterruptResponses();

  let publicList = yield promiseNewList();
  let privateList = yield Downloads.getList(Downloads.PRIVATE);
  let download1 = yield promiseNewDownload(httpUrl("interruptible.txt"));
  let download2 = yield promiseNewDownload(httpUrl("interruptible.txt"));
  let promiseAttempt1 = download1.start();
  let promiseAttempt2 = download2.start();

  
  yield publicList.add(download1);
  yield privateList.add(download2);

  notifyPromptObservers(true, 2, 1);

  
  continueResponses();
  yield promiseAttempt1;
  yield promiseAttempt2;

  
  yield publicList.remove(download1);
  yield privateList.remove(download2);
});





add_task(function test_suspend_resume()
{
  enableObserversTestMode();

  
  
  Services.prefs.setIntPref("browser.download.manager.resumeOnWakeDelay", 5);

  let addDownload = function(list)
  {
    return Task.spawn(function () {
      let download = yield promiseNewDownload(httpUrl("interruptible.txt"));
      download.start();
      list.add(download);
      throw new Task.Result(download);
    });
  }

  let publicList = yield promiseNewList();
  let privateList = yield promiseNewList(true);

  let download1 = yield addDownload(publicList);
  let download2 = yield addDownload(publicList);
  let download3 = yield addDownload(privateList);
  let download4 = yield addDownload(privateList);
  let download5 = yield addDownload(publicList);

  
  Services.obs.notifyObservers(null, "sleep_notification", null);
  do_check_true(download1.canceled);
  do_check_true(download2.canceled);
  do_check_true(download3.canceled);
  do_check_true(download4.canceled);
  do_check_true(download5.canceled);

  
  publicList.remove(download5);
  do_check_true(download5.canceled);

  
  
  
  Services.obs.notifyObservers(null, "wake_notification", null);
  yield download1.whenSucceeded();
  yield download2.whenSucceeded();
  yield download3.whenSucceeded();
  yield download4.whenSucceeded();

  
  
  do_check_false(download1.canceled);
  do_check_true(download5.canceled);

  

  download1 = yield addDownload(publicList);
  download2 = yield addDownload(publicList);
  download3 = yield addDownload(privateList);
  download4 = yield addDownload(privateList);

  
  Services.obs.notifyObservers(null, "network:offline-about-to-go-offline", null);
  do_check_true(download1.canceled);
  do_check_true(download2.canceled);
  do_check_true(download3.canceled);
  do_check_true(download4.canceled);

  
  Services.obs.notifyObservers(null, "network:offline-status-changed", "online");
  yield download1.whenSucceeded();
  yield download2.whenSucceeded();
  yield download3.whenSucceeded();
  yield download4.whenSucceeded();

  Services.prefs.clearUserPref("browser.download.manager.resumeOnWakeDelay");
});





add_task(function test_exit_private_browsing()
{
  enableObserversTestMode();
  mustInterruptResponses();

  let privateList = yield promiseNewList(true);
  let download1 = yield promiseNewDownload(httpUrl("source.txt"));
  let download2 = yield promiseNewDownload(httpUrl("interruptible.txt"));
  let promiseAttempt1 = download1.start();
  let promiseAttempt2 = download2.start();

  
  yield privateList.add(download1);
  yield privateList.add(download2);

  
  yield promiseAttempt1;

  do_check_eq((yield privateList.getAll()).length, 2);

  
  DownloadIntegration._deferTestClearPrivateList = Promise.defer();
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  let result = yield DownloadIntegration._deferTestClearPrivateList.promise;

  do_check_eq(result, "success");
  do_check_eq((yield privateList.getAll()).length, 0);

  continueResponses();
});




let tailFile = do_get_file("tail.js");
Services.scriptloader.loadSubScript(NetUtil.newURI(tailFile).spec);
