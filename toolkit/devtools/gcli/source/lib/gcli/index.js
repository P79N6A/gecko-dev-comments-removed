















'use strict';

var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;


var prefSvc = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefService);
var prefBranch = prefSvc.getBranch(null).QueryInterface(Ci.nsIPrefBranch2);

exports.hiddenByChromePref = function() {
  return !prefBranch.prefHasUserValue('devtools.chrome.enabled');
};
