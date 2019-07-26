




"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
let Cr = Components.results;
let Cm = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

this.EXPORTED_SYMBOLS = ["BrowserElementPromptService"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";
const BROWSER_FRAMES_ENABLED_PREF = "dom.mozBrowserFramesEnabled";

function debug(msg) {
  
}

function BrowserElementPrompt(win, browserElementChild) {
  this._win = win;
  this._browserElementChild = browserElementChild;
}

BrowserElementPrompt.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),

  alert: function(title, text) {
    this._browserElementChild.showModalPrompt(
      this._win, {promptType: "alert", title: title, message: text, returnValue: undefined});
  },

  alertCheck: function(title, text, checkMsg, checkState) {
    
    
    this.alert(title, text);
  },

  confirm: function(title, text) {
    return this._browserElementChild.showModalPrompt(
      this._win, {promptType: "confirm", title: title, message: text, returnValue: undefined});
  },

  confirmCheck: function(title, text, checkMsg, checkState) {
    return this.confirm(title, text);
  },

  
  
  
  
  
  
  
  
  
  
  
  
  confirmEx: function(title, text, buttonFlags, button0Title, button1Title,
                      button2Title, checkMsg, checkState) {
    let buttonProperties = this._buildConfirmExButtonProperties(buttonFlags,
                                                                button0Title,
                                                                button1Title,
                                                                button2Title);
    let defaultReturnValue = { selectedButton: buttonProperties.defaultButton };
    if (checkMsg) {
      defaultReturnValue.checked = checkState.value;
    }
    let ret = this._browserElementChild.showModalPrompt(
      this._win,
      {
        promptType: "custom-prompt",
        title: title,
        message: text,
        defaultButton: buttonProperties.defaultButton,
        buttons: buttonProperties.buttons,
        showCheckbox: !!checkMsg,
        checkboxMessage: checkMsg,
        checkboxCheckedByDefault: !!checkState.value,
        returnValue: defaultReturnValue
      }
    );
    if (checkMsg) {
      checkState.value = ret.checked;
    }
    return buttonProperties.indexToButtonNumberMap[ret.selectedButton];
  },

  prompt: function(title, text, value, checkMsg, checkState) {
    let rv = this._browserElementChild.showModalPrompt(
      this._win,
      { promptType: "prompt",
        title: title,
        message: text,
        initialValue: value.value,
        returnValue: null });

    value.value = rv;

    
    
    
    
    
    return rv !== null;
  },

  promptUsernameAndPassword: function(title, text, username, password, checkMsg, checkState) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  promptPassword: function(title, text, password, checkMsg, checkState) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  select: function(title, text, aCount, aSelectList, aOutSelection) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  _buildConfirmExButtonProperties: function(buttonFlags, button0Title,
                                            button1Title, button2Title) {
    let r = {
      defaultButton: -1,
      buttons: [],
      
      
      indexToButtonNumberMap: []
    };

    let defaultButton = 0;  
    if (buttonFlags & Ci.nsIPrompt.BUTTON_POS_1_DEFAULT) {
      defaultButton = 1;
    } else if (buttonFlags & Ci.nsIPrompt.BUTTON_POS_2_DEFAULT) {
      defaultButton = 2;
    }

    
    let buttonPositions = [
      Ci.nsIPrompt.BUTTON_POS_0,
      Ci.nsIPrompt.BUTTON_POS_1,
      Ci.nsIPrompt.BUTTON_POS_2
    ];

    function buildButton(buttonTitle, buttonNumber) {
      let ret = {};
      let buttonPosition = buttonPositions[buttonNumber];
      let mask = 0xff * buttonPosition;  
      let titleType = (buttonFlags & mask) / buttonPosition;

      ret.messageType = 'builtin';
      switch(titleType) {
      case Ci.nsIPrompt.BUTTON_TITLE_OK:
        ret.message = 'ok';
        break;
      case Ci.nsIPrompt.BUTTON_TITLE_CANCEL:
        ret.message = 'cancel';
        break;
      case Ci.nsIPrompt.BUTTON_TITLE_YES:
        ret.message = 'yes';
        break;
      case Ci.nsIPrompt.BUTTON_TITLE_NO:
        ret.message = 'no';
        break;
      case Ci.nsIPrompt.BUTTON_TITLE_SAVE:
        ret.message = 'save';
        break;
      case Ci.nsIPrompt.BUTTON_TITLE_DONT_SAVE:
        ret.message = 'dontsave';
        break;
      case Ci.nsIPrompt.BUTTON_TITLE_REVERT:
        ret.message = 'revert';
        break;
      case Ci.nsIPrompt.BUTTON_TITLE_IS_STRING:
        ret.message = buttonTitle;
        ret.messageType = 'custom';
        break;
      default:
        
        return;
      }

      
      
      
      if (defaultButton === buttonNumber) {
        r.defaultButton = r.buttons.length;
      }
      r.buttons.push(ret);
      r.indexToButtonNumberMap.push(buttonNumber);
    }

    buildButton(button0Title, 0);
    buildButton(button1Title, 1);
    buildButton(button2Title, 2);

    
    
    if (r.defaultButton === -1) {
      throw new Components.Exception("Default button won't be shown",
                                     Cr.NS_ERROR_FAILURE);
    }

    return r;
  },
};


