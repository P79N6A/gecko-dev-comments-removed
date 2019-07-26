


'use strict';

const { pb, pbUtils } = require('./helper');
const { openDialog, open } = require('sdk/window/utils');
const { promise, close } = require('sdk/window/helpers');
const { isPrivate } = require('sdk/private-browsing');
const { browserWindows: windows } = require('sdk/windows');




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

exports.testIsPrivateOnWindowOn = function(assert, done) {
  windows.open({
    isPrivate: true,
    onOpen: function(window) {
      assert.equal(isPrivate(window), false, 'isPrivate for a window is true when it should be');
      assert.equal(isPrivate(window.tabs[0]), false, 'isPrivate for a tab is false when it should be');
      window.close(done);
    }
  });
}

exports.testIsPrivateOnWindowOff = function(assert, done) {
  windows.open({
    onOpen: function(window) {
      assert.equal(isPrivate(window), false, 'isPrivate for a window is false when it should be');
      assert.equal(isPrivate(window.tabs[0]), false, 'isPrivate for a tab is false when it should be');
      window.close(done);
    }
  })
}

require("test").run(exports);
