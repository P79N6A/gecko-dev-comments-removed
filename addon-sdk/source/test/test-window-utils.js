



"use strict";

var windowUtils = require("sdk/deprecated/window-utils");
var timer = require("sdk/timers");
var { Cc, Ci } = require("chrome");
var { Loader, unload } = require("sdk/test/loader");

function toArray(iterator) {
  let array = [];
  for each (let item in iterator)
    array.push(item);
  return array;
}

function makeEmptyWindow() {
  var xulNs = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
  var blankXul = ('<?xml version="1.0"?>' +
                  '<?xml-stylesheet href="chrome://global/skin/" ' +
                  '                 type="text/css"?>' +
                  '<window xmlns="' + xulNs + '" windowtype="test:window">' +
                  '</window>');
  var url = "data:application/vnd.mozilla.xul+xml;charset=utf-8," + escape(blankXul);
  var features = ["chrome", "width=10", "height=10"];

  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"]
           .getService(Ci.nsIWindowWatcher);
  return ww.openWindow(null, url, null, features.join(","), null);
}

exports['test close on unload'] = function(assert) {
  var timesClosed = 0;
  var fakeWindow = {
    _listeners: [],
    addEventListener: function(name, func, bool) {
      this._listeners.push(func);
    },
    removeEventListener: function(name, func, bool) {
      var index = this._listeners.indexOf(func);
      if (index == -1)
        throw new Error("event listener not found");
      this._listeners.splice(index, 1);
    },
    close: function() {
      timesClosed++;
      this._listeners.forEach(
        function(func) {
          func({target: fakeWindow.document});
        });
    },
    document: {
      get defaultView() { return fakeWindow; }
    }
  };

  let loader = Loader(module);
  loader.require("sdk/deprecated/window-utils").closeOnUnload(fakeWindow);
  assert.equal(fakeWindow._listeners.length, 1,
                   "unload listener added on closeOnUnload()");
  assert.equal(timesClosed, 0,
                   "window not closed when registered.");
  loader.unload();
  assert.equal(timesClosed, 1,
                   "window closed on module unload.");
  assert.equal(fakeWindow._listeners.length, 0,
                   "unload event listener removed on module unload");

  timesClosed = 0;
  loader = Loader(module);
  loader.require("sdk/deprecated/window-utils").closeOnUnload(fakeWindow);
  assert.equal(timesClosed, 0,
                   "window not closed when registered.");
  fakeWindow.close();
  assert.equal(timesClosed, 1,
                   "window closed when close() called.");
  assert.equal(fakeWindow._listeners.length, 0,
                   "unload event listener removed on window close");
  loader.unload();
  assert.equal(timesClosed, 1,
                   "window not closed again on module unload.");
};

exports['test window watcher'] = function(assert, done) {
  var myWindow;
  var finished = false;

  var delegate = {
    onTrack: function(window) {
      if (window == myWindow) {
        assert.pass("onTrack() called with our test window");
        timer.setTimeout(function() { myWindow.close(); }, 1);
      }
    },
    onUntrack: function(window) {
      if (window == myWindow) {
        assert.pass("onUntrack() called with our test window");
        timer.setTimeout(function() {
                           if (!finished) {
                             finished = true;
                             myWindow = null;
                             wt.unload();
                             done();
                           } else
                             assert.fail("finishTest() called multiple times.");
                         }, 1);
      }
    }
  };

  
  var wt = new windowUtils.WindowTracker(delegate);
  myWindow = makeEmptyWindow();
};

exports['test window watcher untracker'] = function(assert, done) {
  var myWindow;
  var tracks = 0;
  var unloadCalled = false;

  var delegate = {
    onTrack: function(window) {
      tracks = tracks + 1;
      if (window == myWindow) {
        assert.pass("onTrack() called with our test window");
        timer.setTimeout(function() {
          myWindow.close();
        }, 1);
      }
    },
    onUntrack: function(window) {
      tracks = tracks - 1;
      if (window == myWindow && !unloadCalled) {
        unloadCalled = true;
        timer.setTimeout(function() {
          wt.unload();
        }, 1);
      }
      if (0 > tracks) {
        assert.fail("WindowTracker onUntrack was called more times than onTrack..");
      }
      else if (0 == tracks) {
        timer.setTimeout(function() {
            myWindow = null;
            done();
        }, 1);
      }
    }
  };

  
  var wt = windowUtils.WindowTracker(delegate);
  myWindow = makeEmptyWindow();
};