function BrowserElementAuthPrompt() {
}

BrowserElementAuthPrompt.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAuthPrompt2]),

  promptAuth: function promptAuth(channel, level, authInfo) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  asyncPromptAuth: function asyncPromptAuth(channel, callback, context, level, authInfo) {
    debug("asyncPromptAuth");

    
    if ((authInfo.flags & Ci.nsIAuthInformation.AUTH_PROXY) &&
        (authInfo.flags & Ci.nsIAuthInformation.ONLY_PASSWORD)) {
      throw Cr.NS_ERROR_FAILURE;
    }

    let frame = this._getFrameFromChannel(channel);
    if (!frame) {
      debug("Cannot get frame, asyncPromptAuth fail");
      throw Cr.NS_ERROR_FAILURE;
    }

    let browserElementParent =
      BrowserElementPromptService.getBrowserElementParentForFrame(frame);

    if (!browserElementParent) {
      debug("Failed to load browser element parent.");
      throw Cr.NS_ERROR_FAILURE;
    }

    let consumer = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsICancelable]),
      callback: callback,
      context: context,
      cancel: function() {
        this.callback.onAuthCancelled(this.context, false);
        this.callback = null;
        this.context = null;
      }
    };

    let [hostname, httpRealm] = this._getAuthTarget(channel, authInfo);
    let hashKey = level + "|" + hostname + "|" + httpRealm;
    let asyncPrompt = this._asyncPrompts[hashKey];
    if (asyncPrompt) {
      asyncPrompt.consumers.push(consumer);
      return consumer;
    }

    asyncPrompt = {
      consumers: [consumer],
      channel: channel,
      authInfo: authInfo,
      level: level,
      inProgress: false,
      browserElementParent: browserElementParent
    };

    this._asyncPrompts[hashKey] = asyncPrompt;
    this._doAsyncPrompt();
    return consumer;
  },

  

  _asyncPrompts: {},
  _asyncPromptInProgress: new WeakMap(),
  _doAsyncPrompt: function() {
    
    
    let hashKey = null;
    for (let key in this._asyncPrompts) {
      let prompt = this._asyncPrompts[key];
      if (!this._asyncPromptInProgress.get(prompt.browserElementParent)) {
        hashKey = key;
        break;
      }
    }

    
    if (!hashKey)
      return;

    let prompt = this._asyncPrompts[hashKey];
    let [hostname, httpRealm] = this._getAuthTarget(prompt.channel,
                                                    prompt.authInfo);

    this._asyncPromptInProgress.set(prompt.browserElementParent, true);
    prompt.inProgress = true;

    let self = this;
    let callback = function(ok, username, password) {
      debug("Async auth callback is called, ok = " +
            ok + ", username = " + username);

      
      
      delete self._asyncPrompts[hashKey];
      prompt.inProgress = false;
      self._asyncPromptInProgress.delete(prompt.browserElementParent);

      
      
      let flags = prompt.authInfo.flags;
      if (username) {
        if (flags & Ci.nsIAuthInformation.NEED_DOMAIN) {
          
          let idx = username.indexOf("\\");
          if (idx == -1) {
            prompt.authInfo.username = username;
          } else {
            prompt.authInfo.domain   = username.substring(0, idx);
            prompt.authInfo.username = username.substring(idx + 1);
          }
        } else {
          prompt.authInfo.username = username;
        }
      }

      if (password) {
        prompt.authInfo.password = password;
      }

      for each (let consumer in prompt.consumers) {
        if (!consumer.callback) {
          
          
          continue;
        }

        try {
          if (ok) {
            debug("Ok, calling onAuthAvailable to finish auth");
            consumer.callback.onAuthAvailable(consumer.context, prompt.authInfo);
          } else {
            debug("Cancelled, calling onAuthCancelled to finish auth.");
            consumer.callback.onAuthCancelled(consumer.context, true);
          }
        } catch (e) {  }
      }

      
      self._doAsyncPrompt();
    };

    let runnable = {
      run: function() {
        
        prompt.browserElementParent.promptAuth(
          self._createAuthDetail(prompt.channel, prompt.authInfo),
          callback);
      }
    }

    Services.tm.currentThread.dispatch(runnable, Ci.nsIThread.DISPATCH_NORMAL);
  },

  _getFrameFromChannel: function(channel) {
    let loadContext = channel.notificationCallbacks.getInterface(Ci.nsILoadContext);
    return loadContext.topFrameElement;
  },

  _createAuthDetail: function(channel, authInfo) {
    let [hostname, httpRealm] = this._getAuthTarget(channel, authInfo);
    return {
      host:             hostname,
      realm:            httpRealm,
      username:         authInfo.username,
      isOnlyPassword:   !!(authInfo.flags & Ci.nsIAuthInformation.ONLY_PASSWORD)
    };
  },

  _getAuthTarget : function (channel, authInfo) {
    let hostname = this._getFormattedHostname(channel.URI);

    
    
    
    let realm = authInfo.realm;
    if (!realm)
      realm = hostname;

    return [hostname, realm];
  },

  _getFormattedHostname : function(uri) {
    let scheme = uri.scheme;
    let hostname = scheme + "://" + uri.host;

    
    
    let port = uri.port;
    if (port != -1) {
      let handler = Services.io.getProtocolHandler(scheme);
      if (port != handler.defaultPort)
        hostname += ":" + port;
    }
    return hostname;
  }
};


