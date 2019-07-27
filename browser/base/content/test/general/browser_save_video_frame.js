


const VIDEO_URL = "http://mochi.test:8888/browser/browser/base/content/test/general/web_video.html";





Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://mochitests/content/browser/toolkit/content/tests/browser/common/mockTransfer.js",
                 this);







function createTemporarySaveDirectory() {
  let saveDir = Cc["@mozilla.org/file/directory_service;1"]
                  .getService(Ci.nsIProperties)
                  .get("TmpD", Ci.nsIFile);
  saveDir.append("testsavedir");
  if (!saveDir.exists())
    saveDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  return saveDir;
}





function waitForTransferComplete() {
  return new Promise((resolve) => {
    mockTransferCallback = () => {
      ok(true, "Transfer completed");
      resolve();
    }
  });
}





function rightClickVideo(browser) {
  let frame_script = () => {
    const Ci = Components.interfaces;
    let utils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);

    let document = content.document;
    let video = document.getElementById("video1");
    let rect = video.getBoundingClientRect();

    
    let left = rect.left + (rect.width / 2);
    let top = rect.top + (rect.height / 2);

    utils.sendMouseEvent("contextmenu", left, top,
                         2, 
                         1, 
                         0  );
  };
  let mm = browser.messageManager;
  mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", true);
}






add_task(function*() {
  let MockFilePicker = SpecialPowers.MockFilePicker;
  MockFilePicker.init(window);

  
  let destDir = createTemporarySaveDirectory();
  let destFile = destDir.clone();

  MockFilePicker.displayDirectory = destDir;
  MockFilePicker.showCallback = function(fp) {
    destFile.append(fp.defaultString);
    MockFilePicker.returnFiles = [destFile];
    MockFilePicker.filterIndex = 1; 
  };

  mockTransferRegisterer.register();

  
  registerCleanupFunction(function () {
    mockTransferRegisterer.unregister();
    MockFilePicker.cleanup();
    destDir.remove(true);
  });

  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  let browser = tab.linkedBrowser;
  info("Loading video tab");
  yield promiseTabLoadEvent(tab, VIDEO_URL);
  info("Video tab loaded.");

  let video = browser.contentDocument.getElementById("video1");
  let context = document.getElementById("contentAreaContextMenu");
  let popupPromise = promisePopupShown(context);

  info("Synthesizing right-click on video element");
  rightClickVideo(browser);
  info("Waiting for popup to fire popupshown.");
  yield popupPromise;
  info("Popup fired popupshown");

  let saveSnapshotCommand = document.getElementById("context-video-saveimage");
  let promiseTransfer = waitForTransferComplete()
  info("Firing save snapshot command");
  saveSnapshotCommand.doCommand();
  context.hidePopup();
  info("Waiting for transfer completion");
  yield promiseTransfer;
  info("Transfer complete");
  gBrowser.removeTab(tab);
});
