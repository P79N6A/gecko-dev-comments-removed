















var EXPORTED_SYMBOLS = ['SpecialInflate', 'SpecialInflateUtils'];

Components.utils.import('resource://gre/modules/Services.jsm');

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

function SimpleStreamListener() {
  this.binaryStream = Cc['@mozilla.org/binaryinputstream;1']
    .createInstance(Ci.nsIBinaryInputStream);
  this.onData = null;
  this.buffer = null;
}
SimpleStreamListener.prototype = {
  QueryInterface: function (iid) {
    if (iid.equals(Ci.nsIStreamListener) ||
      iid.equals(Ci.nsIRequestObserver) ||
      iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  onStartRequest: function (aRequest, aContext) {
    return Cr.NS_OK;
  },
  onStopRequest: function (aRequest, aContext, sStatusCode) {
    return Cr.NS_OK;
  },
  onDataAvailable: function (aRequest, aContext, aInputStream, aOffset, aCount) {
    this.binaryStream.setInputStream(aInputStream);
    if (!this.buffer || aCount > this.buffer.byteLength) {
      this.buffer = new ArrayBuffer(aCount);
    }
    this.binaryStream.readArrayBuffer(aCount, this.buffer);
    this.onData(new Uint8Array(this.buffer, 0, aCount));
    return Cr.NS_OK;
  }
};

function SpecialInflate() {
  var listener = new SimpleStreamListener();
  listener.onData = function (data) {
    this.onData(data);
  }.bind(this);

  var converterService = Cc["@mozilla.org/streamConverters;1"].getService(Ci.nsIStreamConverterService);
  var converter = converterService.asyncConvertData("deflate", "uncompressed", listener, null);
  converter.onStartRequest(null, null);
  this.converter = converter;

  var binaryStream = Cc["@mozilla.org/binaryoutputstream;1"].createInstance(Ci.nsIBinaryOutputStream);
  var pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
  pipe.init(true, true, 0, 0xFFFFFFFF, null);
  binaryStream.setOutputStream(pipe.outputStream);
  this.binaryStream = binaryStream;

  this.pipeInputStream = pipe.inputStream;

  this.onData = null;
}
SpecialInflate.prototype = {
  push: function (data) {
    this.binaryStream.writeByteArray(data, data.length);
    this.converter.onDataAvailable(null, null, this.pipeInputStream, 0, data.length);
  },
  close: function () {
    this.binaryStream.close();
    this.converter.onStopRequest(null, null, Cr.NS_OK);
  }
};

var SpecialInflateUtils = {
  get isSpecialInflateEnabled() {
    try {
      return Services.prefs.getBoolPref('shumway.specialInflate');
    } catch (ex) {
      return false; 
    }
  },

  createWrappedSpecialInflate: function (sandbox) {
    function genPropDesc(value) {
      return {
        enumerable: true, configurable: true, writable: true, value: value
      };
    }

    var wrapped = new SpecialInflate();
    wrapped.onData = function(data) {
      if (wrapper.onData) {
        wrapper.onData.call(wrapper, Components.utils.cloneInto(data, sandbox));
      }
    };

    
    
    
    var wrapper = Components.utils.createObjectIn(sandbox);
    Object.defineProperties(wrapper, {
      onData: genPropDesc(null),

      push: genPropDesc(function (data) {
        
        
        return wrapped.push(data);
      }),
      close: genPropDesc(function () {
        return wrapped.close();
      })
    });
    Components.utils.makeObjectPropsNormal(wrapper);
    return wrapper;
  }
};
