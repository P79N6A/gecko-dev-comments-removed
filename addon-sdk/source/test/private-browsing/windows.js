


'use strict';

const { pb, pbUtils } = require('./helper');
const { onFocus, openDialog, open } = require('sdk/window/utils');
const { open: openPromise, close, focus, promise } = require('sdk/window/helpers');
const { isPrivate } = require('sdk/private-browsing');
const { browserWindows: windows } = require('sdk/windows');
const { defer } = require('sdk/core/promise');
const tabs = require('sdk/tabs');




exports.testPerWindowPrivateBrowsingGetter = function(assert, done) {
  let win = openDialog({
    private: true
  });

  promise(win, 'DOMContentLoaded').then(function onload() {
    assert.equal(pbUtils.getMode(win),
                 true, 'Newly opened window is in PB mode');
    assert.ok(isPrivate(win), 'isPrivate(window) is true');
    assert.equal(pb.isActive, false, 'PB mode is not active');

    close(win).then(function() {
      assert.equal(pb.isActive, false, 'PB mode is not active');
      done();
    });
  });
}




exports.testPerWindowPrivateBrowsingGetter = function(assert, done) {
  let win = open('chrome://browser/content/browser.xul', {
    features: {
      private: true
    }
  });

  promise(win, 'DOMContentLoaded').then(function onload() {
    assert.equal(pbUtils.getMode(win),
                 true, 'Newly opened window is in PB mode');
    assert.ok(isPrivate(win), 'isPrivate(window) is true');
    assert.equal(pb.isActive, false, 'PB mode is not active');

    close(win).then(function() {
      assert.equal(pb.isActive, false, 'PB mode is not active');
      done();
    });
  });
}

exports.testIsPrivateOnWindowOpen = function(assert, done) {
  windows.open({
    isPrivate: true,
    onOpen: function(window) {
      assert.equal(isPrivate(window), false, 'isPrivate for a window is true when it should be');
      assert.equal(isPrivate(window.tabs[0]), false, 'isPrivate for a tab is false when it should be');
      window.close(done);
    }
  });
}

exports.testIsPrivateOnWindowOpenFromPrivate = function(assert, done) {
    
    openPromise(null, {
      features: {
        private: true,
        chrome: true,
        titlebar: true,
        toolbar: true
      }
    }).then(focus).then(function(window) {
      let { promise, resolve } = defer();

      assert.equal(isPrivate(window), true, 'the only open window is private');

      windows.open({
        url: 'about:blank',
        onOpen: function(w) {
          assert.equal(isPrivate(w), false, 'new test window is not private');
          w.close(function() resolve(window));
        }
      });

      return promise;
    }).then(close).
       then(done, assert.fail);
};

exports.testOpenTabWithPrivateWindow = function*(assert) {
  let { promise, resolve } = defer();

  let window = yield openPromise(null, {
    features: {
      private: true,
      toolbar: true
    }
  });
  yield focus(window);

  assert.equal(isPrivate(window), true, 'the focused window is private');

  tabs.open({
    url: 'about:blank',
    onOpen: (tab) => {
      assert.equal(isPrivate(tab), false, 'the opened tab is not private');
      tab.close(resolve);
    }
  });

  yield promise;
  yield close(window);
};

exports.testIsPrivateOnWindowOff = function(assert, done) {
  windows.open({
    onOpen: function(window) {
      assert.equal(isPrivate(window), false, 'isPrivate for a window is false when it should be');
      assert.equal(isPrivate(window.tabs[0]), false, 'isPrivate for a tab is false when it should be');
      window.close(done);
    }
  })
}
