



"use strict";

const { Ci, Cc, Cu, Cr, CC } = require("chrome");
const Services = require("Services");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const { dumpv } = DevToolsUtils;

DevToolsUtils.defineLazyGetter(this, "IOUtil", () => {
  return Cc["@mozilla.org/io-util;1"].getService(Ci.nsIIOUtil);
});

DevToolsUtils.defineLazyGetter(this, "ScriptableInputStream", () => {
  return CC("@mozilla.org/scriptableinputstream;1",
            "nsIScriptableInputStream", "init");
});

const BUFFER_SIZE = 0x8000;


































function copyStream(input, output, length) {
  let copier = new StreamCopier(input, output, length);
  return copier.copy();
}

function StreamCopier(input, output, length) {
  this._id = StreamCopier._nextId++;
  this.input = input;
  
  this.baseAsyncOutput = output;
  if (IOUtil.outputStreamIsBuffered(output)) {
    this.output = output;
  } else {
    this.output = Cc["@mozilla.org/network/buffered-output-stream;1"].
                  createInstance(Ci.nsIBufferedOutputStream);
    this.output.init(output, BUFFER_SIZE);
  }
  this._amountLeft = length;
  this._deferred = promise.defer();

  this._copy = this._copy.bind(this);
  this._flush = this._flush.bind(this);
  this._destroy = this._destroy.bind(this);
  this._deferred.promise.then(this._destroy, this._destroy);

  
  
  this._streamReadyCallback = this._copy;
}
StreamCopier._nextId = 0;

StreamCopier.prototype = {

  get copied() { return this._deferred.promise; },

  copy: function() {
    try {
      this._copy();
    } catch(e) {
      this._deferred.reject(e);
    }
    return this.copied;
  },

  _copy: function() {
    let bytesAvailable = this.input.available();
    let amountToCopy = Math.min(bytesAvailable, this._amountLeft);
    this._debug("Trying to copy: " + amountToCopy);

    let bytesCopied;
    try {
      bytesCopied = this.output.writeFrom(this.input, amountToCopy);
    } catch(e if e.result == Cr.NS_BASE_STREAM_WOULD_BLOCK) {
      this._debug("Base stream would block, will retry");
      this._debug("Waiting for output stream");
      this.baseAsyncOutput.asyncWait(this, 0, 0, Services.tm.currentThread);
      return;
    }

    this._amountLeft -= bytesCopied;
    this._debug("Copied: " + bytesCopied +
                ", Left: " + this._amountLeft);

    if (this._amountLeft === 0) {
      this._debug("Copy done!");
      this._flush();
      return;
    }

    this._debug("Waiting for input stream");
    this.input.asyncWait(this, 0, 0, Services.tm.currentThread);
  },

  _flush: function() {
    try {
      this.output.flush();
    } catch(e if e.result == Cr.NS_BASE_STREAM_WOULD_BLOCK ||
                 e.result == Cr.NS_ERROR_FAILURE) {
      this._debug("Flush would block, will retry");
      this._streamReadyCallback = this._flush;
      this._debug("Waiting for output stream");
      this.baseAsyncOutput.asyncWait(this, 0, 0, Services.tm.currentThread);
      return;
    }
    this._deferred.resolve();
  },

  _destroy: function() {
    this._destroy = null;
    this._copy = null;
    this._flush = null;
    this.input = null;
    this.output = null;
  },

  
  onInputStreamReady: function() {
    this._streamReadyCallback();
  },

  
  onOutputStreamReady: function() {
    this._streamReadyCallback();
  },

  _debug: function(msg) {
    
    
    dumpv("Copier: " + this._id + " " + msg);
  }

};



















function delimitedRead(stream, delimiter, count) {
  dumpv("Starting delimited read for " + delimiter + " up to " +
        count + " bytes");

  let scriptableStream;
  if (stream instanceof Ci.nsIScriptableInputStream) {
    scriptableStream = stream;
  } else {
    scriptableStream = new ScriptableInputStream(stream);
  }

  let data = "";

  
  count = Math.min(count, stream.available());

  if (count <= 0) {
    return data;
  }

  let char;
  while (char !== delimiter && count > 0) {
    char = scriptableStream.readBytes(1);
    count--;
    data += char;
  }

  return data;
}

module.exports = {
  copyStream: copyStream,
  delimitedRead: delimitedRead
};
