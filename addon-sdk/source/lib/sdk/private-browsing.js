


'use strict';

module.metadata = {
  "stability": "stable"
};

const { setMode, getMode, on: onStateChange, isPrivate } = require('./private-browsing/utils');
const { emit, on, once, off } = require('./event/core');
const { when: unload } = require('./system/unload');
const { deprecateFunction, deprecateEvent } = require('./util/deprecate');

onStateChange('start', function onStart() {
  emit(exports, 'start');
});

onStateChange('stop', function onStop() {
  emit(exports, 'stop');
});

Object.defineProperty(exports, "isActive", {
  get: deprecateFunction(getMode, 'require("private-browsing").isActive is deprecated.')
});

exports.activate = function activate() setMode(true);
exports.deactivate = function deactivate() setMode(false);

exports.on = deprecateEvents(on.bind(null, exports));
exports.once = deprecateEvents(once.bind(null, exports));
exports.removeListener = deprecateEvents(function removeListener(type, listener) {
  
  
  
  off(exports, type, listener);
});

exports.isPrivate = isPrivate;

function deprecateEvents(func) deprecateEvent(
  func,
   'The require("sdk/private-browsing") module\'s "start" and "stop" events ' +
   'are deprecated.',
  ['start', 'stop']
);


unload(function() off(exports));
