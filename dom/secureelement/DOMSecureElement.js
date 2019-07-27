





"use strict";



const DEBUG = false;
function debug(s) {
  if (DEBUG) {
    dump("-*- SecureElement DOM: " + s + "\n");
  }
}

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

XPCOMUtils.defineLazyGetter(this, "SE", function() {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});

function PromiseHelpersSubclass(win) {
  this._window = win;
}

PromiseHelpersSubclass.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  _window: null,

  _context: [],

  createSEPromise: function createSEPromise(callback,  ctx) {
    let ctxCallback = (resolverId) => {
      if (ctx) {
        this._context[resolverId] = ctx;
      }

      callback(resolverId);
    };

    return this.createPromise((resolve, reject) => {
      let resolverId = this.getPromiseResolverId({
        resolve: resolve,
        reject: reject
      });
      ctxCallback(resolverId);
    });
  },

  takePromise: function takePromise(resolverId) {
    let resolver = this.takePromiseResolver(resolverId);
    if (!resolver) {
      return;
    }

    
    let context = this._context[resolverId];
    delete this._context[resolverId];

    return {resolver: resolver, context: context};
  },

  rejectWithSEError: function rejectWithSEError(reason) {
    return this.createSEPromise((resolverId) => {
      debug("rejectWithSEError : " + reason);
      this.takePromiseResolver(resolverId).reject(new Error(reason));
    });
  }
};


let PromiseHelpers;







function SEReaderImpl() {}

SEReaderImpl.prototype = {
  _window: null,

  _sessions: [],

  type: null,

  classID: Components.ID("{1c7bdba3-cd35-4f8b-a546-55b3232457d5}"),
  contractID: "@mozilla.org/secureelement/reader;1",
  QueryInterface: XPCOMUtils.generateQI([]),

  
  onSessionClose: function onSessionClose(sessionCtx) {
    let index = this._sessions.indexOf(sessionCtx);
    if (index != -1) {
      this._sessions.splice(index, 1);
    }
  },

  initialize: function initialize(win, type) {
    this._window = win;
    this.type = type;
  },

  openSession: function openSession() {
    return PromiseHelpers.createSEPromise((resolverId) => {
      let chromeObj = new SESessionImpl();
      chromeObj.initialize(this._window, this);
      let contentObj = this._window.SESession._create(this._window, chromeObj);
      this._sessions.push(contentObj);
      PromiseHelpers.takePromiseResolver(resolverId).resolve(contentObj);
    });
  },

  closeAll: function closeAll() {
    return PromiseHelpers.createSEPromise((resolverId) => {
      let promises = [];
      for (let session of this._sessions) {
        if (!session.isClosed) {
          promises.push(session.closeAll());
        }
      }

      let resolver = PromiseHelpers.takePromiseResolver(resolverId);
      
      Promise.all(promises).then(() => {
        this._sessions = [];
        resolver.resolve();
      }, (reason) => {
        resolver.reject(new Error(SE.ERROR_BADSTATE +
          " Unable to close all channels associated with this reader"));
      });
    });
  },

  get isSEPresent() {
    
    
    return true;
  }
};







function SESessionImpl() {}

