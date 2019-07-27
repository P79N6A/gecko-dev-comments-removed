


'use strict';

const { openWindow, closeWindow } = require('./util');
const { Loader } = require('sdk/test/loader');
const { getMostRecentBrowserWindow } = require('sdk/window/utils');
const { Cc, Ci } = require('chrome');
const els = Cc["@mozilla.org/eventlistenerservice;1"].
            getService(Ci.nsIEventListenerService);

function countListeners(target, type) {
  let listeners = els.getListenerInfoFor(target, {});
  return listeners.filter(listener => listener.type == type).length;
}

exports['test window close clears listeners'] = function(assert) {
  let window = yield openWindow();
  let loader = Loader(module);

  
  let gBrowser = window.gBrowser;

  
  let windowListeners = countListeners(window, "DOMWindowClose");

  
  assert.equal(countListeners(gBrowser, "TestEvent1"), 0, "Should be no listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 0, "Should be no listener for test event 2");

  let { open } = loader.require('sdk/event/dom');

  open(gBrowser, "TestEvent1");
  assert.equal(countListeners(window, "DOMWindowClose"), windowListeners + 1,
               "Should have added a DOMWindowClose listener");
  assert.equal(countListeners(gBrowser, "TestEvent1"), 1, "Should be a listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 0, "Should be no listener for test event 2");

  open(gBrowser, "TestEvent2");
  assert.equal(countListeners(window, "DOMWindowClose"), windowListeners + 1,
               "Should not have added another DOMWindowClose listener");
  assert.equal(countListeners(gBrowser, "TestEvent1"), 1, "Should be a listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 1, "Should be a listener for test event 2");

  window = yield closeWindow(window);

  assert.equal(countListeners(window, "DOMWindowClose"), windowListeners,
               "Should have removed a DOMWindowClose listener");
  assert.equal(countListeners(gBrowser, "TestEvent1"), 0, "Should be no listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 0, "Should be no listener for test event 2");

  loader.unload();
};

exports['test unload clears listeners'] = function(assert) {
  let window = getMostRecentBrowserWindow();
  let loader = Loader(module);

  
  let gBrowser = window.gBrowser;

  
  let windowListeners = countListeners(window, "DOMWindowClose");

  
  assert.equal(countListeners(gBrowser, "TestEvent1"), 0, "Should be no listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 0, "Should be no listener for test event 2");

  let { open } = loader.require('sdk/event/dom');

  open(gBrowser, "TestEvent1");
  assert.equal(countListeners(window, "DOMWindowClose"), windowListeners + 1,
               "Should have added a DOMWindowClose listener");
  assert.equal(countListeners(gBrowser, "TestEvent1"), 1, "Should be a listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 0, "Should be no listener for test event 2");

  open(gBrowser, "TestEvent2");
  assert.equal(countListeners(window, "DOMWindowClose"), windowListeners + 1,
               "Should not have added another DOMWindowClose listener");
  assert.equal(countListeners(gBrowser, "TestEvent1"), 1, "Should be a listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 1, "Should be a listener for test event 2");

  loader.unload();

  assert.equal(countListeners(window, "DOMWindowClose"), windowListeners,
               "Should have removed a DOMWindowClose listener");
  assert.equal(countListeners(gBrowser, "TestEvent1"), 0, "Should be no listener for test event 1");
  assert.equal(countListeners(gBrowser, "TestEvent2"), 0, "Should be no listener for test event 2");
};

require('sdk/test').run(exports);
