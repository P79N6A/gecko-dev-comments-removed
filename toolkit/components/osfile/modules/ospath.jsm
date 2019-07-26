
















"use strict";

if (typeof Components == "undefined") {
  let Path;
  if (OS.Constants.Win) {
    Path = require("resource://gre/modules/osfile/ospath_win.jsm");
  } else {
    Path = require("resource://gre/modules/osfile/ospath_unix.jsm");
  }
  module.exports = Path;
} else {
  let Cu = Components.utils;
  let Scope = {};
  Cu.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", Scope);

  let Path = {};
  if (Scope.OS.Constants.Win) {
    Cu.import("resource://gre/modules/osfile/ospath_win.jsm", Path);
  } else {
    Cu.import("resource://gre/modules/osfile/ospath_unix.jsm", Path);
  }

  this.EXPORTED_SYMBOLS = [];
  for (let k in Path) {
    this.EXPORTED_SYMBOLS.push(k);
    this[k] = Path[k];
  }
}
