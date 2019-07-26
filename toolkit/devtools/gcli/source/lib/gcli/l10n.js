















'use strict';

var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;
var Cu = require('chrome').Cu;

var prefSvc = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefService);
var prefBranch = prefSvc.getBranch(null).QueryInterface(Ci.nsIPrefBranch2);

var Services = Cu.import('resource://gre/modules/Services.jsm', {}).Services;
var stringBundle = Services.strings.createBundle(
        'chrome://browser/locale/devtools/gclicommands.properties');




exports.lookup = function(name) {
  try {
    return stringBundle.GetStringFromName(name);
  }
  catch (ex) {
    throw new Error('Failure in lookup(\'' + name + '\')');
  }
};








exports.propertyLookup = Proxy.create({
  get: function(rcvr, name) {
    return exports.lookup(name);
  }
});




exports.lookupFormat = function(name, swaps) {
  try {
    return stringBundle.formatStringFromName(name, swaps, swaps.length);
  }
  catch (ex) {
    throw new Error('Failure in lookupFormat(\'' + name + '\')');
  }
};










exports.hiddenByChromePref = function() {
  return !prefBranch.prefHasUserValue('devtools.chrome.enabled');
};
