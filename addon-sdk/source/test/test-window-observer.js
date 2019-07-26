


"use strict";

const { Loader } = require("sdk/test/loader");
const { open, close } = require("sdk/window/helpers");
const { browserWindows: windows } = require("sdk/windows");
const { isBrowser } = require('sdk/window/utils');
const app = require("sdk/system/xul-app");

exports["test unload window observer"] = function(assert, done) {
  
  let loader = Loader(module);
  let observer = loader.require("sdk/windows/observer").observer;
  let opened = 0;
  let closed = 0;
  let windowsOpen = windows.length;

  observer.on("open", onOpen);
  observer.on("close", onClose);

  
  if (app.is("Fennec")) {
    assert.pass("Windows observer did not throw on Fennec");
    return cleanUp();
  }

  
  open().
    then(close).
    then(loader.unload).
    then(open).
    then(close).
    then(function() {
      
      assert.equal(1, opened, "observer open was called before unload only");
      assert.equal(windowsOpen + 1, closed, "observer close was called before unload only");
    }).
    then(cleanUp, assert.fail);

  function cleanUp () {
    observer.removeListener("open", onOpen);
    observer.removeListener("close", onClose);
    done();
  }

  function onOpen(window) {
    
    if (isBrowser(window))
      opened++;
  }
  function onClose(window) {
    
    
    if (isBrowser(window))
      closed++;
  }
};

require("test").run(exports);
