



let {LoadContextInfo} = Cu.import("resource://gre/modules/LoadContextInfo.jsm", null);

function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI = "http://mochi.test:8888/browser/browser/base/content/test/general/bug792517.html";
  let fileName;
  let MockFilePicker = SpecialPowers.MockFilePicker;
  let cache = Cc["@mozilla.org/netwerk/cache-storage-service;1"]
              .getService(Ci.nsICacheStorageService);

  function checkDiskCacheFor(filename, goon) {
    Visitor.prototype = {
      onCacheStorageInfo: function(num, consumption)
      {
        info("disk storage contains " + num + " entries");
      },
      onCacheEntryInfo: function(uri)
      {
        var urispec = uri.asciiSpec;
        info(urispec);
        is(urispec.includes(filename), false, "web content present in disk cache");
      },
      onCacheEntryVisitCompleted: function()
      {
        goon();
      }
    };
    function Visitor() {}

    var storage = cache.diskCacheStorage(LoadContextInfo.default, false);
    storage.asyncVisitStorage(new Visitor(), true );
  }

  function onTransferComplete(downloadSuccess) {
    ok(downloadSuccess, "Image file should have been downloaded successfully");

    
    executeSoon(function() {
      checkDiskCacheFor(fileName, finish);
      mockTransferCallback = null;
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
    function contextMenuOpened(event) {
      cache.clear();

      aWindow.document.removeEventListener("popupshown", contextMenuOpened);

      
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

    aWindow.gBrowser.addEventListener("pageshow", function pageShown(event) {
      
      if (event.target.location == "about:blank" ||
          event.target.location == "about:privatebrowsing") {
        aWindow.gBrowser.selectedBrowser.loadURI(testURI);
        return;
      }
      aWindow.gBrowser.removeEventListener("pageshow", pageShown);

      waitForFocus(function () {
        aWindow.document.addEventListener("popupshown", contextMenuOpened, false);
        var img = aWindow.gBrowser.selectedBrowser.contentDocument.getElementById("img");
        EventUtils.synthesizeMouseAtCenter(img,
                                           { type: "contextmenu", button: 2 },
                                           aWindow.gBrowser.contentWindow);
      }, aWindow.gBrowser.selectedBrowser.contentWindow);
    });
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
