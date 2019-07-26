


function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI = "http://mochi.test:8888/browser/browser/base/content/test/bug792517.html";
  let fileName;
  let MockFilePicker = SpecialPowers.MockFilePicker;
  let cache = Cc["@mozilla.org/network/cache-service;1"]
              .getService(Ci.nsICacheService);

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

  function contextMenuOpened(aWindow, event) {
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

    
    var saveVideoCommand = aWindow.document.getElementById("context-saveimage");
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

  function createTemporarySaveDirectory() {
    var saveDir = Cc["@mozilla.org/file/directory_service;1"]
                    .getService(Ci.nsIProperties)
                    .get("TmpD", Ci.nsIFile);
    saveDir.append("testsavedir");
    if (!saveDir.exists())
      saveDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    return saveDir;
  }

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    aWindow.gBrowser.addEventListener("pageshow", function pageShown(event) {
      if (event.target.location == "about:blank")
        return;
      aWindow.gBrowser.removeEventListener("pageshow", pageShown);

      executeSoon(function () {
        aWindow.document.addEventListener("popupshown",
                                          function(e) contextMenuOpened(aWindow, e), false);
        var img = aWindow.gBrowser.selectedBrowser.contentDocument.getElementById("img");
        EventUtils.synthesizeMouseAtCenter(img,
                                           { type: "contextmenu", button: 2 },
                                           aWindow.gBrowser.contentWindow);
      });
    });

    aWindow.gBrowser.selectedBrowser.loadURI(testURI);
  }

  function testOnWindow(aOptions, aCallback) {
    whenNewWindowLoaded(aOptions, function(aWin) {
      windowsToClose.push(aWin);
      
      
      
      executeSoon(function() aCallback(aWin));
    });
  };

   
  registerCleanupFunction(function() {
    windowsToClose.forEach(function(aWin) {
      aWin.close();
    });
  });

  MockFilePicker.init(window);
  
  testOnWindow({private: true}, function(aWin) {
    doTest(true, aWin, finish);
  });
}

Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://mochitests/content/browser/toolkit/content/tests/browser/common/mockTransfer.js",
                 this);
