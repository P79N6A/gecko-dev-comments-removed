


"use strict";


module.metadata = {
  engines: {
    'Firefox': '*'
  }
};

const { Loader } = require("sdk/test/loader");
const { open, getMostRecentBrowserWindow, getOuterId } = require("sdk/window/utils");

exports["test browser events"] = function(assert, done) {
  let loader = Loader(module);
  let { events } = loader.require("sdk/window/events");
  let { on, off } = loader.require("sdk/event/core");
  let actual = [];

  on(events, "data", function handler(e) {
    actual.push(e);

    if (e.type === "open") {
      assert.pass("window open has occured");
    }
    else if (e.type === "DOMContentLoaded") {
      assert.pass("window DOMContentLoaded has occured");
    }
    else if (e.type === "load") {
      assert.pass("window load has occured");
      window.close();
    }
    else if (e.type === "close") {
      
      let [ open, ready, load, close ] = actual;
      assert.equal(open.type, "open")
      assert.equal(open.target, window, "window is open")

      assert.equal(ready.type, "DOMContentLoaded")
      assert.equal(ready.target, window, "window ready")

      assert.equal(load.type, "load")
      assert.equal(load.target, window, "window load")

      assert.equal(close.type, "close")
      assert.equal(close.target, window, "window load")

      
      
      
      off(events, "data", handler);
      loader.unload();
      done();
    }
  });

  
  let window = open();
};

require("sdk/test").run(exports);