function AuthPromptWrapper(oldImpl, browserElementImpl) {
  this._oldImpl = oldImpl;
  this._browserElementImpl = browserElementImpl;
}

AuthPromptWrapper.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAuthPrompt2]),
  promptAuth: function(channel, level, authInfo) {
    if (this._canGetParentElement(channel)) {
      return this._browserElementImpl.promptAuth(channel, level, authInfo);
    } else {
      return this._oldImpl.promptAuth(channel, level, authInfo);
    }
  },

  asyncPromptAuth: function(channel, callback, context, level, authInfo) {
    if (this._canGetParentElement(channel)) {
      return this._browserElementImpl.asyncPromptAuth(channel, callback, context, level, authInfo);
    } else {
      return this._oldImpl.asyncPromptAuth(channel, callback, context, level, authInfo);
    }
  },

  _canGetParentElement: function(channel) {
    try {
      let frame = channel.notificationCallbacks.getInterface(Ci.nsILoadContext).topFrameElement;
      if (!frame)
        return false;

      if (!BrowserElementPromptService.getBrowserElementParentForFrame(frame))
        return false;

      return true;
    } catch (e) {
      return false;
    }
  }
};

function BrowserElementPromptFactory(toWrap) {
  this._wrapped = toWrap;
}

BrowserElementPromptFactory.prototype = {
  classID: Components.ID("{24f3d0cf-e417-4b85-9017-c9ecf8bb1299}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPromptFactory]),

  _mayUseNativePrompt: function() {
    try {
      return Services.prefs.getBoolPref("browser.prompt.allowNative");
    } catch (e) {
      
      return true;
    }
  },

  _getNativePromptIfAllowed: function(win, iid, err) {
    if (this._mayUseNativePrompt())
      return this._wrapped.getPrompt(win, iid);
    else {
      
      throw err;
    }
  },

  getPrompt: function(win, iid) {
    
    
    
    if (!win)
      return this._getNativePromptIfAllowed(win, iid, Cr.NS_ERROR_INVALID_ARG);

    if (iid.number != Ci.nsIPrompt.number &&
        iid.number != Ci.nsIAuthPrompt2.number) {
      debug("We don't recognize the requested IID (" + iid + ", " +
            "allowed IID: " +
            "nsIPrompt=" + Ci.nsIPrompt + ", " +
            "nsIAuthPrompt2=" + Ci.nsIAuthPrompt2 + ")");
      return this._getNativePromptIfAllowed(win, iid, Cr.NS_ERROR_INVALID_ARG);
    }

    
    let browserElementChild =
      BrowserElementPromptService.getBrowserElementChildForWindow(win);

    if (iid.number === Ci.nsIAuthPrompt2.number) {
      debug("Caller requests an instance of nsIAuthPrompt2.");

      if (browserElementChild) {
        
        
        
        return new BrowserElementAuthPrompt().QueryInterface(iid);
      }

      
      
      
      
      
      if (this._mayUseNativePrompt()) {
        return new AuthPromptWrapper(
            this._wrapped.getPrompt(win, iid),
            new BrowserElementAuthPrompt().QueryInterface(iid))
          .QueryInterface(iid);
      } else {
        
        
        return new BrowserElementAuthPrompt().QueryInterface(iid);
      }
    }

    if (!browserElementChild) {
      debug("We can't find a browserElementChild for " +
            win + ", " + win.location);
      return this._getNativePromptIfAllowed(win, iid, Cr.NS_ERROR_FAILURE);
    }

    debug("Returning wrapped getPrompt for " + win);
    return new BrowserElementPrompt(win, browserElementChild)
                                   .QueryInterface(iid);
  }
};

this.BrowserElementPromptService = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  _initialized: false,

  _init: function() {
    if (this._initialized) {
      return;
    }

    
    if (!this._browserFramesPrefEnabled()) {
      var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
      prefs.addObserver(BROWSER_FRAMES_ENABLED_PREF, this,  true);
      return;
    }

    this._initialized = true;
    this._browserElementParentMap = new WeakMap();

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.addObserver(this, "outer-window-destroyed",  true);

    
    var contractID = "@mozilla.org/prompter;1";
    var oldCID = Cm.contractIDToCID(contractID);
    var newCID = BrowserElementPromptFactory.prototype.classID;
    var oldFactory = Cm.getClassObject(Cc[contractID], Ci.nsIFactory);

    if (oldCID == newCID) {
      debug("WARNING: Wrapped prompt factory is already installed!");
      return;
    }

    Cm.unregisterFactory(oldCID, oldFactory);

    var oldInstance = oldFactory.createInstance(null, Ci.nsIPromptFactory);
    var newInstance = new BrowserElementPromptFactory(oldInstance);

    var newFactory = {
      createInstance: function(outer, iid) {
        if (outer != null) {
          throw Cr.NS_ERROR_NO_AGGREGATION;
        }
        return newInstance.QueryInterface(iid);
      }
    };
    Cm.registerFactory(newCID,
                       "BrowserElementPromptService's prompter;1 wrapper",
                       contractID, newFactory);

    debug("Done installing new prompt factory.");
  },

  _getOuterWindowID: function(win) {
    return win.QueryInterface(Ci.nsIInterfaceRequestor)
              .getInterface(Ci.nsIDOMWindowUtils)
              .outerWindowID;
  },

  _browserElementChildMap: {},
  mapWindowToBrowserElementChild: function(win, browserElementChild) {
    this._browserElementChildMap[this._getOuterWindowID(win)] = browserElementChild;
  },

  getBrowserElementChildForWindow: function(win) {
    
    
    
    return this._browserElementChildMap[this._getOuterWindowID(win.top)];
  },

  mapFrameToBrowserElementParent: function(frame, browserElementParent) {
    this._browserElementParentMap.set(frame, browserElementParent);
  },

  getBrowserElementParentForFrame: function(frame) {
    return this._browserElementParentMap.get(frame);
  },

  _observeOuterWindowDestroyed: function(outerWindowID) {
    let id = outerWindowID.QueryInterface(Ci.nsISupportsPRUint64).data;
    debug("observeOuterWindowDestroyed " + id);
    delete this._browserElementChildMap[outerWindowID.data];
  },

  _browserFramesPrefEnabled: function() {
    var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    try {
      return prefs.getBoolPref(BROWSER_FRAMES_ENABLED_PREF);
    }
    catch(e) {
      return false;
    }
  },

  observe: function(subject, topic, data) {
    switch(topic) {
    case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
      if (data == BROWSER_FRAMES_ENABLED_PREF) {
        this._init();
      }
      break;
    case "outer-window-destroyed":
      this._observeOuterWindowDestroyed(subject);
      break;
    default:
      debug("Observed unexpected topic " + topic);
    }
  }
};

BrowserElementPromptService._init();