SESessionImpl.prototype = {
  _window: null,

  _channels: [],

  _isClosed: false,

  _reader: null,

  classID: Components.ID("{2b1809f8-17bd-4947-abd7-bdef1498561c}"),
  contractID: "@mozilla.org/secureelement/session;1",
  QueryInterface: XPCOMUtils.generateQI([]),

  
  _checkClosed: function _checkClosed() {
    if (this._isClosed) {
      throw new Error(SE.ERROR_BADSTATE + " Session Already Closed!");
    }
  },

  
  onChannelOpen: function onChannelOpen(channelCtx) {
    this._channels.push(channelCtx);
  },

  
  onChannelClose: function onChannelClose(channelCtx) {
    let index = this._channels.indexOf(channelCtx);
    if (index != -1) {
      this._channels.splice(index, 1);
    }
  },

  initialize: function initialize(win, readerCtx) {
    this._window = win;
    this._reader = readerCtx;
  },

  openLogicalChannel: function openLogicalChannel(aid) {
    this._checkClosed();

    let aidLen = aid ? aid.length : 0;
    if (aidLen < SE.MIN_AID_LEN || aidLen > SE.MAX_AID_LEN) {
      return PromiseHelpers.rejectWithSEError(SE.ERROR_GENERIC +
             " Invalid AID length - " + aidLen);
    }

    return PromiseHelpers.createSEPromise((resolverId) => {
      







      cpmm.sendAsyncMessage("SE:OpenChannel", {
        resolverId: resolverId,
        aid: aid,
        type: this.reader.type,
        appId: this._window.document.nodePrincipal.appId
      });
    }, this);
  },

  closeAll: function closeAll() {
    this._checkClosed();

    return PromiseHelpers.createSEPromise((resolverId) => {
      let promises = [];
      for (let channel of this._channels) {
        if (!channel.isClosed) {
          promises.push(channel.close());
        }
      }

      let resolver = PromiseHelpers.takePromiseResolver(resolverId);
      Promise.all(promises).then(() => {
        this._isClosed = true;
        this._channels = [];
        
        
        this._reader.onSessionClose(this.__DOM_IMPL__);
        resolver.resolve();
      }, (reason) => {
        resolver.reject(new Error(SE.ERROR_BADSTATE +
          " Unable to close all channels associated with this session"));
      });
    });
  },

  get reader() {
    return this._reader.__DOM_IMPL__;
  },

  get isClosed() {
    return this._isClosed;
  },

  set isClosed(isClosed) {
    this._isClosed = isClosed;
  }
};






function SEChannelImpl() {}

SEChannelImpl.prototype = {
  _window: null,

  _channelToken: null,

  _isClosed: false,

  _session: null,

  openResponse: [],

  type: null,

  classID: Components.ID("{181ebcf4-5164-4e28-99f2-877ec6fa83b9}"),
  contractID: "@mozilla.org/secureelement/channel;1",
  QueryInterface: XPCOMUtils.generateQI([]),

  _checkClosed: function _checkClosed() {
    if (this._isClosed) {
      throw new Error(SE.ERROR_BADSTATE + " Channel Already Closed!");
    }
  },

  
  onClose: function onClose() {
    this._isClosed = true;
    
    this._session.onChannelClose(this.__DOM_IMPL__);
  },

  initialize: function initialize(win, channelToken, isBasicChannel,
                                  openResponse, sessionCtx) {
    this._window = win;
    
    
    this._channelToken = channelToken;
    
    this._session = sessionCtx;
    this.openResponse = Cu.cloneInto(new Uint8Array(openResponse), win);
    this.type = isBasicChannel ? "basic" : "logical";
  },

  transmit: function transmit(command) {
    
    
    if (!command) {
      return PromiseHelpers.rejectWithSEError(SE.ERROR_GENERIC +
        " SECommand dict must be defined");
    }

    this._checkClosed();

    let dataLen = command.data ? command.data.length : 0;
    if (dataLen > SE.MAX_APDU_LEN) {
      return PromiseHelpers.rejectWithSEError(SE.ERROR_GENERIC +
             " Command data length exceeds max limit - 255. " +
             " Extended APDU is not supported!");
    }

    if ((command.cla & 0x80 === 0) && ((command.cla & 0x60) !== 0x20)) {
      if (command.ins === SE.INS_MANAGE_CHANNEL) {
        return PromiseHelpers.rejectWithSEError(SE.ERROR_SECURITY +
               ", MANAGE CHANNEL command not permitted");
      }
      if ((command.ins === SE.INS_SELECT) && (command.p1 == 0x04)) {
        
        return PromiseHelpers.rejectWithSEError(SE.ERROR_SECURITY +
               ", SELECT command not permitted");
      }
      debug("Attempting to transmit an ISO command");
    } else {
      debug("Attempting to transmit GlobalPlatform command");
    }

    return PromiseHelpers.createSEPromise((resolverId) => {
      








      cpmm.sendAsyncMessage("SE:TransmitAPDU", {
        resolverId: resolverId,
        apdu: command,
        channelToken: this._channelToken,
        appId: this._window.document.nodePrincipal.appId
      });
    }, this);
  },

  close: function close() {
    this._checkClosed();

    return PromiseHelpers.createSEPromise((resolverId) => {
      







      cpmm.sendAsyncMessage("SE:CloseChannel", {
        resolverId: resolverId,
        channelToken: this._channelToken,
        appId: this._window.document.nodePrincipal.appId
      });
    }, this);
  },

  get session() {
    return this._session.__DOM_IMPL__;
  },

  get isClosed() {
    return this._isClosed;
  },

  set isClosed(isClosed) {
    this._isClosed = isClosed;
  }
};

