



let FormHistory = (Components.utils.import("resource://gre/modules/FormHistory.jsm", {})).FormHistory;


function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI =
    "http://example.com/tests/toolkit/components/satchel/test/subtst_privbrowsing.html";

  function doTest(aIsPrivateMode, aShouldValueExist, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      let checks = 0;
      function doneCheck() {
        checks++;
        if (checks == 2) {
          executeSoon(aCallback);
        }
      }

      
      
      
      aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
        aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
        doneCheck();
      }, true);

      FormHistory.count({ fieldname: "field", value: "value" },
        { onSuccess: function(num) {
            is(num >= 1, aShouldValueExist, "Checking value exists in form history");
            doneCheck();
          },
          onFailure: function (error) {
            do_throw("Error occurred searching form history: " + error);
          }
        });
    }, true);

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


  testOnWindow({private: true}, function(aWin) {
    doTest(true, false, aWin, function() {
      
      
      testOnWindow({}, function(aWin) {
        doTest(false, false, aWin, function() {
          finish();
        });
      });
    });
  });
}
