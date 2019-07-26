



const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const EXPORTED_SYMBOLS = ["CommonUtils"];

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/log4moz.js");

let CommonUtils = {
  exceptionStr: function exceptionStr(e) {
    let message = e.message ? e.message : e;
    return message + " " + CommonUtils.stackTrace(e);
  },

  stackTrace: function stackTrace(e) {
    
    if (e.location) {
      let frame = e.location;
      let output = [];
      while (frame) {
        
        
        
        let str = "<file:unknown>";

        let file = frame.filename || frame.fileName;
        if (file){
          str = file.replace(/^(?:chrome|file):.*?([^\/\.]+\.\w+)$/, "$1");
        }

        if (frame.lineNumber){
          str += ":" + frame.lineNumber;
        }
        if (frame.name){
          str = frame.name + "()@" + str;
        }

        if (str){
          output.push(str);
        }
        frame = frame.caller;
      }
      return "Stack trace: " + output.join(" < ");
    }
    
    if (e.stack){
      return "JS Stack trace: " + e.stack.trim().replace(/\n/g, " < ").
        replace(/@[^@]*?([^\/\.]+\.\w+:)/g, "@$1");
    }

    return "No traceback available";
  },

  








  encodeBase64URL: function encodeBase64URL(bytes, pad=true) {
    let s = btoa(bytes).replace("+", "-", "g").replace("/", "_", "g");

    if (!pad) {
      s = s.replace("=", "");
    }

    return s;
  },

  


  makeURI: function makeURI(URIString) {
    if (!URIString)
      return null;
    try {
      return Services.io.newURI(URIString, null, null);
    } catch (e) {
      let log = Log4Moz.repository.getLogger("Common.Utils");
      log.debug("Could not create URI: " + CommonUtils.exceptionStr(e));
      return null;
    }
  },

  







  nextTick: function nextTick(callback, thisObj) {
    if (thisObj) {
      callback = callback.bind(thisObj);
    }
    Services.tm.currentThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
  },

  





  waitForNextTick: function waitForNextTick() {
    let cb = Async.makeSyncCallback();
    this.nextTick(cb);
    Async.waitForSyncCallback(cb);

    return;
  },

  




  namedTimer: function namedTimer(callback, wait, thisObj, name) {
    if (!thisObj || !name) {
      throw "You must provide both an object and a property name for the timer!";
    }

    
    if (name in thisObj && thisObj[name] instanceof Ci.nsITimer) {
      thisObj[name].delay = wait;
      return;
    }

    
    let timer = {};
    timer.__proto__ = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    
    timer.clear = function() {
      thisObj[name] = null;
      timer.cancel();
    };

    
    timer.initWithCallback({
      notify: function notify() {
        
        timer.clear();
        callback.call(thisObj, timer);
      }
    }, wait, timer.TYPE_ONE_SHOT);

    return thisObj[name] = timer;
  },

  encodeUTF8: function encodeUTF8(str) {
    try {
      str = this._utf8Converter.ConvertFromUnicode(str);
      return str + this._utf8Converter.Finish();
    } catch (ex) {
      return null;
    }
  },

  decodeUTF8: function decodeUTF8(str) {
    try {
      str = this._utf8Converter.ConvertToUnicode(str);
      return str + this._utf8Converter.Finish();
    } catch (ex) {
      return null;
    }
  },

  byteArrayToString: function byteArrayToString(bytes) {
    return [String.fromCharCode(byte) for each (byte in bytes)].join("");
  },

  bytesAsHex: function bytesAsHex(bytes) {
    let hex = "";
    for (let i = 0; i < bytes.length; i++) {
      hex += ("0" + bytes[i].charCodeAt().toString(16)).slice(-2);
    }
    return hex;
  },

  


  encodeBase32: function encodeBase32(bytes) {
    const key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    let quanta = Math.floor(bytes.length / 5);
    let leftover = bytes.length % 5;

    
    if (leftover) {
      quanta += 1;
      for (let i = leftover; i < 5; i++)
        bytes += "\0";
    }

    
    
    let ret = "";
    for (let i = 0; i < bytes.length; i += 5) {
      let c = [byte.charCodeAt() for each (byte in bytes.slice(i, i + 5))];
      ret += key[c[0] >> 3]
           + key[((c[0] << 2) & 0x1f) | (c[1] >> 6)]
           + key[(c[1] >> 1) & 0x1f]
           + key[((c[1] << 4) & 0x1f) | (c[2] >> 4)]
           + key[((c[2] << 1) & 0x1f) | (c[3] >> 7)]
           + key[(c[3] >> 2) & 0x1f]
           + key[((c[3] << 3) & 0x1f) | (c[4] >> 5)]
           + key[c[4] & 0x1f];
    }

    switch (leftover) {
      case 1:
        return ret.slice(0, -6) + "======";
      case 2:
        return ret.slice(0, -4) + "====";
      case 3:
        return ret.slice(0, -3) + "===";
      case 4:
        return ret.slice(0, -1) + "=";
      default:
        return ret;
    }
  },

  


  decodeBase32: function decodeBase32(str) {
    const key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    let padChar = str.indexOf("=");
    let chars = (padChar == -1) ? str.length : padChar;
    let bytes = Math.floor(chars * 5 / 8);
    let blocks = Math.ceil(chars / 8);

    
    
    
    function processBlock(ret, cOffset, rOffset) {
      let c, val;

      
      
      function accumulate(val) {
        ret[rOffset] |= val;
      }

      function advance() {
        c  = str[cOffset++];
        if (!c || c == "" || c == "=") 
          throw "Done";                
        val = key.indexOf(c);
        if (val == -1)
          throw "Unknown character in base32: " + c;
      }

      
      function left(octet, shift)
        (octet << shift) & 0xff;

      advance();
      accumulate(left(val, 3));
      advance();
      accumulate(val >> 2);
      ++rOffset;
      accumulate(left(val, 6));
      advance();
      accumulate(left(val, 1));
      advance();
      accumulate(val >> 4);
      ++rOffset;
      accumulate(left(val, 4));
      advance();
      accumulate(val >> 1);
      ++rOffset;
      accumulate(left(val, 7));
      advance();
      accumulate(left(val, 2));
      advance();
      accumulate(val >> 3);
      ++rOffset;
      accumulate(left(val, 5));
      advance();
      accumulate(val);
      ++rOffset;
    }

    
    let ret  = new Array(bytes);
    let i    = 0;
    let cOff = 0;
    let rOff = 0;

    for (; i < blocks; ++i) {
      try {
        processBlock(ret, cOff, rOff);
      } catch (ex) {
        
        if (ex == "Done")
          break;
        throw ex;
      }
      cOff += 8;
      rOff += 5;
    }

    
    return CommonUtils.byteArrayToString(ret.slice(0, bytes));
  },

  




  safeAtoB: function safeAtoB(b64) {
    let len = b64.length;
    let over = len % 4;
    return over ? atob(b64.substr(0, len - over)) : atob(b64);
  },

  












  jsonLoad: function jsonLoad(filePath, that, callback) {
    let path = filePath + ".json";

    if (that._log) {
      that._log.trace("Loading json from disk: " + filePath);
    }

    let file = FileUtils.getFile("ProfD", path.split("/"), true);
    if (!file.exists()) {
      callback.call(that);
      return;
    }

    let channel = NetUtil.newChannel(file);
    channel.contentType = "application/json";

    NetUtil.asyncFetch(channel, function (is, result) {
      if (!Components.isSuccessCode(result)) {
        callback.call(that);
        return;
      }
      let string = NetUtil.readInputStreamToString(is, is.available());
      is.close();
      let json;
      try {
        json = JSON.parse(string);
      } catch (ex) {
        if (that._log) {
          that._log.debug("Failed to load json: " +
                          CommonUtils.exceptionStr(ex));
        }
      }
      callback.call(that, json);
    });
  },

  















  jsonSave: function jsonSave(filePath, that, obj, callback) {
    let path = filePath + ".json";
    if (that._log) {
      that._log.trace("Saving json to disk: " + path);
    }

    let file = FileUtils.getFile("ProfD", path.split("/"), true);
    let json = typeof obj == "function" ? obj.call(that) : obj;
    let out = JSON.stringify(json);

    let fos = FileUtils.openSafeFileOutputStream(file);
    let is = this._utf8Converter.convertToInputStream(out);
    NetUtil.asyncCopy(is, fos, function (result) {
      if (typeof callback == "function") {
        let error = (result == Cr.NS_OK) ? null : result;
        callback.call(that, error);
      }
    });
  },

  











  ensureMillisecondsTimestamp: function ensureMillisecondsTimestamp(value) {
    if (!value) {
      return;
    }

    if (!/^[0-9]+$/.test(value)) {
      throw new Error("Timestamp value is not a positive integer: " + value);
    }

    let intValue = parseInt(value, 10);

    if (!intValue) {
       return;
    }

    
    if (intValue < 10000000000) {
      throw new Error("Timestamp appears to be in seconds: " + intValue);
    }
  },

  








  readBytesFromInputStream: function readBytesFromInputStream(stream, count) {
    let BinaryInputStream = Components.Constructor(
        "@mozilla.org/binaryinputstream;1",
        "nsIBinaryInputStream",
        "setInputStream");
    if (!count) {
      count = stream.available();
    }

    return new BinaryInputStream(stream).readBytes(count);
  },
};

XPCOMUtils.defineLazyGetter(CommonUtils, "_utf8Converter", function() {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  return converter;
});
