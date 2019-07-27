


function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let innerID;
  let beforeEvents;
  let afterEvents;
  let storageShouldOccur;
  let consoleObserver;
  let testURI =
    "http://example.com/browser/dom/tests/browser/test-console-api.html";
  let ConsoleAPIStorage = Cc["@mozilla.org/consoleAPI-storage;1"]
                            .getService(Ci.nsIConsoleAPIStorage);

  function getInnerWindowId(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIDOMWindowUtils)
                  .currentInnerWindowID;
  }

  function whenNewWindowLoaded(aOptions, aCallback) {
    let win = OpenBrowserWindow(aOptions);
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);
      aCallback(win);
    }, false);
  }

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      consoleObserver = {
        observe: function(aSubject, aTopic, aData) {
          if (aTopic == "console-api-log-event") {
            afterEvents = ConsoleAPIStorage.getEvents(innerID);
            is(beforeEvents.length == afterEvents.length - 1, storageShouldOccur,
              "storage should" + (storageShouldOccur ? "" : " not") + " occur");

            executeSoon(function() {
              Services.obs.removeObserver(consoleObserver, "console-api-log-event");
              aCallback();
            });
          }
        }
      };

      aWindow.Services.obs.addObserver(
        consoleObserver, "console-api-log-event", false);
      aWindow.nativeConsole.log("foo bar baz (private: " + aIsPrivateMode + ")");
    }, true);

    
    storageShouldOccur = true;
    innerID = getInnerWindowId(aWindow);
    beforeEvents = ConsoleAPIStorage.getEvents(innerID);
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

  
  testOnWindow({}, function(aWin) {
    doTest(false, aWin, function() {
      
      testOnWindow({private: true}, function(aWin) {
        doTest(true, aWin, finish);
      });
    });
  });
}
