















var EXPORTED_SYMBOLS = ['RtmpUtils'];

Components.utils.import('resource://gre/modules/Services.jsm');

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

var RtmpUtils = {
  get isRtmpEnabled() {
    try {
      return Services.prefs.getBoolPref('shumway.rtmp.enabled');
    } catch (ex) {
      return false;
    }
  },

  createSocket: function (sandbox, params) {
    var host = params.host, port = params.port, ssl = params.ssl;

    var baseSocket = Cc["@mozilla.org/tcp-socket;1"].createInstance(Ci.nsIDOMTCPSocket);
    var socket = baseSocket.open(host, port, {useSecureTransport: ssl, binaryType: 'arraybuffer'});
    if (!socket) {
      return null;
    }

    socket.onopen = function () {
      if (wrapperOnOpen) {
        wrapperOnOpen(new sandbox.Object());
      }
    };
    socket.ondata = function (e) {
      if (wrapperOnData) {
        var wrappedE = new sandbox.Object();
        wrappedE.data = Components.utils.cloneInto(e.data, sandbox);
        wrapperOnData(wrappedE);
      }
    };
    socket.ondrain = function () {
      if (wrapperOnDrain) {
        wrapperOnDrain(new sandbox.Object());
      }
    };
    socket.onerror = function (e) {
      if (wrapperOnError) {
        var wrappedE = new sandbox.Object();
        wrappedE.data = Components.utils.cloneInto(e.data, sandbox);
        wrapperOnError(wrappedE);
      }
    };
    socket.onclose = function () {
      if (wrapperOnClose) {
        wrapperOnClose(new sandbox.Object());
      }
    };

    var wrapperOnOpen, wrapperOnData, wrapperOnDrain, wrapperOnError, wrapperOnClose;
    var wrapper = Components.utils.cloneInto({
      setOpenCallback: function (callback) {
        wrapperOnOpen = callback;
      },
      setDataCallback: function (callback) {
        wrapperOnData = callback;
      },
      setDrainCallback: function (callback) {
        wrapperOnDrain = callback;
      },
      setErrorCallback: function (callback) {
        wrapperOnError = callback;
      },
      setCloseCallback: function (callback) {
        wrapperOnClose = callback;
      },

      send: function (buffer, offset, count) {
        return socket.send(buffer, offset, count);
      },

      close: function () {
        socket.close();
      }
    }, sandbox, {cloneFunctions:true});
    return wrapper;
  },

  createXHR: function (sandbox) {
    var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);

    xhr.onload = function () {
      wrapper.status = xhr.status;
      wrapper.response = Components.utils.cloneInto(xhr.response, sandbox);
      if (wrapperOnLoad) {
        wrapperOnLoad(new sandbox.Object());
      }
    };
    xhr.onerror = function () {
      wrapper.status = xhr.status;
      if (wrapperOnError) {
        wrapperOnError(new sandbox.Object());
      }
    };

    var wrapperOnLoad, wrapperOnError;
    var wrapper = Components.utils.cloneInto({
      status: 0,
      response: undefined,
      responseType: 'text',

      setLoadCallback: function (callback) {
        wrapperOnLoad = callback;
      },
      setErrorCallback: function (callback) {
        wrapperOnError = callback;
      },

      open: function (method, path, async) {
        if (method !== 'POST' || !path || (async !== undefined && !async)) {
          throw new Error('invalid open() arguments');
        }
        
        xhr.open('POST', path, true);
        xhr.responseType = 'arraybuffer';
        xhr.setRequestHeader('Content-Type', 'application/x-fcs');
      },

      setRequestHeader: function (header, value) {
        if (header !== 'Content-Type' || value !== 'application/x-fcs') {
          throw new Error('invalid setRequestHeader() arguments');
        }
      },

      send: function (data) {
        if (wrapper.responseType !== 'arraybuffer') {
          throw new Error('Invalid responseType.');
        }
        xhr.send(data);
      }
    }, sandbox, {cloneFunctions:true});
    return wrapper;
  }
};
