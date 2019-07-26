



"use strict";

const { Loader } = require("sdk/test/loader");
const timer = require("sdk/timers");

exports["test unload window observer"] = function(assert, done) {
  
  let loader = Loader(module);

  let utils = loader.require("sdk/deprecated/window-utils");
  let { activeBrowserWindow: activeWindow } = utils;
  let { isBrowser } = require('sdk/window/utils');
  let observer = loader.require("sdk/windows/observer").observer;
  let opened = 0;
  let closed = 0;

  observer.on("open", function onOpen(window) {
    
    if (isBrowser(window))
      opened++;
  });
  observer.on("close", function onClose(window) {
    
    
    if (isBrowser(window) && window !== activeWindow)
      closed++;
  });

  
  activeWindow.open().close();

  
  loader.unload();

  
  activeWindow.open().close();

  
  timer.setTimeout(function () {
    assert.equal(1, opened, "observer open was called before unload only");
    assert.equal(1, closed, "observer close was called before unload only");
    done();
  }, 0);
};

if (require("sdk/system/xul-app").is("Fennec")) {
  module.exports = {
    "test Unsupported Test": function UnsupportedTest (assert) {
        assert.pass(
          "Skipping this test until Fennec support is implemented." +
          "See bug 793071");
    }
  }
}

require("test").run(exports);