exports['test window watcher unregs 4 loading wins'] = function(assert, done) {
  var myWindow;
  var finished = false;
  let browserWindow =  Cc["@mozilla.org/appshell/window-mediator;1"]
      .getService(Ci.nsIWindowMediator)
      .getMostRecentWindow("navigator:browser");
  var counter = 0;

  var delegate = {
    onTrack: function(window) {
      var type = window.document.documentElement.getAttribute("windowtype");
      if (type == "test:window")
        assert.fail("onTrack shouldn't have been executed.");
    }
  };
  var wt = new windowUtils.WindowTracker(delegate);

  
  myWindow = makeEmptyWindow();

  
  assert.notEqual(
      myWindow.document.readyState,
      "complete",
      "window hasn't loaded yet.");

  
  wt.unload();

  
  
  assert.notEqual(
      myWindow.document.readyState,
      "complete",
      "window still hasn't loaded yet.");

  
  
  
  myWindow.addEventListener("load", function() {
    
    myWindow.setTimeout(function() {
      myWindow.addEventListener("unload", function() {
        
        done();
      }, false);
      myWindow.close();
    }, 0);
  }, false);
}

exports['test window watcher without untracker'] = function(assert, done) {
  var myWindow;
  var finished = false;

  var delegate = {
    onTrack: function(window) {
      if (window == myWindow) {
        assert.pass("onTrack() called with our test window");
        timer.setTimeout(function() {
          myWindow.close();

          if (!finished) {
              finished = true;
              myWindow = null;
              wt.unload();
              done();
            } else {
              assert.fail("onTrack() called multiple times.");
            }
        }, 1);
      }
    }
  };

  var wt = new windowUtils.WindowTracker(delegate);
  myWindow = makeEmptyWindow();
};

exports['test active window'] = function(assert, done) {
  let browserWindow =  Cc["@mozilla.org/appshell/window-mediator;1"]
                      .getService(Ci.nsIWindowMediator)
                      .getMostRecentWindow("navigator:browser");

  assert.equal(windowUtils.activeBrowserWindow, browserWindow,
               "Browser window is the active browser window.");


  let testSteps = [
    function() {
      windowUtils.activeWindow = browserWindow;
      continueAfterFocus(browserWindow);
    },
    function() {
      assert.equal(windowUtils.activeWindow, browserWindow,
                       "Correct active window [1]");
      nextTest();
    },
    function() {
      assert.equal(windowUtils.activeBrowserWindow, browserWindow,
                       "Correct active browser window [2]");
      continueAfterFocus(windowUtils.activeWindow = browserWindow);
    },
    function() {
      assert.equal(windowUtils.activeWindow, browserWindow,
                       "Correct active window [3]");
      nextTest();
    },
    function() {
      assert.equal(windowUtils.activeBrowserWindow, browserWindow,
                       "Correct active browser window [4]");
      browserWindow = null;
      done();
    }
  ];

  let nextTest = function() {
    let func = testSteps.shift();
    if (func) {
      func();
    }
  }

  function continueAfterFocus(targetWindow) {
    
    var fm = Cc["@mozilla.org/focus-manager;1"].
             getService(Ci.nsIFocusManager);

    var childTargetWindow = {};
    fm.getFocusedElementForWindow(targetWindow, true, childTargetWindow);
    childTargetWindow = childTargetWindow.value;

    var focusedChildWindow = {};
    if (fm.activeWindow) {
      fm.getFocusedElementForWindow(fm.activeWindow, true, focusedChildWindow);
      focusedChildWindow = focusedChildWindow.value;
    }

    var focused = (focusedChildWindow == childTargetWindow);
    if (focused) {
      nextTest();
    } else {
      childTargetWindow.addEventListener("focus", function focusListener() {
        childTargetWindow.removeEventListener("focus", focusListener, true);
        nextTest();
      }, true);
    }

  }

  nextTest();
};

exports['test windowIterator'] = function(assert, done) {
  
  let window = makeEmptyWindow();

  
  assert.notEqual(
      window.document.readyState,
      "complete",
      "window hasn't loaded yet.");

  
  assert.ok(toArray(windowUtils.windowIterator()).indexOf(window) === -1,
            "window isn't in windowIterator()");

  
  window.addEventListener("load", function onload() {
    window.addEventListener("load", onload, false);
    assert.ok(toArray(windowUtils.windowIterator()).indexOf(window) !== -1,
              "window is now in windowIterator(false)");

    
    window.addEventListener("unload", function onunload() {
      window.addEventListener("unload", onunload, false);
      done();
    }, false);
    window.close();
  }, false);
}


if (require("sdk/system/xul-app").is("Fennec")) {
  module.exports = {
    "test Unsupported Test": function UnsupportedTest (assert) {
        assert.pass(
          "Skipping this test until Fennec support is implemented." +
          "See bug 809412");
    }
  }
}

require("test").run(exports);
