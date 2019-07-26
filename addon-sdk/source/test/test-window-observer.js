


"use strict";


module.metadata = {
  engines: {
    'Firefox': '*'
  }
};

const { Loader } = require("sdk/test/loader");
const { open, close } = require("sdk/window/helpers");
const { browserWindows: windows } = require("sdk/windows");
const { isBrowser } = require('sdk/window/utils');

exports["test unload window observer"] = function(assert, done) {
  
  let loader = Loader(module);
  let observer = loader.require("sdk/windows/observer").observer;
  let opened = 0;
  let closed = 0;
  let windowsOpen = windows.length;

  observer.on("open", function onOpen(window) {
    
    if (isBrowser(window))
      opened++;
  });
  observer.on("close", function onClose(window) {
    
    
    if (isBrowser(window))
      closed++;
  });

  
  open().
    then(close).
    then(loader.unload).
    then(open).
    then(close).
    then(function() {
      
      assert.equal(1, opened, "observer open was called before unload only");
      assert.equal(windowsOpen + 1, closed, "observer close was called before unload only");
    }).
    then(done, assert.fail);
};

require("test").run(exports);
