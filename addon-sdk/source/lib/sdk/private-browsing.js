


'use strict';

module.metadata = {
  "stability": "stable"
};

const { setMode, getMode, on: onStateChange } = require('./private-browsing/utils');
const { isWindowPrivate } = require('./window/utils');
const { emit, on, once, off } = require('./event/core');
const { when: unload } = require('./system/unload');
const { deprecateUsage, deprecateFunction, deprecateEvent } = require('./util/deprecate');
const { getOwnerWindow } = require('./private-browsing/window/utils');

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

exports.isPrivate = function(thing) {
  
  
  if (!!thing) {
    
    
    if (isWindowPrivate(thing)) {
      return true;
    }

    
    
    if (thing.tab) {
      let tabWindow = getOwnerWindow(thing.tab);
      if (tabWindow) {
        let isThingPrivate = isWindowPrivate(tabWindow);
        if (isThingPrivate)
          return isThingPrivate;
      }
    }

    
    let window = getOwnerWindow(thing);
    if (window)
      return isWindowPrivate(window);
  }

  
  
  
  return getMode();
};

function deprecateEvents(func) deprecateEvent(
  func,
   'The require("private-browsing") module\'s "start" and "stop" events are deprecated.',
  ['start', 'stop']
);


unload(function() off(exports));
