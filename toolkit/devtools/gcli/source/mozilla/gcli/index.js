















'use strict';

var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;
var Cu = require('chrome').Cu;














var items = [
  require('./types/delegate').items,
  require('./types/selection').items,
  require('./types/array').items,

  require('./types/boolean').items,
  require('./types/command').items,
  require('./types/date').items,
  require('./types/file').items,
  require('./types/javascript').items,
  require('./types/node').items,
  require('./types/number').items,
  require('./types/resource').items,
  require('./types/setting').items,
  require('./types/string').items,

  require('./fields/delegate').items,
  require('./fields/selection').items,

  require('./ui/focus').items,
  require('./ui/intro').items,

  require('./converters/converters').items,
  require('./converters/basic').items,
  
  require('./converters/terminal').items,

  require('./languages/command').items,
  require('./languages/javascript').items,

  
  
  
  

  
  require('./commands/clear').items,
  
  require('./commands/context').items,
  
  require('./commands/global').items,
  require('./commands/help').items,
  
  require('./commands/lang').items,
  
  require('./commands/pref').items,
  
  

  

].reduce(function(prev, curr) { return prev.concat(curr); }, []);

var api = require('./api');
api.populateApi(exports);
exports.addItems(items);

var host = require('./util/host');

exports.useTarget = host.script.useTarget;















exports.createDisplay = function(opts) {
  var FFDisplay = require('./mozui/ffdisplay').FFDisplay;
  return new FFDisplay(opts);
};

var prefSvc = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefService);
var prefBranch = prefSvc.getBranch(null).QueryInterface(Ci.nsIPrefBranch2);

exports.hiddenByChromePref = function() {
  return !prefBranch.prefHasUserValue('devtools.chrome.enabled');
};


try {
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

  


  exports.lookupFormat = function(name, swaps) {
    try {
      return stringBundle.formatStringFromName(name, swaps, swaps.length);
    }
    catch (ex) {
      throw new Error('Failure in lookupFormat(\'' + name + '\')');
    }
  };
}
catch (ex) {
  console.error('Using string fallbacks', ex);

  exports.lookup = function(name) {
    return name;
  };
  exports.lookupFormat = function(name, swaps) {
    return name;
  };
}
