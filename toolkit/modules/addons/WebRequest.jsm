



"use strict";

const EXPORTED_SYMBOLS = ["WebRequest"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "WebRequestCommon",
                                  "resource://gre/modules/WebRequestCommon.jsm");






function parseFilter(filter) {
  if (!filter) {
    filter = {};
  }

  
  return {urls: filter.urls || null, types: filter.types || null};
}

function parseExtra(extra, allowed) {
  if (extra) {
    for (let ex of extra) {
      if (allowed.indexOf(ex) == -1) {
        throw `Invalid option ${ex}`;
      }
    }
  }

  let result = {};
  for (let al of allowed) {
    if (extra && extra.indexOf(al) != -1) {
      result[al] = true;
    }
  }
  return result;
}

let ContentPolicyManager = {
  policyData: new Map(),
  policies: new Map(),
  idMap: new Map(),
  nextId: 0,

  init() {
    Services.ppmm.initialProcessData.webRequestContentPolicies = this.policyData;

    Services.ppmm.addMessageListener("WebRequest:ShouldLoad", this);
    Services.mm.addMessageListener("WebRequest:ShouldLoad", this);
  },

  receiveMessage(msg) {
    let browser = msg.target instanceof Ci.nsIDOMXULElement ? msg.target : null;

    for (let id of msg.data.ids) {
      let callback = this.policies.get(id);
      if (!callback) {
        
        
        continue;
      }
      let response = null;
      try {
        response = callback({
          url: msg.data.url,
          windowId: msg.data.windowId,
          parentWindowId: msg.data.parentWindowId,
          type: msg.data.type,
          browser: browser
        });
      } catch (e) {
        Cu.reportError(e);
      }

      if (response && response.cancel) {
        return {cancel: true};
      }

      
    }

    return {};
  },

  addListener(callback, opts) {
    let id = this.nextId++;
    opts.id = id;
    Services.ppmm.broadcastAsyncMessage("WebRequest:AddContentPolicy", opts);

    this.policyData.set(id, opts);

    this.policies.set(id, callback);
    this.idMap.set(callback, id);
  },

  removeListener(callback) {
    let id = this.idMap.get(callback);
    Services.ppmm.broadcastAsyncMessage("WebRequest:RemoveContentPolicy", {id});

    this.policyData.delete(id);
    this.idMap.delete(callback);
    this.policies.delete(id);
  },
};
ContentPolicyManager.init();

function StartStopListener(manager)
{
  this.manager = manager;
  this.orig = null;
}

StartStopListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRequestObserver,
                                         Ci.nsIStreamListener,
                                         Ci.nsISupports]),

  onStartRequest: function(request, context) {
    this.manager.onStartRequest(request);
    return this.orig.onStartRequest(request, context);
  },

  onStopRequest(request, context, statusCode) {
    let result = this.orig.onStopRequest(request, context, statusCode);
    this.manager.onStopRequest(request);
    return result;
  },

  onDataAvailable(...args) {
    return this.orig.onDataAvailable(...args);
  }
};

