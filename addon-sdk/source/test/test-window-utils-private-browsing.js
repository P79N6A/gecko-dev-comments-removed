


'use strict';

const windowUtils = require('sdk/deprecated/window-utils');
const { Cc, Ci } = require('chrome');
const { isWindowPBSupported } = require('sdk/private-browsing/utils');
const { getFrames, getWindowTitle, onFocus, isWindowPrivate } = require('sdk/window/utils');
const { open, close, focus } = require('sdk/window/helpers');
const WM = Cc['@mozilla.org/appshell/window-mediator;1'].getService(Ci.nsIWindowMediator);
const { isPrivate } = require('sdk/private-browsing');
const { fromIterator: toArray } = require('sdk/util/array');

function makeEmptyBrowserWindow(options) {
  options = options || {};
  return open('chrome://browser/content/browser.xul', {
    features: {
      chrome: true,
      private: !!options.private
    }
  });
}

exports.testWindowTrackerIgnoresPrivateWindows = function(assert, done) {
  var myNonPrivateWindow, myPrivateWindow;
  var finished = false;
  var privateWindow;
  var privateWindowClosed = false;

  let wt = windowUtils.WindowTracker({
    onTrack: function(window) {
      if (isWindowPrivate(window)) {
        assert.fail('private window was tracked!');
      }
    },
    onUntrack: function(window) {
      if (isWindowPrivate(window)) {
        assert.fail('private window was tracked!');
      }
      
      if (window === myPrivateWindow && isWindowPBSupported) {
        privateWindowClosed = true;
      }
      if (window === myNonPrivateWindow) {
        assert.ok(!privateWindowClosed);
        wt.unload();
        done();
      }
    }
  });

  
  makeEmptyBrowserWindow({
    private: true
  }).then(function(window) {
    myPrivateWindow = window;

    assert.equal(isWindowPrivate(window), isWindowPBSupported);
    assert.ok(getFrames(window).length > 1, 'there are frames for private window');
    assert.equal(getWindowTitle(window), window.document.title,
                 'getWindowTitle works');

    close(window).then(function() {
      makeEmptyBrowserWindow().then(function(window) {
        myNonPrivateWindow = window;
        assert.pass('opened new window');
        window.close();
      });
    });
  });
};


exports.testSettingActiveWindowDoesNotIgnorePrivateWindow = function(assert, done) {
  let browserWindow = WM.getMostRecentWindow("navigator:browser");
  let testSteps;

  assert.equal(windowUtils.activeBrowserWindow, browserWindow,
               "Browser window is the active browser window.");
  assert.ok(!isPrivate(browserWindow), "Browser window is not private.");

  
  makeEmptyBrowserWindow({
    private: true
  }).then(focus).then(function(window) {
    let continueAfterFocus = function(window) onFocus(window).then(nextTest);

    
    if (isWindowPBSupported) {
      assert.ok(isPrivate(window), "window is private");
      assert.notDeepEqual(windowUtils.activeBrowserWindow, browserWindow);
    }
    
    else {
      assert.ok(!isPrivate(window), "window is not private");
    }

    assert.strictEqual(windowUtils.activeBrowserWindow, window,
                 "Correct active browser window pb supported");
    assert.notStrictEqual(browserWindow, window,
                 "The window is not the old browser window");

    testSteps = [
      function() {
        
        continueAfterFocus(windowUtils.activeWindow = browserWindow);
      },
      function() {
        assert.strictEqual(windowUtils.activeWindow, browserWindow,
                           "Correct active window [1]");
        assert.strictEqual(windowUtils.activeBrowserWindow, browserWindow,
                           "Correct active browser window [1]");

        
        focus(window).then(nextTest);
      },
      function(w) {
        assert.strictEqual(w, window, 'require("sdk/window/helpers").focus on window works');
        assert.strictEqual(windowUtils.activeBrowserWindow, window,
                           "Correct active browser window [2]");
        assert.strictEqual(windowUtils.activeWindow, window,
                           "Correct active window [2]");

        
        continueAfterFocus(windowUtils.activeWindow = window);
      },
      function() {
        assert.deepEqual(windowUtils.activeBrowserWindow, window,
                         "Correct active browser window [3]");
        assert.deepEqual(windowUtils.activeWindow, window,
                         "Correct active window [3]");

        
        continueAfterFocus(windowUtils.activeWindow = browserWindow);
      },
      function() {
        assert.deepEqual(windowUtils.activeBrowserWindow, browserWindow,
                         "Correct active browser window when pb mode is supported [4]");
        assert.deepEqual(windowUtils.activeWindow, browserWindow,
                         "Correct active window when pb mode is supported [4]");

        close(window).then(done);
      }
    ];

    function nextTest() {
      let args = arguments;
      if (testSteps.length) {
        require('sdk/timers').setTimeout(function() {
          (testSteps.shift()).apply(null, args);
        }, 0);
      }
    }
    nextTest();
  });
};

exports.testActiveWindowDoesNotIgnorePrivateWindow = function(assert, done) {
  
  makeEmptyBrowserWindow({
    private: true
  }).then(function(window) {
    
    if (isWindowPBSupported) {
      assert.equal(isPrivate(windowUtils.activeWindow), true,
                   "active window is private");
      assert.equal(isPrivate(windowUtils.activeBrowserWindow), true,
                   "active browser window is private");
      assert.ok(isWindowPrivate(window), "window is private");
      assert.ok(isPrivate(window), "window is private");

      
      assert.ok(
        isWindowPrivate(windowUtils.activeWindow),
        "active window is private when pb mode is supported");
      assert.ok(
        isWindowPrivate(windowUtils.activeBrowserWindow),
        "active browser window is private when pb mode is supported");
      assert.ok(isPrivate(windowUtils.activeWindow),
                "active window is private when pb mode is supported");
      assert.ok(isPrivate(windowUtils.activeBrowserWindow),
        "active browser window is private when pb mode is supported");
    }
    
    else {
      assert.equal(isPrivate(windowUtils.activeWindow), false,
                   "active window is not private");
      assert.equal(isPrivate(windowUtils.activeBrowserWindow), false,
                   "active browser window is not private");
      assert.equal(isWindowPrivate(window), false, "window is not private");
      assert.equal(isPrivate(window), false, "window is not private");
    }

    close(window).then(done);
  });
}

exports.testWindowIteratorIgnoresPrivateWindows = function(assert, done) {
  
  makeEmptyBrowserWindow({
    private: true
  }).then(function(window) {
    
    if (isWindowPBSupported) {
      assert.ok(isWindowPrivate(window), "window is private");
      assert.equal(toArray(windowUtils.windowIterator()).indexOf(window), -1,
                   "window is not in windowIterator()");
    }
    
    else {
      assert.equal(isWindowPrivate(window), false, "window is not private");
      assert.ok(toArray(windowUtils.windowIterator()).indexOf(window) > -1,
                "window is in windowIterator()"); 
    }

    close(window).then(done);
  });
};

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
