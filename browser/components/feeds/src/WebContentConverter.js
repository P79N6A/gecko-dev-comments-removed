# -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Web Content Converter System.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <beng@google.com>
#   Asaf Romano <mano@mozilla.com>
#   Dan Mosedale <dmose@mozilla.org>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK ***** */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function LOG(str) {
  dump("*** " + str + "\n");
}

const WCCR_CONTRACTID = "@mozilla.org/embeddor.implemented/web-content-handler-registrar;1";
const WCCR_CLASSID = Components.ID("{792a7e82-06a0-437c-af63-b2d12e808acc}");
const WCCR_CLASSNAME = "Web Content Handler Registrar";

const WCC_CLASSID = Components.ID("{db7ebf28-cc40-415f-8a51-1b111851df1e}");
const WCC_CLASSNAME = "Web Service Handler";

const TYPE_MAYBE_FEED = "application/vnd.mozilla.maybe.feed";
const TYPE_ANY = "*/*";

const PREF_CONTENTHANDLERS_AUTO = "browser.contentHandlers.auto.";
const PREF_CONTENTHANDLERS_BRANCH = "browser.contentHandlers.types.";
const PREF_SELECTED_WEB = "browser.feeds.handlers.webservice";
const PREF_SELECTED_ACTION = "browser.feeds.handler";
const PREF_SELECTED_READER = "browser.feeds.handler.default";
const PREF_HANDLER_EXTERNAL_PREFIX = "network.protocol-handler.external";
const PREF_ALLOW_DIFFERENT_HOST = "gecko.handlerService.allowRegisterFromDifferentHost";

const STRING_BUNDLE_URI = "chrome://browser/locale/feeds/subscribe.properties";

const NS_ERROR_MODULE_DOM = 2152923136;
const NS_ERROR_DOM_SYNTAX_ERR = NS_ERROR_MODULE_DOM + 12;

