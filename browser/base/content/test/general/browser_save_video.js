


var MockFilePicker = SpecialPowers.MockFilePicker;
MockFilePicker.init(window);





add_task(function* () {
  var fileName;

  let loadPromise = BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);
  gBrowser.loadURI("http://mochi.test:8888/browser/browser/base/content/test/general/web_video.html");
  yield loadPromise;

  let popupShownPromise = BrowserTestUtils.waitForEvent(document, "popupshown");

  yield BrowserTestUtils.synthesizeMouseAtCenter("#video1",
                                                 { type: "contextmenu", button: 2 },
                                                 gBrowser.selectedBrowser);
  info("context menu click on video1");

  yield popupShownPromise;

  info("context menu opened on video1");

  
  var destDir = createTemporarySaveDirectory();
  var destFile = destDir.clone();

  MockFilePicker.displayDirectory = destDir;
  MockFilePicker.showCallback = function(fp) {
    fileName = fp.defaultString;
    destFile.append(fileName);
    MockFilePicker.returnFiles = [destFile];
    MockFilePicker.filterIndex = 1; 
  };

  let transferCompletePromise = new Promise((resolve) => {
    function onTransferComplete(downloadSuccess) {
      ok(downloadSuccess, "Video file should have been downloaded successfully");

      is(fileName, "web-video1-expectedName.ogv",
         "Video file name is correctly retrieved from Content-Disposition http header");
      resolve();
    }

    mockTransferCallback = onTransferComplete;
    mockTransferRegisterer.register();
  });

  registerCleanupFunction(function () {
    mockTransferRegisterer.unregister();
    MockFilePicker.cleanup();
    destDir.remove(true);
  });

  
  var saveVideoCommand = document.getElementById("context-savevideo");
  saveVideoCommand.doCommand();
  info("context-savevideo command executed");

  let contextMenu = document.getElementById("contentAreaContextMenu");
  let popupHiddenPromise = BrowserTestUtils.waitForEvent(contextMenu, "popuphidden");
  contextMenu.hidePopup();
  yield popupHiddenPromise;

  yield transferCompletePromise;
});


Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://mochitests/content/browser/toolkit/content/tests/browser/common/mockTransfer.js",
                 this);

function createTemporarySaveDirectory() {
  var saveDir = Cc["@mozilla.org/file/directory_service;1"]
                  .getService(Ci.nsIProperties)
                  .get("TmpD", Ci.nsIFile);
  saveDir.append("testsavedir");
  if (!saveDir.exists())
    saveDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  return saveDir;
}
