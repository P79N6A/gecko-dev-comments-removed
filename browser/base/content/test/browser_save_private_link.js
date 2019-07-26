


var MockFilePicker = SpecialPowers.MockFilePicker;
MockFilePicker.init(window);

function checkDiskCacheFor(filename) {
  let visitor = {
    visitDevice: function(deviceID, deviceInfo) {
      if (deviceID == "disk")
        info(deviceID + " device contains " + deviceInfo.entryCount + " entries");
      return deviceID == "disk";
    },
    
    visitEntry: function(deviceID, entryInfo) {
      info(entryInfo.key);
      is(entryInfo.key.contains(filename), false, "web content present in disk cache");
    }
  };
  cache.visitEntries(visitor);
}

var cache = Cc["@mozilla.org/network/cache-service;1"]
              .getService(Ci.nsICacheService);

function test() {
  waitForExplicitFinish();
  var fileName;

  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  pb.privateBrowsingEnabled = true;

  gBrowser.loadURI("http://mochi.test:8888/browser/browser/base/content/test/bug792517.html");

  registerCleanupFunction(function () {
    pb.privateBrowsingEnabled = false;
    gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
    gBrowser.addTab();
    gBrowser.removeCurrentTab();
  });

  gBrowser.addEventListener("pageshow", function pageShown(event) {
    if (event.target.location == "about:blank")
      return;
    gBrowser.removeEventListener("pageshow", pageShown);

    executeSoon(function () {
      document.addEventListener("popupshown", contextMenuOpened);

      var img = gBrowser.contentDocument.getElementById("img");
      EventUtils.synthesizeMouseAtCenter(img,
                                         { type: "contextmenu", button: 2 },
                                         gBrowser.contentWindow);
    });
  });

  function contextMenuOpened(event) {
    cache.evictEntries(Ci.nsICache.STORE_ANYWHERE);

    event.currentTarget.removeEventListener("popupshown", contextMenuOpened);

    
    var destDir = createTemporarySaveDirectory();
    var destFile = destDir.clone();

    MockFilePicker.displayDirectory = destDir;
    MockFilePicker.showCallback = function(fp) {
      fileName = fp.defaultString;
      destFile.append (fileName);
      MockFilePicker.returnFiles = [destFile];
      MockFilePicker.filterIndex = 1; 
    };

    mockTransferCallback = onTransferComplete;
    mockTransferRegisterer.register();

    registerCleanupFunction(function () {
      mockTransferRegisterer.unregister();
      MockFilePicker.cleanup();
      destDir.remove(true);
    });

    
    var saveVideoCommand = document.getElementById("context-saveimage");
    saveVideoCommand.doCommand();

    event.target.hidePopup();
  }

  function onTransferComplete(downloadSuccess) {
    ok(downloadSuccess, "Image file should have been downloaded successfully");

    
    executeSoon(function() {
      checkDiskCacheFor(fileName);
      finish();
    });
  }
}

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
