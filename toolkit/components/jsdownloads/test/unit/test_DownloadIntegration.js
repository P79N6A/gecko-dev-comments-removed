






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
    do_check_true(downloadDir instanceof Ci.nsIFile);
    do_check_eq(downloadDir.path, tempDir.path);
    do_check_true(yield OS.File.exists(downloadDir.path));

    let info = yield OS.File.stat(downloadDir.path);
    do_check_true(info.isDir);
  } else {
    let targetPath = OS.Path.join(tempDir.path,
                       gStringBundle.GetStringFromName("downloadsFolder"));
    try {
      yield OS.File.removeEmptyDir(targetPath);
    } catch(e) {}
    downloadDir = yield DownloadIntegration.getSystemDownloadsDirectory();
    do_check_eq(downloadDir.path, targetPath);
    do_check_true(yield OS.File.exists(downloadDir.path));

    let info = yield OS.File.stat(downloadDir.path);
    do_check_true(info.isDir);
    yield OS.File.removeEmptyDir(targetPath);
  }

  let downloadDirBefore = yield DownloadIntegration.getSystemDownloadsDirectory();
  cleanup();
  let downloadDirAfter = yield DownloadIntegration.getSystemDownloadsDirectory();
  do_check_false(downloadDirBefore.equals(downloadDirAfter));
});





add_task(function test_getUserDownloadsDirectory()
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
  let downloadDir = yield DownloadIntegration.getUserDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
  do_check_eq(downloadDir.path, systemDir.path);

  
  Services.prefs.setIntPref(folderListPrefName, 0);
  downloadDir = yield DownloadIntegration.getUserDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
  do_check_eq(downloadDir.path, Services.dirsvc.get("Desk", Ci.nsIFile).path);

  
  
  Services.prefs.setIntPref(folderListPrefName, 2);
  let downloadDir = yield DownloadIntegration.getUserDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
  do_check_eq(downloadDir.path, systemDir.path);

  
  let time = (new Date()).getTime();
  let tempDir = Services.dirsvc.get("TmpD", Ci.nsIFile);
  tempDir.append(time);
  Services.prefs.setComplexValue("browser.download.dir", Ci.nsIFile, tempDir);
  downloadDir = yield DownloadIntegration.getUserDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
  do_check_eq(downloadDir.path,  tempDir.path);
  do_check_true(yield OS.File.exists(downloadDir.path));
  yield OS.File.removeEmptyDir(tempDir.path);

  
  
  tempDir = Services.dirsvc.get("TmpD", Ci.nsIFile);
  tempDir.append("dir_not_exist");
  tempDir.append(time);
  Services.prefs.setComplexValue("browser.download.dir", Ci.nsIFile, tempDir);
  downloadDir = yield DownloadIntegration.getUserDownloadsDirectory();
  do_check_eq(downloadDir.path, systemDir.path);

  
  
  Services.prefs.setIntPref(folderListPrefName, 999);
  let downloadDir = yield DownloadIntegration.getUserDownloadsDirectory();
  do_check_eq(downloadDir.path, systemDir.path);

  cleanup();
});





add_task(function test_getTemporaryDownloadsDirectory()
{
  let downloadDir = yield DownloadIntegration.getTemporaryDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);

  if ("nsILocalFileMac" in Ci) {
    let userDownloadDir = yield DownloadIntegration.getUserDownloadsDirectory();
    do_check_eq(downloadDir.path, userDownloadDir.path);
  } else {
    let tempDir = Services.dirsvc.get("TmpD", Ci.nsIFile);
    do_check_eq(downloadDir.path, tempDir.path);
  }
});








add_task(function test_notifications()
{
  enableObserversTestMode();
  mustInterruptResponses();

  for (let isPrivate of [false, true]) {
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

