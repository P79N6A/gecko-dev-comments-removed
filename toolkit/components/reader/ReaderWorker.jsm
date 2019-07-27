



"use strict";





const Cu = Components.utils;

Cu.import("resource://gre/modules/PromiseWorker.jsm", this);

this.EXPORTED_SYMBOLS = ["ReaderWorker"];

this.ReaderWorker = new BasePromiseWorker("resource://gre/modules/reader/ReaderWorker.js");
