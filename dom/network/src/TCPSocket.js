



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;
const CC = Components.Constructor;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const InputStreamPump = CC(
        "@mozilla.org/network/input-stream-pump;1", "nsIInputStreamPump", "init"),
      AsyncStreamCopier = CC(
        "@mozilla.org/network/async-stream-copier;1", "nsIAsyncStreamCopier", "init"),
      ScriptableInputStream = CC(
        "@mozilla.org/scriptableinputstream;1", "nsIScriptableInputStream", "init"),
      BinaryInputStream = CC(
        "@mozilla.org/binaryinputstream;1", "nsIBinaryInputStream", "setInputStream"),
      StringInputStream = CC(
        '@mozilla.org/io/string-input-stream;1', 'nsIStringInputStream'),
      ArrayBufferInputStream = CC(
        '@mozilla.org/io/arraybuffer-input-stream;1', 'nsIArrayBufferInputStream'),
      MultiplexInputStream = CC(
        '@mozilla.org/io/multiplex-input-stream;1', 'nsIMultiplexInputStream');

const kCONNECTING = 'connecting';
const kOPEN = 'open';
const kCLOSING = 'closing';
const kCLOSED = 'closed';

const BUFFER_SIZE = 65536;





let debug = true;
function LOG(msg) {
  if (debug)
    dump("TCPSocket: " + msg + "\n");
}





function TCPSocketEvent(type, sock, data) {
  this._type = type;
  this._target = sock;
  this._data = data;
}

TCPSocketEvent.prototype = {
  __exposedProps__: {
    type: 'r',
    target: 'r',
    data: 'r'
  },
  get type() {
    return this._type;
  },
  get target() {
    return this._target;
  },
  get data() {
    return this._data;
  }
}





function TCPSocket() {
  this._readyState = kCLOSED;

  this._onopen = null;
  this._ondrain = null;
  this._ondata = null;
  this._onerror = null;
  this._onclose = null;

  this._binaryType = "string";

  this._host = "";
  this._port = 0;
  this._ssl = false;

  this.useWin = null;
}

