


'use strict';

const { defer } = require('../core/promise');
const events = require('../system/events');
const { setImmediate } = require('../timers');
const { open: openWindow, onFocus, getToplevelWindow,
        isInteractive } = require('./utils');

function open(uri, options) {
  return promise(openWindow.apply(null, arguments), 'load');
}
exports.open = open;

function close(window) {
  
  
  
  
  
  
  
  let deferred = defer();
  let toplevelWindow = getToplevelWindow(window);
  events.on("domwindowclosed", function onclose({subject}) {
    if (subject == toplevelWindow) {
      events.off("domwindowclosed", onclose);
      setImmediate(function() deferred.resolve(window));
    }
  }, true);
  window.close();
  return deferred.promise;
}
exports.close = close;

function focus(window) {
  let p = onFocus(window);
  window.focus();
  return p;
}
exports.focus = focus;

function ready(window) {
  let { promise: result, resolve } = defer();

  if (isInteractive(window))
    resolve(window);
  else
    resolve(promise(window, 'DOMContentLoaded'));

  return result;
}
exports.ready = ready;

function promise(target, evt, capture) {
  let deferred = defer();
  capture = !!capture;

  target.addEventListener(evt, function eventHandler() {
    target.removeEventListener(evt, eventHandler, capture);
    deferred.resolve(target);
  }, capture);

  return deferred.promise;
}
exports.promise = promise;
