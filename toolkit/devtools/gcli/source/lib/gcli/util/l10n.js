















'use strict';

var Cu = require('chrome').Cu;

var XPCOMUtils = Cu.import('resource://gre/modules/XPCOMUtils.jsm', {}).XPCOMUtils;
var Services = Cu.import('resource://gre/modules/Services.jsm', {}).Services;

var imports = {};
XPCOMUtils.defineLazyGetter(imports, 'stringBundle', function () {
  return Services.strings.createBundle('chrome://browser/locale/devtools/gcli.properties');
});





exports.registerStringsSource = function(modulePath) {
  throw new Error('registerStringsSource is not available in mozilla');
};

exports.unregisterStringsSource = function(modulePath) {
  throw new Error('unregisterStringsSource is not available in mozilla');
};

exports.lookupSwap = function(key, swaps) {
  throw new Error('lookupSwap is not available in mozilla');
};

exports.lookupPlural = function(key, ord, swaps) {
  throw new Error('lookupPlural is not available in mozilla');
};

exports.getPreferredLocales = function() {
  return [ 'root' ];
};


exports.lookup = function(key) {
  try {
    
    
    
    





    return imports.stringBundle.GetStringFromName(key);
  }
  catch (ex) {
    console.error('Failed to lookup ', key, ex);
    return key;
  }
};


exports.propertyLookup = Proxy.create({
  get: function(rcvr, name) {
    return exports.lookup(name);
  }
});


exports.lookupFormat = function(key, swaps) {
  try {
    return imports.stringBundle.formatStringFromName(key, swaps, swaps.length);
  }
  catch (ex) {
    console.error('Failed to format ', key, ex);
    return key;
  }
};
