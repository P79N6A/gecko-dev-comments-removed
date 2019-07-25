



































 




var EXPORTED_SYMBOLS = ["Logger"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;

var Logger =
{
  _foStream: null,
  _converter: null,
  _potentialError: null,

  init: function (path) {
    if (this._converter != null) {
      
      return;
    }

    let prefs = CC["@mozilla.org/preferences-service;1"]
                .getService(CI.nsIPrefBranch);
    if (path) {
      prefs.setCharPref("tps.logfile", path);
    }
    else {
      path = prefs.getCharPref("tps.logfile");
    }

    this._file = CC["@mozilla.org/file/local;1"]
                 .createInstance(CI.nsILocalFile);
    this._file.initWithPath(path);
    var exists = this._file.exists();

    
    this._foStream = CC["@mozilla.org/network/file-output-stream;1"]
                     .createInstance(CI.nsIFileOutputStream);
    
    var fileflags = exists ? 0x02 | 0x08 | 0x10 : 0x02 | 0x08 | 0x20;

    this._foStream.init(this._file, fileflags, 0666, 0);
    this._converter = CC["@mozilla.org/intl/converter-output-stream;1"]
                      .createInstance(CI.nsIConverterOutputStream);
    this._converter.init(this._foStream, "UTF-8", 0, 0);
  },

  write: function (data) {
    if (this._converter == null) {
      CU.reportError(
          "TPS Logger.write called with _converter == null!");
      return;
    }
    this._converter.writeString(data);
  },

  close: function () {
    if (this._converter != null) {
      this._converter.close();
      this._converter = null;
      this._foStream = null;
    }
  },

  AssertTrue: function(bool, msg, showPotentialError) {
    if (!bool) {
      let message = msg;
      if (showPotentialError && this._potentialError) {
        message += "; " + this._potentialError;
        this._potentialError = null;
      }
      throw("ASSERTION FAILED! " + message);
    }
  },

  AssertEqual: function(val1, val2, msg) {
    if (val1 != val2)
      throw("ASSERTION FAILED! " + msg + "; expected " + 
            JSON.stringify(val2) + ", got " + JSON.stringify(val1));
  },

  log: function (msg, withoutPrefix) {
    dump(msg + "\n");
    if (withoutPrefix) {
      this.write(msg + "\n");
    }
    else {
      var now = new Date()
      this.write(now.getFullYear() + "-" + (now.getMonth() < 9 ? '0' : '') + 
          (now.getMonth() + 1) + "-" + 
          (now.getDate() < 9 ? '0' : '') + (now.getDate() + 1) + " " +
          (now.getHours() < 10 ? '0' : '') + now.getHours() + ":" +
          (now.getMinutes() < 10 ? '0' : '') + now.getMinutes() + ":" +
          (now.getSeconds() < 10 ? '0' : '') + now.getSeconds() + " " + 
          msg + "\n");
    }
  },

  clearPotentialError: function() {
    this._potentialError = null;
  },

  logPotentialError: function(msg) {
    this._potentialError = msg;
  },

  logLastPotentialError: function(msg) {
    var message = msg;
    if (this._potentialError) {
      message = this._poentialError;
      this._potentialError = null;
    }
    this.log("CROSSWEAVE ERROR: " + message);
  },

  logError: function (msg) {
    this.log("CROSSWEAVE ERROR: " + msg);
  },

  logInfo: function (msg, withoutPrefix) {
    if (withoutPrefix)
      this.log(msg, true);
    else
      this.log("CROSSWEAVE INFO: " + msg);
  },

  logPass: function (msg) {
    this.log("CROSSWEAVE TEST PASS: " + msg);
  },
};

