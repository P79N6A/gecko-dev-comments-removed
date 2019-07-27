



"use strict";





const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/PromiseWorker.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);

this.EXPORTED_SYMBOLS = ["SessionWorker"];

this.SessionWorker = new BasePromiseWorker("resource:///modules/sessionstore/SessionWorker.js");


this.SessionWorker.ExceptionHandlers["OS.File.Error"] = OS.File.Error.fromMsg;

