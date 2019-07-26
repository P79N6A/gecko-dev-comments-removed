




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

exports.console = new PlainTextConsole();



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
