















"use strict";

this.EXPORTED_SYMBOLS = [ "gcli", "Requisition" ];

let require = Components.utils.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;

this.gcli = require('gcli/index');
this.Requisition = require('gcli/cli').Requisition;