function WebContentConverter() {
}
WebContentConverter.prototype = {
  convert: function WCC_convert() { },
  asyncConvertData: function WCC_asyncConvertData() { },
  onDataAvailable: function WCC_onDataAvailable() { },
  onStopRequest: function WCC_onStopRequest() { },
  
  onStartRequest: function WCC_onStartRequest(request, context) {
    var wccr = 
        Cc[WCCR_CONTRACTID].
        getService(Ci.nsIWebContentConverterService);
    wccr.loadPreferredHandler(request);
  },
  
  QueryInterface: function WCC_QueryInterface(iid) {
    if (iid.equals(Ci.nsIStreamConverter) ||
        iid.equals(Ci.nsIStreamListener) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

var WebContentConverterFactory = {
  createInstance: function WCCF_createInstance(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return new WebContentConverter().QueryInterface(iid);
  },
    
  QueryInterface: function WCC_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFactory) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

function ServiceInfo(contentType, uri, name) {
  this._contentType = contentType;
  this._uri = uri;
  this._name = name;
}
ServiceInfo.prototype = {
  


  get name() {
    return this._name;
  },
  
  


  equals: function SI_equals(aHandlerApp) {
    if (!aHandlerApp)
      throw Cr.NS_ERROR_NULL_POINTER;

    if (aHandlerApp instanceof Ci.nsIWebContentHandlerInfo &&
        aHandlerApp.contentType == this.contentType &&
        aHandlerApp.uri == this.uri)
      return true;

    return false;
  },

  


  get contentType() {
    return this._contentType;
  },

  


  get uri() {
    return this._uri;
  },

  


  getHandlerURI: function SI_getHandlerURI(uri) {
    return this._uri.replace(/%s/gi, encodeURIComponent(uri));
  },
  
  QueryInterface: function SI_QueryInterface(iid) {
    if (iid.equals(Ci.nsIWebContentHandlerInfo) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

function WebContentConverterRegistrar() {
  this._contentTypes = { };
  this._autoHandleContentTypes = { };
}

WebContentConverterRegistrar.prototype = {
  get stringBundle() {
    var sb = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService).
              createBundle(STRING_BUNDLE_URI);
    delete WebContentConverterRegistrar.prototype.stringBundle;
    return WebContentConverterRegistrar.prototype.stringBundle = sb;
  },

  _getFormattedString: function WCCR__getFormattedString(key, params) {
    return this.stringBundle.formatStringFromName(key, params, params.length);
  },
  
  _getString: function WCCR_getString(key) {
    return this.stringBundle.GetStringFromName(key);
  },

  


  getAutoHandler: 
  function WCCR_getAutoHandler(contentType) {
    contentType = this._resolveContentType(contentType);
    if (contentType in this._autoHandleContentTypes)
      return this._autoHandleContentTypes[contentType];
    return null;
  },
  
  


  setAutoHandler:
  function WCCR_setAutoHandler(contentType, handler) {
    if (handler && !this._typeIsRegistered(contentType, handler.uri))
      throw Cr.NS_ERROR_NOT_AVAILABLE;
      
    contentType = this._resolveContentType(contentType);
    this._setAutoHandler(contentType, handler);
    
    var ps = 
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefService);
    var autoBranch = ps.getBranch(PREF_CONTENTHANDLERS_AUTO);
    if (handler)
      autoBranch.setCharPref(contentType, handler.uri);
    else if (autoBranch.prefHasUserValue(contentType))
      autoBranch.clearUserPref(contentType);
     
    ps.savePrefFile(null);
  },
  
  


  _setAutoHandler:
  function WCCR__setAutoHandler(contentType, handler) {
    if (handler) 
      this._autoHandleContentTypes[contentType] = handler;
    else if (contentType in this._autoHandleContentTypes)
      delete this._autoHandleContentTypes[contentType];
  },
  
  


  getWebContentHandlerByURI:
  function WCCR_getWebContentHandlerByURI(contentType, uri) {
    var handlers = this.getContentHandlers(contentType, { });
    for (var i = 0; i < handlers.length; ++i) {
      if (handlers[i].uri == uri) 
        return handlers[i];
    }
    return null;
  },
  
  


  loadPreferredHandler: 
  function WCCR_loadPreferredHandler(request) {
    var channel = request.QueryInterface(Ci.nsIChannel);
    var contentType = this._resolveContentType(channel.contentType);
    var handler = this.getAutoHandler(contentType);
    if (handler) {
      request.cancel(Cr.NS_ERROR_FAILURE);
      
      var webNavigation = 
          channel.notificationCallbacks.getInterface(Ci.nsIWebNavigation);
      webNavigation.loadURI(handler.getHandlerURI(channel.URI.spec), 
                            Ci.nsIWebNavigation.LOAD_FLAGS_NONE, 
                            null, null, null);
    }      
  },
  
  


  removeProtocolHandler: 
  function WCCR_removeProtocolHandler(aProtocol, aURITemplate) {
    var eps = Cc["@mozilla.org/uriloader/external-protocol-service;1"].
              getService(Ci.nsIExternalProtocolService);
    var handlerInfo = eps.getProtocolHandlerInfo(aProtocol);
    var handlers =  handlerInfo.possibleApplicationHandlers;
    for (let i = 0; i < handlers.length; i++) {
      try { 
        let handler = handlers.queryElementAt(i, Ci.nsIWebHandlerApp);
        if (handler.uriTemplate == aURITemplate) {
          handlers.removeElementAt(i);
          var hs = Cc["@mozilla.org/uriloader/handler-service;1"].
                   getService(Ci.nsIHandlerService);
          hs.store(handlerInfo);
          return;
        }
      } catch (e) {  }
    }
  },
  
  


  removeContentHandler: 
  function WCCR_removeContentHandler(contentType, uri) {
    function notURI(serviceInfo) {
      return serviceInfo.uri != uri;
    }
  
    if (contentType in this._contentTypes) {
      this._contentTypes[contentType] = 
        this._contentTypes[contentType].filter(notURI);
    }
  },
  
  


  _mappings: { 
    "application/rss+xml": TYPE_MAYBE_FEED,
    "application/atom+xml": TYPE_MAYBE_FEED,
  },
  
  




  _blockedTypes: {
    "application/vnd.mozilla.maybe.feed": true,
  },
  
  




  _resolveContentType: 
  function WCCR__resolveContentType(contentType) {
    if (contentType in this._mappings)
      return this._mappings[contentType];
    return contentType;
  },

  _makeURI: function(aURL, aOriginCharset, aBaseURI) {
    var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService);
    return ioService.newURI(aURL, aOriginCharset, aBaseURI);
  },

  _checkAndGetURI:
  function WCCR_checkAndGetURI(aURIString, aContentWindow)
  {
    try {
      var uri = this._makeURI(aURIString);
    } catch (ex) {
      
      return; 
    }

    
    
    
    
    if (uri.scheme != "http" && uri.scheme != "https")
      throw("Permission denied to add " + uri.spec + " as a content or protocol handler");

    
    
    var pb = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    if ((!pb.prefHasUserValue(PREF_ALLOW_DIFFERENT_HOST) ||
         !pb.getBoolPref(PREF_ALLOW_DIFFERENT_HOST)) &&
        aContentWindow.location.hostname != uri.host)
      throw("Permission denied to add " + uri.spec + " as a content or protocol handler");

    
    if (uri.spec.indexOf("%s") < 0)
      throw NS_ERROR_DOM_SYNTAX_ERR; 

    return uri;
  },

  








  _protocolHandlerRegistered:
  function WCCR_protocolHandlerRegistered(aProtocol, aURITemplate) {
    var eps = Cc["@mozilla.org/uriloader/external-protocol-service;1"].
              getService(Ci.nsIExternalProtocolService);
    var handlerInfo = eps.getProtocolHandlerInfo(aProtocol);
    var handlers =  handlerInfo.possibleApplicationHandlers;
    for (let i = 0; i < handlers.length; i++) {
      try { 
        let handler = handlers.queryElementAt(i, Ci.nsIWebHandlerApp);
        if (handler.uriTemplate == aURITemplate)
          return true;
      } catch (e) {  }
    }
    return false;
  },

  


  registerProtocolHandler: 
  function WCCR_registerProtocolHandler(aProtocol, aURIString, aTitle, aContentWindow) {
    LOG("registerProtocolHandler(" + aProtocol + "," + aURIString + "," + aTitle + ")");
    
    
    
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    var handler = ios.getProtocolHandler(aProtocol);
    if (!(handler instanceof Ci.nsIExternalProtocolHandler)) {
      
      
      
      throw("Permission denied to add " + aURIString + "as a protocol handler");
    }

    
    var pb = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    var allowed;
    try {
      allowed = pb.getBoolPref(PREF_HANDLER_EXTERNAL_PREFIX + "." + aProtocol);
    }
    catch (e) {
      allowed = pb.getBoolPref(PREF_HANDLER_EXTERNAL_PREFIX + "-default");
    }
    if (!allowed) {
      
      throw("Not allowed to register a protocol handler for " + aProtocol);
    }

    var uri = this._checkAndGetURI(aURIString, aContentWindow);

    var buttons, message;
    if (this._protocolHandlerRegistered(aProtocol, uri.spec))
      message = this._getFormattedString("protocolHandlerRegistered",
                                         [aTitle, aProtocol]);
    else {
      
      message = this._getFormattedString("addProtocolHandler",
                                         [aTitle, uri.host, aProtocol]);
      var fis = Cc["@mozilla.org/browser/favicon-service;1"].
                getService(Ci.nsIFaviconService);
      var notificationIcon = fis.getFaviconLinkForIcon(uri);
      var notificationValue = "Protocol Registration: " + aProtocol;
      var addButton = {
        label: this._getString("addProtocolHandlerAddButton"),
        accessKey: this._getString("addHandlerAddButtonAccesskey"),
        protocolInfo: { protocol: aProtocol, uri: uri.spec, name: aTitle },

        callback:
        function WCCR_addProtocolHandlerButtonCallback(aNotification, aButtonInfo) {
          var protocol = aButtonInfo.protocolInfo.protocol;
          var uri      = aButtonInfo.protocolInfo.uri;
          var name     = aButtonInfo.protocolInfo.name;

          var handler = Cc["@mozilla.org/uriloader/web-handler-app;1"].
                        createInstance(Ci.nsIWebHandlerApp);
          handler.name = name;
          handler.uriTemplate = uri;

          var eps = Cc["@mozilla.org/uriloader/external-protocol-service;1"].
                    getService(Ci.nsIExternalProtocolService);
          var handlerInfo = eps.getProtocolHandlerInfo(protocol);
          handlerInfo.possibleApplicationHandlers.appendElement(handler, false);

          
          
          
          
          handlerInfo.alwaysAskBeforeHandling = true;

          var hs = Cc["@mozilla.org/uriloader/handler-service;1"].
                   getService(Ci.nsIHandlerService);
          hs.store(handlerInfo);
        }
      };
      buttons = [addButton];
    }

    var browserWindow = this._getBrowserWindowForContentWindow(aContentWindow);
    var browserElement = this._getBrowserForContentWindow(browserWindow, aContentWindow);
    var notificationBox = browserWindow.getBrowser().getNotificationBox(browserElement);
    notificationBox.appendNotification(message,
                                       notificationValue,
                                       notificationIcon,
                                       notificationBox.PRIORITY_INFO_LOW,
                                       buttons);
  },

  




  registerContentHandler: 
  function WCCR_registerContentHandler(aContentType, aURIString, aTitle, aContentWindow) {
    LOG("registerContentHandler(" + aContentType + "," + aURIString + "," + aTitle + ")");

    
    
    
    var contentType = this._resolveContentType(aContentType);
    if (contentType != TYPE_MAYBE_FEED)
      return;

    if (aContentWindow) {
      var uri = this._checkAndGetURI(aURIString, aContentWindow);
  
      var browserWindow = this._getBrowserWindowForContentWindow(aContentWindow);
      var browserElement = this._getBrowserForContentWindow(browserWindow, aContentWindow);
      var notificationBox = browserWindow.getBrowser().getNotificationBox(browserElement);
      this._appendFeedReaderNotification(uri, aTitle, notificationBox);
    }
    else
      this._registerContentHandler(contentType, aURIString, aTitle);
  },

  


  _getBrowserWindowForContentWindow:
  function WCCR__getBrowserWindowForContentWindow(aContentWindow) {
    return aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShellTreeItem)
                         .rootTreeItem
                         .QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindow)
                         .wrappedJSObject;
  },

  









  _getBrowserForContentWindow:
  function WCCR__getBrowserForContentWindow(aBrowserWindow, aContentWindow) {
    
    aContentWindow = aContentWindow.top;
    var browsers = aBrowserWindow.getBrowser().browsers;
    for (var i = 0; i < browsers.length; ++i) {
      if (browsers[i].contentWindow == aContentWindow)
        return browsers[i];
    }
  },

  




















  _appendFeedReaderNotification:
  function WCCR__appendFeedReaderNotification(aURI, aName, aNotificationBox) {
    var uriSpec = aURI.spec;
    var notificationValue = "feed reader notification: " + uriSpec;
    var notificationIcon = aURI.prePath + "/favicon.ico";

    
    
    if (aNotificationBox.getNotificationWithValue(notificationValue))
      return false;

    var buttons, message;
    if (this.getWebContentHandlerByURI(TYPE_MAYBE_FEED, uriSpec))
      message = this._getFormattedString("handlerRegistered", [aName]);
    else {
      message = this._getFormattedString("addHandler", [aName, aURI.host]);
      var self = this;
      var addButton = {
        _outer: self,
        label: self._getString("addHandlerAddButton"),
        accessKey: self._getString("addHandlerAddButtonAccesskey"),
        feedReaderInfo: { uri: uriSpec, name: aName },

        
        callback:
        function WCCR__addFeedReaderButtonCallback(aNotification, aButtonInfo) {
          var uri = aButtonInfo.feedReaderInfo.uri;
          var name = aButtonInfo.feedReaderInfo.name;
          var outer = aButtonInfo._outer;

          
          if (!outer.getWebContentHandlerByURI(TYPE_MAYBE_FEED, uri))
            outer._registerContentHandler(TYPE_MAYBE_FEED, uri, name);

          
          aButtonInfo._outer = null;

          return false;
        }
      };
      buttons = [addButton];
    }

    aNotificationBox.appendNotification(message,
                                        notificationValue,
                                        notificationIcon,
                                        aNotificationBox.PRIORITY_INFO_LOW,
                                        buttons);
    return true;
  },

  














  _saveContentHandlerToPrefs: 
  function WCCR__saveContentHandlerToPrefs(contentType, uri, title) {
    var ps = 
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefService);
    var i = 0;
    var typeBranch = null;
    while (true) {
      typeBranch = 
        ps.getBranch(PREF_CONTENTHANDLERS_BRANCH + i + ".");
      try {
        typeBranch.getCharPref("type");
        ++i;
      }
      catch (e) {
        
        break;
      }
    }
    if (typeBranch) {
      typeBranch.setCharPref("type", contentType);
      var pls = 
          Cc["@mozilla.org/pref-localizedstring;1"].
          createInstance(Ci.nsIPrefLocalizedString);
      pls.data = uri;
      typeBranch.setComplexValue("uri", Ci.nsIPrefLocalizedString, pls);
      pls.data = title;
      typeBranch.setComplexValue("title", Ci.nsIPrefLocalizedString, pls);
    
      ps.savePrefFile(null);
    }
  },
  
  







  _typeIsRegistered: function WCCR__typeIsRegistered(contentType, uri) {
    if (!(contentType in this._contentTypes))
      return false;
      
    var services = this._contentTypes[contentType];
    for (var i = 0; i < services.length; ++i) {
      
      if (services[i].uri == uri)
        return true;
    }
    return false;
  },
  
  






  _getConverterContractID: function WCCR__getConverterContractID(contentType) {
    const template = "@mozilla.org/streamconv;1?from=%s&to=*/*";
    return template.replace(/%s/, contentType);
  },
  
  









  _registerContentHandler:
  function WCCR__registerContentHandler(contentType, uri, title) {
    this._updateContentTypeHandlerMap(contentType, uri, title);
    this._saveContentHandlerToPrefs(contentType, uri, title);

    if (contentType == TYPE_MAYBE_FEED) {
      
      
      var pb = Cc["@mozilla.org/preferences-service;1"].
               getService(Ci.nsIPrefService).getBranch(null);
      pb.setCharPref(PREF_SELECTED_READER, "web");
  
      var supportsString = 
        Cc["@mozilla.org/supports-string;1"].
        createInstance(Ci.nsISupportsString);
        supportsString.data = uri;
      pb.setComplexValue(PREF_SELECTED_WEB, Ci.nsISupportsString,
                         supportsString);
      pb.setCharPref(PREF_SELECTED_ACTION, "ask");
      this._setAutoHandler(TYPE_MAYBE_FEED, null);
    }
  },

  









  _updateContentTypeHandlerMap: 
  function WCCR__updateContentTypeHandlerMap(contentType, uri, title) {
    if (!(contentType in this._contentTypes))
      this._contentTypes[contentType] = [];

    
    if (this._typeIsRegistered(contentType, uri)) 
      return;
    
    this._contentTypes[contentType].push(new ServiceInfo(contentType, uri, title));
    
    if (!(contentType in this._blockedTypes)) {
      var converterContractID = this._getConverterContractID(contentType);
      var cr = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
      cr.registerFactory(WCC_CLASSID, WCC_CLASSNAME, converterContractID, 
                         WebContentConverterFactory);
    }
  },
  
  


  getContentHandlers: 
  function WCCR_getContentHandlers(contentType, countRef) {
    countRef.value = 0;
    if (!(contentType in this._contentTypes))
      return [];
    
    var handlers = this._contentTypes[contentType];
    countRef.value = handlers.length;
    return handlers;
  },
  
  


  resetHandlersForType: 
  function WCCR_resetHandlersForType(contentType) {
    
    
    
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  
  






  _registerContentHandlerWithBranch: function(branch) {
    







    var vals = branch.getChildList("", {});
    if (vals.length == 0)
      return;

    try {
      var type = branch.getCharPref("type");
      var uri = branch.getComplexValue("uri", Ci.nsIPrefLocalizedString).data;
      var title = branch.getComplexValue("title",
                                         Ci.nsIPrefLocalizedString).data;
      this._updateContentTypeHandlerMap(type, uri, title);
    }
    catch(ex) {
      
    }
  },

  



  _init: function WCCR__init() {
    var ps = 
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefService);

    var kids = ps.getBranch(PREF_CONTENTHANDLERS_BRANCH)
                 .getChildList("", {});

    
    var nums = [];
    for (var i = 0; i < kids.length; i++) {
      var match = /^(\d+)\.uri$/.exec(kids[i]);
      if (!match)
        continue;
      else
        nums.push(match[1]);
    }

    
    nums.sort(function(a, b) {return a - b;});

    
    for (var i = 0; i < nums.length; i++) {
      var branch = ps.getBranch(PREF_CONTENTHANDLERS_BRANCH + nums[i] + ".");
      this._registerContentHandlerWithBranch(branch);
    }

    
    
    try {
      var autoBranch = ps.getBranch(PREF_CONTENTHANDLERS_AUTO);
      var childPrefs = autoBranch.getChildList("", { });
      for (var i = 0; i < childPrefs.length; ++i) {
        var type = childPrefs[i];
        var uri = autoBranch.getCharPref(type);
        if (uri) {
          var handler = this.getWebContentHandlerByURI(type, uri);
          this._setAutoHandler(type, handler);
        }
      }
    }
    catch (e) {
      
      
    }
  },

  


  observe: function WCCR_observe(subject, topic, data) {
    var os = 
        Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
    switch (topic) {
    case "app-startup":
      os.addObserver(this, "browser-ui-startup-complete", false);
      break;
    case "browser-ui-startup-complete":
      os.removeObserver(this, "browser-ui-startup-complete");
      this._init();
      break;
    }
  },
  
  


  createInstance: function WCCR_createInstance(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return this.QueryInterface(iid);
  },

  


  getInterfaces: function WCCR_getInterfaces(countRef) {
    var interfaces = 
        [Ci.nsIWebContentConverterService, Ci.nsIWebContentHandlerRegistrar,
         Ci.nsIObserver, Ci.nsIClassInfo, Ci.nsIFactory, Ci.nsISupports];
    countRef.value = interfaces.length;
    return interfaces;
  },
  getHelperForLanguage: function WCCR_getHelperForLanguage(language) {
    return null;
  },
  contractID: WCCR_CONTRACTID,
  classDescription: WCCR_CLASSNAME,
  classID: WCCR_CLASSID,
  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
  flags: Ci.nsIClassInfo.DOM_OBJECT,
  
  


  QueryInterface: XPCOMUtils.generateQI(
     [Ci.nsIWebContentConverterService, 
      Ci.nsIWebContentHandlerRegistrar,
      Ci.nsIObserver,
      Ci.nsIClassInfo,
      Ci.nsIFactory,
      Ci.nsISupports]),

  _xpcom_categories: [{
    category: "app-startup",
    service: true
  }]
};

function NSGetModule(cm, file) {
  return XPCOMUtils.generateModule([WebContentConverterRegistrar]);
}