let HttpObserverManager = {
  modifyInitialized: false,
  examineInitialized: false,

  listeners: {
    modify: new Map(),
    afterModify: new Map(),
    headersReceived: new Map(),
    onStart: new Map(),
    onStop: new Map(),
  },

  addOrRemove() {
    let needModify = this.listeners.modify.size || this.listeners.afterModify.size;
    if (needModify && !this.modifyInitialized) {
      this.modifyInitialized = true;
      Services.obs.addObserver(this, "http-on-modify-request", false);
    } else if (!needModify && this.modifyInitialized) {
      this.modifyInitialized = false;
      Services.obs.removeObserver(this, "http-on-modify-request");
    }

    let needExamine = this.listeners.headersReceived.size ||
                      this.listeners.onStart.size ||
                      this.listeners.onStop.size;
    if (needExamine && !this.examineInitialized) {
      this.examineInitialized = true;
      Services.obs.addObserver(this, "http-on-examine-response", false);
      Services.obs.addObserver(this, "http-on-examine-cached-response", false);
      Services.obs.addObserver(this, "http-on-examine-merged-response", false);
    } else if (!needExamine && this.examineInitialized) {
      this.examineInitialized = false;
      Services.obs.removeObserver(this, "http-on-examine-response");
      Services.obs.removeObserver(this, "http-on-examine-cached-response");
      Services.obs.removeObserver(this, "http-on-examine-merged-response");
    }
  },

  addListener(kind, callback, opts) {
    this.listeners[kind].set(callback, opts);
    this.addOrRemove();
  },

  removeListener(kind, callback) {
    this.listeners[kind].delete(callback);
    this.addOrRemove();
  },

  getLoadContext(channel) {
    try {
      return channel.QueryInterface(Ci.nsIChannel)
                    .notificationCallbacks
                    .getInterface(Components.interfaces.nsILoadContext);
    } catch (e) {
      try {
        return channel.loadGroup
                      .notificationCallbacks
                      .getInterface(Components.interfaces.nsILoadContext);
      } catch (e) {
        return null;
      }
    }
  },

  getHeaders(channel, method) {
    let headers = [];
    let visitor = {
      visitHeader(name, value) {
        headers.push({name, value});
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.nsIHttpHeaderVisitor,
                                             Ci.nsISupports]),
    };

    channel[method](visitor);
    return headers;
  },

  observe(subject, topic, data) {
    if (topic == "http-on-modify-request") {
      this.modify(subject, topic, data);
    } else if (topic == "http-on-examine-response" ||
               topic == "http-on-examine-cached-response" ||
               topic == "http-on-examine-merged-response") {
      this.examine(subject, topic, data);
    }
  },

  shouldRunListener(policyType, uri, filter) {
    return WebRequestCommon.typeMatches(policyType, filter.types) &&
           WebRequestCommon.urlMatches(uri, filter.urls);
  },

  runChannelListener(request, kind) {
    let listeners = this.listeners[kind];
    let channel = request.QueryInterface(Ci.nsIHttpChannel);
    let loadContext = this.getLoadContext(channel);
    let browser = loadContext ? loadContext.topFrameElement : null;
    let policyType = channel.loadInfo.contentPolicyType;

    let requestHeaders;
    let responseHeaders;

    let includeStatus = kind == "headersReceived" || kind == "onStart" || kind == "onStop";

    for (let [callback, opts] of listeners.entries()) {
      if (!this.shouldRunListener(policyType, channel.URI, opts.filter)) {
        continue;
      }

      let data = {
        url: channel.URI.spec,
        method: channel.requestMethod,
        browser: browser,
        type: WebRequestCommon.typeForPolicyType(policyType),
      };
      if (opts.requestHeaders) {
        if (!requestHeaders) {
          requestHeaders = this.getHeaders(channel, "visitRequestHeaders");
        }
        data.requestHeaders = requestHeaders;
      }
      if (opts.responseHeaders) {
        if (!responseHeaders) {
          responseHeaders = this.getHeaders(channel, "visitResponseHeaders");
        }
        data.responseHeaders = responseHeaders;
      }
      if (includeStatus) {
        data.statusCode = channel.responseStatus;
      }

      let result = null;
      try {
        result = callback(data);
      } catch (e) {
        Cu.reportError(e);
      }

      if (!result || !opts.blocking) {
        return true;
      }
      if (result.cancel) {
        channel.cancel();
        return false;
      }
      if (result.redirectUrl) {
        channel.redirectTo(BrowserUtils.makeURI(result.redirectUrl));
        return false;
      }
      if (opts.requestHeaders && result.requestHeaders) {
        
        for (let {name, value} of requestHeaders) {
          channel.setRequestHeader(name, "", false);
        }

        for (let {name, value} of result.requestHeaders) {
          channel.setRequestHeader(name, value, false);
        }
      }
      if (opts.responseHeaders && result.responseHeaders) {
        
        for (let {name, value} of responseHeaders) {
          channel.setResponseHeader(name, "", false);
        }

        for (let {name, value} of result.responseHeaders) {
          channel.setResponseHeader(name, value, false);
        }
      }
    }
  },

  modify(subject, topic, data) {
    if (this.runChannelListener(subject, "modify")) {
      this.runChannelListener(subject, "afterModify");
    }
  },

  examine(subject, topic, data) {
    let channel = subject.QueryInterface(Ci.nsIHttpChannel);
    if (this.listeners.onStart.size || this.listeners.onStop.size) {
      if (channel instanceof Components.interfaces.nsITraceableChannel) {
        let listener = new StartStopListener(this);
        let orig = subject.setNewListener(listener);
        listener.orig = orig;
      }
    }

    this.runChannelListener(subject, "headersReceived");
  },

  onStartRequest(request) {
    this.runChannelListener(request, "onStart");
  },

  onStopRequest(request) {
    this.runChannelListener(request, "onStop");
  },
};

