






"use strict";




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
  cleanup();
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

