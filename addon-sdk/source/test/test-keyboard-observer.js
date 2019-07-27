


"use strict";

const { keyPress } = require("sdk/dom/events/keys");
const { Loader } = require("sdk/test/loader");
const timer = require("sdk/timers");

exports["test unload keyboard observer"] = function(assert, done) {
  let loader = Loader(module);
  let element = loader.require("sdk/deprecated/window-utils").
                       activeBrowserWindow.document.documentElement;
  let observer = loader.require("sdk/keyboard/observer").
                        observer;
  let called = 0;

  observer.on("keypress", function () { called++; });

  
  keyPress(element, "accel-%");

  
  loader.unload();

  
  keyPress(element, "accel-%");

  
  timer.setTimeout(function () {
    assert.equal(called, 1, "observer was called before unload only.");
    done();
  }, 0);
};

require("sdk/test").run(exports);