let onBeforeRequest = {
  addListener(callback, filter = null, opt_extraInfoSpec = null) {
    
    let opts = parseExtra(opt_extraInfoSpec, ["blocking"]);
    opts.filter = parseFilter(filter);
    ContentPolicyManager.addListener(callback, opts);
  },

  removeListener(callback) {
    ContentPolicyManager.removeListener(callback);
  }
};

let onBeforeSendHeaders = {
  addListener(callback, filter = null, opt_extraInfoSpec = null) {
    let opts = parseExtra(opt_extraInfoSpec, ["requestHeaders", "blocking"]);
    opts.filter = parseFilter(filter);
    HttpObserverManager.addListener("modify", callback, opts);
  },

  removeListener(callback) {
    HttpObserverManager.removeListener("modify", callback);
  }
};

let onSendHeaders = {
  addListener(callback, filter = null, opt_extraInfoSpec = null) {
    let opts = parseExtra(opt_extraInfoSpec, ["requestHeaders"]);
    opts.filter = parseFilter(filter);
    HttpObserverManager.addListener("afterModify", callback, opts);
  },

  removeListener(callback) {
    HttpObserverManager.removeListener("afterModify", callback);
  }
};

let onHeadersReceived = {
  addListener(callback, filter = null, opt_extraInfoSpec = null) {
    let opts = parseExtra(opt_extraInfoSpec, ["blocking", "responseHeaders"]);
    opts.filter = parseFilter(filter);
    HttpObserverManager.addListener("headersReceived", callback, opts);
  },

  removeListener(callback) {
    HttpObserverManager.removeListener("headersReceived", callback);
  }
};

let onResponseStarted = {
  addListener(callback, filter = null, opt_extraInfoSpec = null) {
    let opts = parseExtra(opt_extraInfoSpec, ["responseHeaders"]);
    opts.filter = parseFilter(filter);
    HttpObserverManager.addListener("onStart", callback, opts);
  },

  removeListener(callback) {
    HttpObserverManager.removeListener("onStart", callback);
  }
};

let onCompleted = {
  addListener(callback, filter = null, opt_extraInfoSpec = null) {
    let opts = parseExtra(opt_extraInfoSpec, ["responseHeaders"]);
    opts.filter = parseFilter(filter);
    HttpObserverManager.addListener("onStop", callback, opts);
  },

  removeListener(callback) {
    HttpObserverManager.removeListener("onStop", callback);
  }
};

let WebRequest = {
  
  onBeforeRequest: onBeforeRequest,

  
  onBeforeSendHeaders: onBeforeSendHeaders,

  
  onSendHeaders: onSendHeaders,

  
  onHeadersReceived: onHeadersReceived,

  
  onResponseStarted: onResponseStarted,

  
  onCompleted: onCompleted,
};

Services.ppmm.loadProcessScript("resource://gre/modules/WebRequestContent.js", true);