function SEResponseImpl() {}

SEResponseImpl.prototype = {
  sw1: 0x00,

  sw2: 0x00,

  data: null,

  _channel: null,

  classID: Components.ID("{58bc6c7b-686c-47cc-8867-578a6ed23f4e}"),
  contractID: "@mozilla.org/secureelement/response;1",
  QueryInterface: XPCOMUtils.generateQI([]),

  initialize: function initialize(sw1, sw2, response, channelCtx) {
    
    this.sw1 = sw1;
    this.sw2 = sw2;
    this.data = response ? response.slice(0) : null;
    
    this._channel = channelCtx;
  },

  get channel() {
    return this._channel.__DOM_IMPL__;
  }
};




function SEManagerImpl() {}

SEManagerImpl.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  _window: null,

  classID: Components.ID("{4a8b6ec0-4674-11e4-916c-0800200c9a66}"),
  contractID: "@mozilla.org/secureelement/manager;1",
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMGlobalPropertyInitializer,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver
  ]),

  init: function init(win) {
    this._window = win;
    PromiseHelpers = new PromiseHelpersSubclass(this._window);

    
    const messages = ["SE:GetSEReadersResolved",
                      "SE:OpenChannelResolved",
                      "SE:CloseChannelResolved",
                      "SE:TransmitAPDUResolved",
                      "SE:GetSEReadersRejected",
                      "SE:OpenChannelRejected",
                      "SE:CloseChannelRejected",
                      "SE:TransmitAPDURejected"];

    this.initDOMRequestHelper(win, messages);
  },

  
  uninit: function uninit() {
    
    
    this.forEachPromiseResolver((k) => {
      this.takePromiseResolver(k).reject("Window Context got destroyed!");
    });
    PromiseHelpers = null;
    this._window = null;
  },

  getSEReaders: function getSEReaders() {
    return PromiseHelpers.createSEPromise((resolverId) => {
      





      cpmm.sendAsyncMessage("SE:GetSEReaders", {
        resolverId: resolverId,
        appId: this._window.document.nodePrincipal.appId
      });
    });
  },

  receiveMessage: function receiveMessage(message) {
    let result = message.data.result;
    let chromeObj = null;
    let contentObj = null;
    let resolver = null;
    let context = null;

    let promiseResolver = PromiseHelpers.takePromise(result.resolverId);
    if (promiseResolver) {
      resolver = promiseResolver.resolver;
      
      context = promiseResolver.context;
    }

    debug("receiveMessage(): " + message.name);
    switch (message.name) {
      case "SE:GetSEReadersResolved":
        let readers = new this._window.Array();
        for (let i = 0; i < result.readerTypes.length; i++) {
          chromeObj = new SEReaderImpl();
          chromeObj.initialize(this._window, result.readerTypes[i]);
          contentObj = this._window.SEReader._create(this._window, chromeObj);
          readers.push(contentObj);
        }
        resolver.resolve(readers);
        break;
      case "SE:OpenChannelResolved":
        chromeObj = new SEChannelImpl();
        chromeObj.initialize(this._window,
                             result.channelToken,
                             result.isBasicChannel,
                             result.openResponse,
                             context);
        contentObj = this._window.SEChannel._create(this._window, chromeObj);
        if (context) {
          
          context.onChannelOpen(contentObj);
        }
        resolver.resolve(contentObj);
        break;
      case "SE:TransmitAPDUResolved":
        chromeObj = new SEResponseImpl();
        chromeObj.initialize(result.sw1,
                             result.sw2,
                             result.response,
                             context);
        contentObj = this._window.SEResponse._create(this._window, chromeObj);
        resolver.resolve(contentObj);
        break;
      case "SE:CloseChannelResolved":
        if (context) {
          
          context.onClose();
        }
        resolver.resolve();
        break;
      case "SE:GetSEReadersRejected":
      case "SE:OpenChannelRejected":
      case "SE:CloseChannelRejected":
      case "SE:TransmitAPDURejected":
        let error = result.error || SE.ERROR_GENERIC;
        resolver.reject(error);
        break;
      default:
        debug("Could not find a handler for " + message.name);
        resolver.reject();
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([
  SEResponseImpl, SEChannelImpl, SESessionImpl, SEReaderImpl, SEManagerImpl
]);
