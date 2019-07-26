




"use strict";

module.metadata = {
  "stability": "unstable"
};

let { Cc, Ci, CC } = require('chrome');
let { PlainTextConsole } = require('../console/plain-text');
let { stdout } = require('../system');
let ScriptError = CC('@mozilla.org/scripterror;1', 'nsIScriptError');
let consoleService = Cc['@mozilla.org/consoleservice;1'].getService().
                     QueryInterface(Ci.nsIConsoleService);





exports.dump = stdout.write;


function forsakenConsoleDump(msg, level) {
  stdout.write(msg);

  if (level === 'error') {
    let error = ScriptError();
    msg = msg.replace(/^error: /, '');
    error.init(msg, null, null, 0, 0, 0, 'Add-on SDK');
    consoleService.logMessage(error);
  }
  else
    consoleService.logStringMessage(msg);
};
exports.console = new PlainTextConsole(forsakenConsoleDump);



Object.defineProperty(exports, 'define', {
  
  
  
  configurable: true,
  get: function() {
    let sandbox = this;
    return function define(factory) {
      factory = Array.slice(arguments).pop();
      factory.call(sandbox, sandbox.require, sandbox.exports, sandbox.module);
    }
  }
});