TCPSocket.prototype = {
  __exposedProps__: {
    open: 'r',
    host: 'r',
    port: 'r',
    ssl: 'r',
    bufferedAmount: 'r',
    suspend: 'r',
    resume: 'r',
    close: 'r',
    send: 'r',
    readyState: 'r',
    binaryType: 'r',
    onopen: 'rw',
    ondrain: 'rw',
    ondata: 'rw',
    onerror: 'rw',
    onclose: 'rw'
  },
  
  _binaryType: null,

  
  _hasPrivileges: null,

  
  _transport: null,
  _socketInputStream: null,
  _socketOutputStream: null,

  
  _inputStreamPump: null,
  _inputStreamScriptable: null,
  _inputStreamBinary: null,

  
  _multiplexStream: null,
  _multiplexStreamCopier: null,

  _asyncCopierActive: false,
  _waitingForDrain: false,
  _suspendCount: 0,

  
  _bufferedAmount: 0,

  
  _socketBridge: null,

  
  get readyState() {
    return this._readyState;
  },
  get binaryType() {
    return this._binaryType;
  },
  get host() {
    return this._host;
  },
  get port() {
    return this._port;
  },
  get ssl() {
    return this._ssl;
  },
  get bufferedAmount() {
    if (this._inChild) {
      return this._bufferedAmount;
    }
    return this._multiplexStream.available();
  },
  get onopen() {
    return this._onopen;
  },
  set onopen(f) {
    this._onopen = f;
  },
  get ondrain() {
    return this._ondrain;
  },
  set ondrain(f) {
    this._ondrain = f;
  },
  get ondata() {
    return this._ondata;
  },
  set ondata(f) {
    this._ondata = f;
  },
  get onerror() {
    return this._onerror;
  },
  set onerror(f) {
    this._onerror = f;
  },
  get onclose() {
    return this._onclose;
  },
  set onclose(f) {
    this._onclose = f;
  },

  
  _createTransport: function ts_createTransport(host, port, sslMode) {
    let options, optlen;
    if (sslMode) {
      options = [sslMode];
      optlen = 1;
    } else {
      options = null;
      optlen = 0;
    }
    return Cc["@mozilla.org/network/socket-transport-service;1"]
             .getService(Ci.nsISocketTransportService)
             .createTransport(options, optlen, host, port, null);
  },

  _ensureCopying: function ts_ensureCopying() {
    let self = this;
    if (this._asyncCopierActive) {
      return;
    }
    this._asyncCopierActive = true;
    this._multiplexStreamCopier.asyncCopy({
      onStartRequest: function ts_output_onStartRequest() {
      },
      onStopRequest: function ts_output_onStopRequest(request, context, status) {
        self._asyncCopierActive = false;
        self._multiplexStream.removeStream(0);

        if (status) {
          self._readyState = kCLOSED;
          let err = new Error("Connection closed while writing: " + status);
          err.status = status;
          self.callListener("error", err);
          self.callListener("close");
          return;
        }

        if (self._multiplexStream.count) {
          self._ensureCopying();
        } else {
          if (self._waitingForDrain) {
            self._waitingForDrain = false;
            self.callListener("drain");
          }
          if (self._readyState === kCLOSING) {
            self._socketOutputStream.close();
            self._readyState = kCLOSED;
            self.callListener("close");
          }
        }
      }
    }, null);
  },

  callListener: function ts_callListener(type, data) {
    if (!this["on" + type])
      return;

    this["on" + type].call(null, new TCPSocketEvent(type, this, data || ""));
  },

  
  callListenerError: function ts_callListenerError(type, message, filename,
                                                   lineNumber, columnNumber) {
    this.callListener(type, new Error(message, filename, lineNumber, columnNumber));
  },

  callListenerData: function ts_callListenerString(type, data) {
    this.callListener(type, data);
  },

  callListenerArrayBuffer: function ts_callListenerArrayBuffer(type, data) {
    this.callListener(type, data);
  },

  callListenerVoid: function ts_callListenerVoid(type) {
    this.callListener(type);
  },

  updateReadyStateAndBuffered: function ts_setReadyState(readyState, bufferedAmount) {
    this._readyState = readyState;
    this._bufferedAmount = bufferedAmount;
  },
  

  initWindowless: function ts_initWindowless() {
    return Services.prefs.getBoolPref("dom.mozTCPSocket.enabled");
  },

  init: function ts_init(aWindow) {
    if (!this.initWindowless())
      return null;

    let principal = aWindow.document.nodePrincipal;
    let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
                   .getService(Ci.nsIScriptSecurityManager);

    let perm = principal == secMan.getSystemPrincipal()
                 ? Ci.nsIPermissionManager.ALLOW_ACTION
                 : Services.perms.testExactPermissionFromPrincipal(principal, "tcp-socket");

    this._hasPrivileges = perm == Ci.nsIPermissionManager.ALLOW_ACTION;

    let util = aWindow.QueryInterface(
      Ci.nsIInterfaceRequestor
    ).getInterface(Ci.nsIDOMWindowUtils);

    this.useWin = XPCNativeWrapper.unwrap(aWindow);
    this.innerWindowID = util.currentInnerWindowID;
    LOG("window init: " + this.innerWindowID);
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "inner-window-destroyed") {
      let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      if (wId == this.innerWindowID) {
        LOG("inner-window-destroyed: " + this.innerWindowID);

        
        
        
        this.onopen = null;
        this.ondrain = null;
        this.ondata = null;
        this.onerror = null;
        this.onclose = null;

        this.useWin = null;

        
        this.close();
      }
    }
  },

  
  open: function ts_open(host, port, options) {
    if (!this.initWindowless())
      return null;

    this._inChild = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
                       .processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
    LOG("content process: " + (this._inChild ? "true" : "false") + "\n");

    
    
    if (this._hasPrivileges !== true && this._hasPrivileges !== null) {
      throw new Error("TCPSocket does not have permission in this context.\n");
    }
    let that = new TCPSocket();

    that.useWin = this.useWin;
    that.innerWindowID = this.innerWindowID;
    that._inChild = this._inChild;

    LOG("window init: " + that.innerWindowID);
    Services.obs.addObserver(that, "inner-window-destroyed", true);

    LOG("startup called\n");
    LOG("Host info: " + host + ":" + port + "\n");

    that._readyState = kCONNECTING;
    that._host = host;
    that._port = port;
    if (options !== undefined) {
      if (options.useSSL) {
          that._ssl = 'ssl';
      } else {
          that._ssl = false;
      }
      that._binaryType = options.binaryType || that._binaryType;
    }

    LOG("SSL: " + that.ssl + "\n");

    if (this._inChild) {
      that._socketBridge = Cc["@mozilla.org/tcp-socket-child;1"]
                             .createInstance(Ci.nsITCPSocketChild);
      that._socketBridge.open(that, host, port, !!that._ssl,
                              that._binaryType, this.useWin, this);
      return that;
    }

    let transport = that._transport = this._createTransport(host, port, that._ssl);
    transport.setEventSink(that, Services.tm.currentThread);
    transport.securityCallbacks = new SecurityCallbacks(that);

    that._socketInputStream = transport.openInputStream(0, 0, 0);
    that._socketOutputStream = transport.openOutputStream(
      Ci.nsITransport.OPEN_UNBUFFERED, 0, 0);

    
    
    
    that._socketInputStream.asyncWait(
      that, that._socketInputStream.WAIT_CLOSURE_ONLY, 0, Services.tm.currentThread);

    if (that._binaryType === "arraybuffer") {
      that._inputStreamBinary = new BinaryInputStream(that._socketInputStream);
    } else {
      that._inputStreamScriptable = new ScriptableInputStream(that._socketInputStream);
    }

    that._multiplexStream = new MultiplexInputStream();

    that._multiplexStreamCopier = new AsyncStreamCopier(
      that._multiplexStream,
      that._socketOutputStream,
      
      Cc["@mozilla.org/network/socket-transport-service;1"]
        .getService(Ci.nsIEventTarget),
       true,  false,
      BUFFER_SIZE,  false,  false);

    return that;
  },

  close: function ts_close() {
    if (this._readyState === kCLOSED || this._readyState === kCLOSING)
      return;

    LOG("close called\n");
    this._readyState = kCLOSING;

    if (this._inChild) {
      this._socketBridge.close();
      return;
    }

    if (!this._multiplexStream.count) {
      this._socketOutputStream.close();
    }
    this._socketInputStream.close();
  },

  send: function ts_send(data, byteOffset, byteLength) {
    if (this._readyState !== kOPEN) {
      throw new Error("Socket not open.");
    }

    if (this._binaryType === "arraybuffer") {
      byteLength = byteLength || data.byteLength;
    }

    if (this._inChild) {
      this._socketBridge.send(data, byteOffset, byteLength);
    }

    let length = this._binaryType === "arraybuffer" ? byteLength : data.length;

    var newBufferedAmount = this.bufferedAmount + length;
    var bufferNotFull = newBufferedAmount < BUFFER_SIZE;
    if (this._inChild) {
      return bufferNotFull;
    }

    let new_stream;
    if (this._binaryType === "arraybuffer") {
      new_stream = new ArrayBufferInputStream();
      new_stream.setData(data, byteOffset, byteLength);
    } else {
      new_stream = new StringInputStream();
      new_stream.setData(data, length);
    }
    this._multiplexStream.appendStream(new_stream);

    if (newBufferedAmount >= BUFFER_SIZE) {
      
      
      
      
      
      this._waitingForDrain = true;
    }

    this._ensureCopying();
    return bufferNotFull;
  },

  suspend: function ts_suspend() {
    if (this._inChild) {
      this._socketBridge.suspend();
      return;
    }

    if (this._inputStreamPump) {
      this._inputStreamPump.suspend();
    } else {
      ++this._suspendCount;
    }
  },

  resume: function ts_resume() {
    if (this._inChild) {
      this._socketBridge.resume();
      return;
    }

    if (this._inputStreamPump) {
      this._inputStreamPump.resume();
    } else {
      --this._suspendCount;
    }
  },

  
  onTransportStatus: function ts_onTransportStatus(
    transport, status, progress, max) {
    if (status === Ci.nsISocketTransport.STATUS_CONNECTED_TO) {
      this._readyState = kOPEN;
      this.callListener("open");

      this._inputStreamPump = new InputStreamPump(
        this._socketInputStream, -1, -1, 0, 0, false
      );

      while (this._suspendCount--) {
        this._inputStreamPump.suspend();
      }

      this._inputStreamPump.asyncRead(this, null);
    }
  },

  
  
  onInputStreamReady: function ts_onInputStreamReady(input) {
    try {
      input.available();
    } catch (e) {
      this.callListener("error", new Error("Connection refused"));
    }
  },

  
  onStartRequest: function ts_onStartRequest(request, context) {
  },

  
  onStopRequest: function ts_onStopRequest(request, context, status) {
    let buffered_output = this._multiplexStream.count !== 0;

    this._inputStreamPump = null;

    if (buffered_output && !status) {
      
      
      
      
      
      return;
    }

    this._readyState = kCLOSED;

    if (status) {
      let err = new Error("Connection closed: " + status);
      err.status = status;
      this.callListener("error", err);
    }

    this.callListener("close");
  },

  
  onDataAvailable: function ts_onDataAvailable(request, context, inputStream, offset, count) {
    if (this._binaryType === "arraybuffer") {
      this.callListener("data", this._inputStreamBinary.readArrayBuffer(count));
    } else {
      this.callListener("data", this._inputStreamScriptable.read(count));
    }
  },

  classID: Components.ID("{cda91b22-6472-11e1-aa11-834fec09cd0a}"),

  classInfo: XPCOMUtils.generateCI({
    classID: Components.ID("{cda91b22-6472-11e1-aa11-834fec09cd0a}"),
    contractID: "@mozilla.org/tcp-socket;1",
    classDescription: "Client TCP Socket",
    interfaces: [
      Ci.nsIDOMTCPSocket,
    ],
    flags: Ci.nsIClassInfo.DOM_OBJECT,
  }),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMTCPSocket,
    Ci.nsITCPSocketInternal,
    Ci.nsIDOMGlobalPropertyInitializer,
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ])
}


function SecurityCallbacks(socket) {
  this._socket = socket;
}

SecurityCallbacks.prototype = {
  notifyCertProblem: function sc_notifyCertProblem(socketInfo, status,
                                                   targetSite) {
    this._socket.callListener("error", status);
    this._socket.close();
    return true;
  },

  getInterface: function sc_getInterface(iid) {
    return this.QueryInterface(iid);
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIBadCertListener2,
    Ci.nsIInterfaceRequestor,
    Ci.nsISupports
  ])
};


this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TCPSocket]);
