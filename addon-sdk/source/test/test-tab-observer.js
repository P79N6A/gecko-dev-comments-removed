



"use strict";

const { openTab, closeTab } = require("sdk/tabs/utils");
const { Loader } = require("sdk/test/loader");
const { setTimeout } = require("sdk/timers");

exports["test unload tab observer"] = function(assert, done) {
  let loader = Loader(module);

  let window = loader.require("sdk/deprecated/window-utils").activeBrowserWindow;
  let observer = loader.require("sdk/tabs/observer").observer;
  let opened = 0;
  let closed = 0;

  observer.on("open", function onOpen(window) { opened++; });
  observer.on("close", function onClose(window) { closed++; });

  
  closeTab(openTab(window, "data:text/html;charset=utf-8,tab-1"));

  
  loader.unload();

  
  closeTab(openTab(window, "data:text/html;charset=utf-8,tab-2"));

  
  setTimeout(function () {
    assert.equal(1, opened, "observer open was called before unload only");
    assert.equal(1, closed, "observer close was called before unload only");
    done();
  }, 0);
};

if (require("sdk/system/xul-app").is("Fennec")) {
  module.exports = {
    "test Unsupported Test": function UnsupportedTest (assert) {
        assert.pass(
          "Skipping this test until Fennec support is implemented."
        );
    }
  }
}

require("test").run(exports);

