



"use strict";

const { Loader } = require("sdk/test/loader");
const utils = require("sdk/tabs/utils");
const { open, close } = require("sdk/window/helpers");
const { getMostRecentBrowserWindow } = require("sdk/window/utils");
const { events } = require("sdk/tab/events");
const { on, off } = require("sdk/event/core");
const { resolve } = require("sdk/core/promise");

let isFennec = require("sdk/system/xul-app").is("Fennec");

function test(scenario, currentWindow) {
  let useActiveWindow = isFennec || currentWindow;
  return function(assert, done) {
    let actual = [];
    function handler(event) actual.push(event)

    let win = useActiveWindow ? resolve(getMostRecentBrowserWindow()) :
              open(null, {
                features: { private: true, toolbar:true, chrome: true }
              });
    let window = null;

    win.then(function(w) {
      window = w;
      on(events, "data", handler);
      return scenario(assert, window, actual);
    }).then(function() {
      off(events, "data", handler);
      return useActiveWindow ? null : close(window);
    }).then(done, assert.fail);
  }
}

exports["test current window"] = test(function(assert, window, events) {
  
  
  let tab = utils.openTab(window, 'data:text/plain,open');
  utils.closeTab(tab);

  let [open, select, close] = events;

  assert.equal(open.type, "TabOpen");
  assert.equal(open.target, tab);

  assert.equal(select.type, "TabSelect");
  assert.equal(select.target, tab);

  assert.equal(close.type, "TabClose");
  assert.equal(close.target, tab);
});

exports["test open"] = test(function(assert, window, events) {
  let tab = utils.openTab(window, 'data:text/plain,open');
  let [open, select] = events;

  assert.equal(open.type, "TabOpen");
  assert.equal(open.target, tab);

  assert.equal(select.type, "TabSelect");
  assert.equal(select.target, tab);
});

exports["test open -> close"] = test(function(assert, window, events) {
  
  
  let _ = utils.openTab(window, 'daat:text/plain,ignore');
  let tab = utils.openTab(window, 'data:text/plain,open-close');
  utils.closeTab(tab);

  let [_open, _select, open, select, close] = events;

  assert.equal(open.type, "TabOpen");
  assert.equal(open.target, tab);

  assert.equal(select.type, "TabSelect");
  assert.equal(select.target, tab);

  assert.equal(close.type, "TabClose");
  assert.equal(close.target, tab);
});

exports["test open -> open -> select"] = test(function(assert, window, events) {
  let tab1 = utils.openTab(window, 'data:text/plain,Tab-1');
  let tab2 = utils.openTab(window, 'data:text/plain,Tab-2');
  utils.activateTab(tab1, window);

  let [open1, select1, open2, select2, select3] = events;

  
  assert.equal(open1.type, "TabOpen", "first tab opened")
  assert.equal(open1.target, tab1, "event.target is first tab")

  assert.equal(select1.type, "TabSelect", "first tab seleceted")
  assert.equal(select1.target, tab1, "event.target is first tab")


  
  assert.equal(open2.type, "TabOpen", "second tab opened");
  assert.equal(open2.target, tab2, "event.target is second tab");

  assert.equal(select2.type, "TabSelect", "second tab seleceted");
  assert.equal(select2.target, tab2, "event.target is second tab");

  
  assert.equal(select3.type, "TabSelect", "tab seleceted");
  assert.equal(select3.target, tab1, "event.target is first tab");
});

exports["test open -> pin -> unpin"] = test(function(assert, window, events) {
  let tab = utils.openTab(window, 'data:text/plain,pin-unpin');
  utils.pin(tab);
  utils.unpin(tab);

  let [open, select, move, pin, unpin] = events;

  assert.equal(open.type, "TabOpen");
  assert.equal(open.target, tab);

  assert.equal(select.type, "TabSelect");
  assert.equal(select.target, tab);

  if (isFennec) {
    assert.pass("Tab pin / unpin is not supported by Fennec");
  }
  else {
    assert.equal(move.type, "TabMove");
    assert.equal(move.target, tab);

    assert.equal(pin.type, "TabPinned");
    assert.equal(pin.target, tab);

    assert.equal(unpin.type, "TabUnpinned");
    assert.equal(unpin.target, tab);
  }
});

exports["test open -> open -> move "] = test(function(assert, window, events) {
  let tab1 = utils.openTab(window, 'data:text/plain,Tab-1');
  let tab2 = utils.openTab(window, 'data:text/plain,Tab-2');
  utils.move(tab1, 2);

  let [open1, select1, open2, select2, move] = events;

  
  assert.equal(open1.type, "TabOpen", "first tab opened");
  assert.equal(open1.target, tab1, "event.target is first tab");

  assert.equal(select1.type, "TabSelect", "first tab seleceted")
  assert.equal(select1.target, tab1, "event.target is first tab");


  
  assert.equal(open2.type, "TabOpen", "second tab opened");
  assert.equal(open2.target, tab2, "event.target is second tab");

  assert.equal(select2.type, "TabSelect", "second tab seleceted");
  assert.equal(select2.target, tab2, "event.target is second tab");

  if (isFennec) {
    assert.pass("Tab index changes not supported on Fennec yet")
  }
  else {
    
    assert.equal(move.type, "TabMove", "tab moved");
    assert.equal(move.target, tab1, "event.target is first tab");
  }
});

require("test").run(exports);
