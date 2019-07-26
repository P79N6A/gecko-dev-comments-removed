


'use strict';

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci, Cu } = require('chrome');
const { defer } = require('../lang/functional');
const { emit, on, once, off } = require('../event/core');
const { when: unload } = require('../system/unload');
const { getWindowLoadingContext, windows } = require('../window/utils');
const { WindowTracker } = require("../deprecated/window-utils");
const events = require('../system/events');
const { deprecateFunction } = require('../util/deprecate');

let deferredEmit = defer(emit);
let pbService;
let PrivateBrowsingUtils;


if (require("../system/xul-app").is("Firefox")) {
  
  try {
    pbService = Cc["@mozilla.org/privatebrowsing;1"].
                getService(Ci.nsIPrivateBrowsingService);

    
    
    
    if (!('privateBrowsingEnabled' in pbService))
      pbService = undefined;
  } catch(e) {  }

  try {
    PrivateBrowsingUtils = Cu.import('resource://gre/modules/PrivateBrowsingUtils.jsm', {}).PrivateBrowsingUtils;
  }
  catch(e) {  }
}

function isWindowPrivate(win) {
  
  
  
  return win instanceof Ci.nsIDOMWindow &&
         isWindowPBSupported &&
         PrivateBrowsingUtils.isWindowPrivate(win);
}
exports.isWindowPrivate = isWindowPrivate;


let isGlobalPBSupported = exports.isGlobalPBSupported =  !!pbService;


let isWindowPBSupported = exports.isWindowPBSupported = !isGlobalPBSupported && !!PrivateBrowsingUtils;

function onChange() {
  
  deferredEmit(exports, pbService.privateBrowsingEnabled ? 'start' : 'stop');
}


if (isGlobalPBSupported) {
  
  events.on('private-browsing-transition-complete', onChange);
}




let setMode = defer(function setMode(value) {
  value = !!value;  

  
  return pbService && (pbService.privateBrowsingEnabled = value);
});
exports.setMode = deprecateFunction(
  setMode,
  'require("private-browsing").activate and require("private-browsing").deactivate ' +
  'is deprecated.'
);

let getMode = function getMode(chromeWin) {
  if (isWindowPrivate(chromeWin))
    return true;

  
  return pbService ? pbService.privateBrowsingEnabled : false;
};
exports.getMode = getMode;

exports.on = on.bind(null, exports);


unload(function() off(exports));
