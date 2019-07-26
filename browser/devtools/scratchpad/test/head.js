



"use strict";

let tempScope = {};

Cu.import("resource://gre/modules/NetUtil.jsm", tempScope);
Cu.import("resource://gre/modules/FileUtils.jsm", tempScope);
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", tempScope);


let NetUtil = tempScope.NetUtil;
let FileUtils = tempScope.FileUtils;
let Promise = tempScope.Promise;

let gScratchpadWindow; 






















function openScratchpad(aReadyCallback, aOptions)
{
  aOptions = aOptions || {};

  let win = aOptions.window ||
            Scratchpad.ScratchpadManager.openScratchpad(aOptions.state);
  if (!win) {
    return;
  }

  let onLoad = function() {
    win.removeEventListener("load", onLoad, false);

    win.Scratchpad.addObserver({
      onReady: function(aScratchpad) {
        aScratchpad.removeObserver(this);

        if (aOptions.noFocus) {
          aReadyCallback(win, aScratchpad);
        } else {
          waitForFocus(aReadyCallback.bind(null, win, aScratchpad), win);
        }
      }
    });
  };

  if (aReadyCallback) {
    win.addEventListener("load", onLoad, false);
  }

  gScratchpadWindow = win;
  return gScratchpadWindow;
}














function createTempFile(aName, aContent, aCallback=function(){})
{
  
  let file = FileUtils.getFile("TmpD", [aName]);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));

  
  let fout = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
  fout.init(file.QueryInterface(Ci.nsILocalFile), 0x02 | 0x08 | 0x20,
            parseInt("644", 8), fout.DEFER_OPEN);

  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let fileContentStream = converter.convertToInputStream(aContent);

  NetUtil.asyncCopy(fileContentStream, fout, function (aStatus) {
    aCallback(aStatus, file);
  });
}



















function runAsyncTests(aScratchpad, aTests)
{
  let deferred = Promise.defer();

  (function runTest() {
    if (aTests.length) {
      let test = aTests.shift();
      aScratchpad.setText(test.code);
      aScratchpad[test.method]().then(function success() {
        is(aScratchpad.getText(), test.result, test.label);
        runTest();
      }, function failure(error) {
        ok(false, error.stack + " " + test.label);
        runTest();
      });
    } else {
      deferred.resolve();
    }
  })();

  return deferred.promise;
}


















function runAsyncCallbackTests(aScratchpad, aTests)
{
  let deferred = Promise.defer();

  (function runTest() {
    if (aTests.length) {
      let test = aTests.shift();
      test.prepare();
      aScratchpad[test.method]().then(test.then.bind(test)).then(runTest);
    } else {
      deferred.resolve();
    }
  })();

  return deferred.promise;
}


function cleanup()
{
  if (gScratchpadWindow) {
    gScratchpadWindow.close();
    gScratchpadWindow = null;
  }
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
}

registerCleanupFunction(cleanup);
