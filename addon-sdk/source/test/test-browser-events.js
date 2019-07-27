


"use strict";

module.metadata = {
  engines: {
    "Firefox": "*"
  }
};

const { Loader } = require("sdk/test/loader");
const { open, getMostRecentBrowserWindow, getOuterId } = require("sdk/window/utils");
const { setTimeout } = require("sdk/timers");

exports["test browser events"] = function(assert, done) {
  let loader = Loader(module);
  let { events } = loader.require("sdk/browser/events");
  let { on, off } = loader.require("sdk/event/core");
  let actual = [];

  on(events, "data", function handler(e) {
    actual.push(e);
    if (e.type === "load") window.close();
    if (e.type === "close") {
      

      let [ ready, load, close ] = actual;

      assert.equal(ready.type, "DOMContentLoaded");
      assert.equal(ready.target, window, "window ready");

      assert.equal(load.type, "load");
      assert.equal(load.target, window, "window load");

      assert.equal(close.type, "close");
      assert.equal(close.target, window, "window load");

      
      
      
      off(events, "data", handler);
      loader.unload();
      done();
    }
  });

  
  let window = open();
};

exports["test browser events ignore other wins"] = function(assert, done) {
  let loader = Loader(module);
  let { events: windowEvents } = loader.require("sdk/window/events");
  let { events: browserEvents } = loader.require("sdk/browser/events");
  let { on, off } = loader.require("sdk/event/core");
  let actualBrowser = [];
  let actualWindow = [];

  function browserEventHandler(e) actualBrowser.push(e)
  on(browserEvents, "data", browserEventHandler);
  on(windowEvents, "data", function handler(e) {
    actualWindow.push(e);
    
    
    if (e.type === "load") setTimeout(window.close);
    if (e.type === "close") {
      assert.deepEqual(actualBrowser, [], "browser events were not triggered");
      let [ open, ready, load, close ] = actualWindow;

      assert.equal(open.type, "open");
      assert.equal(open.target, window, "window is open");



      assert.equal(ready.type, "DOMContentLoaded");
      assert.equal(ready.target, window, "window ready");

      assert.equal(load.type, "load");
      assert.equal(load.target, window, "window load");

      assert.equal(close.type, "close");
      assert.equal(close.target, window, "window load");


      
      
      
      off(windowEvents, "data", handler);
      off(browserEvents, "data", browserEventHandler);
      loader.unload();
      done();
    }
  });

  
  let window = open("data:text/html,not a browser");
};

require("sdk/test").run(exports);
