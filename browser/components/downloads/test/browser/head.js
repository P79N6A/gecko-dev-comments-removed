











XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
const nsIDM = Ci.nsIDownloadManager;

let gTestTargetFile = FileUtils.getFile("TmpD", ["dm-ui-test.file"]);
gTestTargetFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
registerCleanupFunction(function () {
  gTestTargetFile.remove(false);
});




function promiseOpenAndLoadWindow(aOptions)
{
  return new Promise((resolve, reject) => {
    let win = OpenBrowserWindow(aOptions);
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad);
      resolve(win);
    });
  });
}















function promiseTabLoadEvent(tab, url, eventType="load")
{
  let deferred = Promise.defer();
  info("Wait tab event: " + eventType);

  function handle(event) {
    if (event.originalTarget != tab.linkedBrowser.contentDocument ||
        event.target.location.href == "about:blank" ||
        (url && event.target.location.href != url)) {
      info("Skipping spurious '" + eventType + "'' event" +
           " for " + event.target.location.href);
      return;
    }
    
    realCleanup = () => {};
    tab.linkedBrowser.removeEventListener(eventType, handle, true);
    info("Tab event received: " + eventType);
    deferred.resolve(event);
  }

  
  let realCleanup = () => tab.linkedBrowser.removeEventListener(eventType, handle, true);
  registerCleanupFunction(() => realCleanup());

  tab.linkedBrowser.addEventListener(eventType, handle, true, true);
  if (url)
    tab.linkedBrowser.loadURI(url);
  return deferred.promise;
}

function promiseWindowClosed(win)
{
  let promise = new Promise((resolve, reject) => {
    Services.obs.addObserver(function obs(subject, topic) {
      if (subject == win) {
        Services.obs.removeObserver(obs, topic);
        resolve();
      }
    }, "domwindowclosed", false);
  });
  win.close();
  return promise;
}


function promiseFocus()
{
  let deferred = Promise.defer();
  waitForFocus(deferred.resolve);
  return deferred.promise;
}

function promisePanelOpened()
{
  let deferred = Promise.defer();

  if (DownloadsPanel.panel && DownloadsPanel.panel.state == "open") {
    return deferred.resolve();
  }

  
  let originalOnPopupShown = DownloadsPanel.onPopupShown;
  DownloadsPanel.onPopupShown = function () {
    DownloadsPanel.onPopupShown = originalOnPopupShown;
    originalOnPopupShown.apply(this, arguments);

    
    
    setTimeout(deferred.resolve, 0);
  };

  return deferred.promise;
}

function task_resetState()
{
  
  let publicList = yield Downloads.getList(Downloads.PUBLIC);
  let downloads = yield publicList.getAll();
  for (let download of downloads) {
    publicList.remove(download);
    yield download.finalize(true);
  }

  DownloadsPanel.hidePanel();

  yield promiseFocus();
}

function task_addDownloads(aItems)
{
  let startTimeMs = Date.now();

  let publicList = yield Downloads.getList(Downloads.PUBLIC);
  for (let item of aItems) {
    publicList.add(yield Downloads.createDownload({
      source: "http://www.example.com/test-download.txt",
      target: gTestTargetFile,
      succeeded: item.state == nsIDM.DOWNLOAD_FINISHED,
      canceled: item.state == nsIDM.DOWNLOAD_CANCELED ||
                item.state == nsIDM.DOWNLOAD_PAUSED,
      error: item.state == nsIDM.DOWNLOAD_FAILED ? new Error("Failed.") : null,
      hasPartialData: item.state == nsIDM.DOWNLOAD_PAUSED,
      startTime: new Date(startTimeMs++),
    }));
  }
}

function task_openPanel()
{
  yield promiseFocus();

  let promise = promisePanelOpened();
  DownloadsPanel.showPanel();
  yield promise;
}
