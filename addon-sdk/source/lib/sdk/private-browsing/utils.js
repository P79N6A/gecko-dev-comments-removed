


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
const { isOneOf, is, satisfiesVersion, version } = require('../system/xul-app');

let deferredEmit = defer(emit);
let pbService;
let PrivateBrowsingUtils;


if (isOneOf(['Firefox', 'Fennec'])) {
  
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
  if (!PrivateBrowsingUtils || !win)
    return false;

  
  
  
  if (win instanceof Ci.nsIDOMWindow) {
    return PrivateBrowsingUtils.isWindowPrivate(win);
  }

  
  try {
    return !!win.docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing;
  }
  catch (e) {}

  return false;
}
exports.isWindowPrivate = isWindowPrivate;


let isGlobalPBSupported = exports.isGlobalPBSupported = !!pbService && is('Firefox');


let isWindowPBSupported = exports.isWindowPBSupported =
                          !pbService && !!PrivateBrowsingUtils && is('Firefox');


let isTabPBSupported = exports.isTabPBSupported =
                       !pbService && !!PrivateBrowsingUtils && is('Fennec') && satisfiesVersion(version, '>=20.0*');

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
