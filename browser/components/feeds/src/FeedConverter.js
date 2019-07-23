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
# The Original Code is the Feed Stream Converter.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <beng@google.com>
#   Jeff Walden <jwalden+code@mit.edu>
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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function LOG(str) {
  dump("*** " + str + "\n");
}

const FC_CLASSID = Components.ID("{229fa115-9412-4d32-baf3-2fc407f76fb1}");
const FC_CLASSNAME = "Feed Stream Converter";
const FS_CLASSID = Components.ID("{2376201c-bbc6-472f-9b62-7548040a61c6}");
const FS_CLASSNAME = "Feed Result Service";
const FS_CONTRACTID = "@mozilla.org/browser/feeds/result-service;1";
const FPH_CONTRACTID = "@mozilla.org/network/protocol;1?name=feed";
const FPH_CLASSID = Components.ID("{4f91ef2e-57ba-472e-ab7a-b4999e42d6c0}");
const FPH_CLASSNAME = "Feed Protocol Handler";
const PCPH_CONTRACTID = "@mozilla.org/network/protocol;1?name=pcast";
const PCPH_CLASSID = Components.ID("{1c31ed79-accd-4b94-b517-06e0c81999d5}");
const PCPH_CLASSNAME = "Podcast Protocol Handler";
const FHS_CONTRACTID = "@mozilla.org/browser/feeds/handler-service;1";
const FHS_CLASSID = Components.ID("{792a7e82-06a0-437c-af63-b2d12e808acc}");
const FHS_CLASSNAME = "Feed Handler Service";

const TYPE_MAYBE_FEED = "application/vnd.mozilla.maybe.feed";
const TYPE_ANY = "*/*";

const FEEDHANDLER_URI = "about:feeds";

const PREF_SELECTED_APP = "browser.feeds.handlers.application";
const PREF_SELECTED_WEB = "browser.feeds.handlers.webservice";
const PREF_SELECTED_ACTION = "browser.feeds.handler";
const PREF_SELECTED_READER = "browser.feeds.handler.default";

function safeGetBoolPref(pref, defaultValue) {
  var prefs =   
      Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch);
  try {
    return prefs.getBoolPref(pref);
  }
  catch (e) {
  }
  return defaultValue;
}

function safeGetCharPref(pref, defaultValue) {
  var prefs =   
      Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch);
  try {
    return prefs.getCharPref(pref);
  }
  catch (e) {
  }
  return defaultValue;
}

function FeedConverter() {
}
FeedConverter.prototype = {
  


  _data: null,
  
  



  _listener: null,

  


  _sniffed: false,
  
  


  canConvert: function FC_canConvert(sourceType, destinationType) {
    
    return destinationType == TYPE_ANY && sourceType == TYPE_MAYBE_FEED;
  },
  
  


  convert: function FC_convert(sourceStream, sourceType, destinationType, 
                               context) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  
  


  asyncConvertData: function FC_asyncConvertData(sourceType, destinationType,
                                                 listener, context) {
    this._listener = listener;
  },
  
  


  _forcePreviewPage: false,
  
  


  _releaseHandles: function FC__releaseHandles() {
    this._listener = null;
    this._request = null;
  },
  
  


  handleResult: function FC_handleResult(result) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    try {
      var feedService = 
          Cc["@mozilla.org/browser/feeds/result-service;1"].
          getService(Ci.nsIFeedResultService);
      if (!this._forcePreviewPage && result.doc) {
        var handler = safeGetCharPref(PREF_SELECTED_ACTION, "ask");
        if (handler != "ask") {
          if (handler == "reader")
            handler = safeGetCharPref(PREF_SELECTED_READER, "bookmarks");
          switch (handler) {
            case "web":
              var wccr = 
                  Cc["@mozilla.org/embeddor.implemented/web-content-handler-registrar;1"].
                  getService(Ci.nsIWebContentConverterService);
              var feed = result.doc.QueryInterface(Ci.nsIFeed);
              if (feed.type == Ci.nsIFeed.TYPE_FEED &&
                  wccr.getAutoHandler(TYPE_MAYBE_FEED)) {
                wccr.loadPreferredHandler(this._request);
                return;
              }
              break;

            default:
              LOG("unexpected handler: " + handler);
              
            case "bookmarks":
            case "client":
              try {
                var feed = result.doc.QueryInterface(Ci.nsIFeed);
                var title = feed.title ? feed.title.plainText() : "";
                var desc = feed.subtitle ? feed.subtitle.plainText() : "";
                feedService.addToClientReader(result.uri.spec, title, desc);
                return;
              } catch(ex) {  }
          }
        }
      }
          
      var ios = 
          Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);
      var chromeChannel;

      
      
      if (result.doc && (!this._sniffed ||
          (result.doc.title && (result.doc.link || result.doc.id)))) {

        
        
        
        
        
        

        feedService.addFeedResult(result);

        
        var chromeURI = ios.newURI(FEEDHANDLER_URI, null, null);
        chromeChannel = ios.newChannelFromURI(chromeURI, null);
        chromeChannel.originalURI = result.uri;
      }
      else
        chromeChannel = ios.newChannelFromURI(result.uri, null);

      chromeChannel.loadGroup = this._request.loadGroup;
      chromeChannel.asyncOpen(this._listener, null);
    }
    finally {
      this._releaseHandles();
    }
  },
  
  


  onDataAvailable: function FC_onDataAvailable(request, context, inputStream, 
                                               sourceOffset, count) {
    this._processor.onDataAvailable(request, context, inputStream,
                                    sourceOffset, count);
  },
  
  


  onStartRequest: function FC_onStartRequest(request, context) {
    var channel = request.QueryInterface(Ci.nsIChannel);

    
    
    try {
      var httpChannel = channel.QueryInterface(Ci.nsIHttpChannel);
      var noSniff = httpChannel.getResponseHeader("X-Moz-Is-Feed");
    }
    catch (ex) {
      this._sniffed = true;
    }

    this._request = request;
    
    
    
    var feedService = 
        Cc["@mozilla.org/browser/feeds/result-service;1"].
        getService(Ci.nsIFeedResultService);
    this._forcePreviewPage = feedService.forcePreviewPage;
    feedService.forcePreviewPage = false;

    
    this._processor =
        Cc["@mozilla.org/feed-processor;1"].
        createInstance(Ci.nsIFeedProcessor);
    this._processor.listener = this;
    this._processor.parseAsync(null, channel.URI);
    
    this._processor.onStartRequest(request, context);
  },
  
  


  onStopRequest: function FC_onStopReqeust(request, context, status) {
    this._processor.onStopRequest(request, context, status);
  },
  
  


  QueryInterface: function FC_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFeedResultListener) ||
        iid.equals(Ci.nsIStreamConverter) ||
        iid.equals(Ci.nsIStreamListener) ||
        iid.equals(Ci.nsIRequestObserver)||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
};

var FeedConverterFactory = {
  createInstance: function FS_createInstance(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return new FeedConverter().QueryInterface(iid);
  },

  QueryInterface: function FS_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFactory) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
};





var FeedResultService = {
  
  



  _results: { },
  
  


  forcePreviewPage: false,
  
  


  addToClientReader: function FRS_addToClientReader(spec, title, subtitle) {
    var prefs =   
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);

    var handler = safeGetCharPref(PREF_SELECTED_ACTION, "bookmarks");
    if (handler == "ask" || handler == "reader")
      handler = safeGetCharPref(PREF_SELECTED_READER, "bookmarks");

    switch (handler) {
    case "client":
      var clientApp = prefs.getComplexValue(PREF_SELECTED_APP, Ci.nsILocalFile);
#ifdef XP_MACOSX
      
      
      
      
      
      
      
      
      
      var ios = 
          Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);
      var macURI = ios.newURI(spec, null, null);
      if (macURI.schemeIs("http")) {
        macURI.scheme = "feed";
        spec = macURI.spec;
      }
      else
        spec = "feed:" + spec;
#endif
      var ss = 
          Cc["@mozilla.org/browser/shell-service;1"].
          getService(Ci.nsIShellService);
      ss.openApplicationWithURI(clientApp, spec);
      break;

    default:
      
      LOG("unexpected handler: " + handler);
      
    case "bookmarks":
      var wm = 
          Cc["@mozilla.org/appshell/window-mediator;1"].
          getService(Ci.nsIWindowMediator);
      var topWindow = wm.getMostRecentWindow("navigator:browser");
#ifdef MOZ_PLACES_BOOKMARKS
      topWindow.PlacesCommandHook.addLiveBookmark(spec, title, subtitle);
#else
      topWindow.FeedHandler.addLiveBookmark(spec, title, subtitle);
#endif
      break;
    }
  },
  
  


  addFeedResult: function FRS_addFeedResult(feedResult) {
    NS_ASSERT(feedResult.uri != null, "null URI!");
    NS_ASSERT(feedResult.uri != null, "null feedResult!");
    var spec = feedResult.uri.spec;
    if(!this._results[spec])  
      this._results[spec] = [];
    this._results[spec].push(feedResult);
  },
  
  


  getFeedResult: function RFS_getFeedResult(uri) {
    NS_ASSERT(uri != null, "null URI!");
    var resultList = this._results[uri.spec];
    for (var i in resultList) {
      if (resultList[i].uri == uri)
        return resultList[i];
    }
    return null;
  },
  
  


  removeFeedResult: function FRS_removeFeedResult(uri) {
    NS_ASSERT(uri != null, "null URI!");
    var resultList = this._results[uri.spec];
    if (!resultList)
      return;
    var deletions = 0;
    for (var i = 0; i < resultList.length; ++i) {
      if (resultList[i].uri == uri) {
        delete resultList[i];
        ++deletions;
      }
    }
    
    
    resultList.sort();
    
    resultList.splice(resultList.length - deletions, deletions);
    if (resultList.length == 0)
      delete this._results[uri.spec];
  },

  createInstance: function FRS_createInstance(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return this.QueryInterface(iid);
  },
  
  QueryInterface: function FRS_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFeedResultService) ||
        iid.equals(Ci.nsIFactory) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
};






function FeedProtocolHandler(scheme) {
  this._scheme = scheme;
  var ios = 
      Cc["@mozilla.org/network/io-service;1"].
      getService(Ci.nsIIOService);
  this._http = ios.getProtocolHandler("http");
}
FeedProtocolHandler.prototype = {
  _scheme: "",
  get scheme() {
    return this._scheme;
  },
  
  get protocolFlags() {
    return this._http.protocolFlags;
  },
  
  get defaultPort() {
    return this._http.defaultPort;
  },
  
  allowPort: function FPH_allowPort(port, scheme) {
    return this._http.allowPort(port, scheme);
  },
  
  newURI: function FPH_newURI(spec, originalCharset, baseURI) {
    var uri = 
        Cc["@mozilla.org/network/standard-url;1"].
        createInstance(Ci.nsIStandardURL);
    uri.init(Ci.nsIStandardURL.URLTYPE_STANDARD, 80, spec, originalCharset,
             baseURI);
    return uri;
  },
  
  newChannel: function FPH_newChannel(uri) {
    var ios = 
        Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService);
    
    
    const httpsChunk = "feed:
    const httpChunk = "feed:
    if (uri.spec.substr(0, httpsChunk.length) == httpsChunk)
      uri.spec = "https:
    else if (uri.spec.substr(0, httpChunk.length) == httpChunk)
      uri.spec = "http:
    else
      uri.scheme = "http";

    var channel =
      ios.newChannelFromURI(uri, null).QueryInterface(Ci.nsIHttpChannel);
    
    channel.setRequestHeader("X-Moz-Is-Feed", "1", false);
    channel.originalURI = uri;
    return channel;
  },
  
  QueryInterface: function FPH_QueryInterface(iid) {
    if (iid.equals(Ci.nsIProtocolHandler) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }  
};

var Module = {
  QueryInterface: function M_QueryInterface(iid) {
    if (iid.equals(Ci.nsIModule) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  
  getClassObject: function M_getClassObject(cm, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    
    if (cid.equals(FS_CLASSID))
      return FeedResultService;
    if (cid.equals(FPH_CLASSID))
      return new GenericComponentFactory(FeedProtocolHandler, "feed");
    if (cid.equals(PCPH_CLASSID))
      return new GenericComponentFactory(FeedProtocolHandler, "pcast");
    if (cid.equals(FC_CLASSID))
      return new GenericComponentFactory(FeedConverter);
      
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  
  registerSelf: function M_registerSelf(cm, file, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    
    cr.registerFactoryLocation(FS_CLASSID, FS_CLASSNAME, FS_CONTRACTID,
                               file, location, type);
    cr.registerFactoryLocation(FPH_CLASSID, FPH_CLASSNAME, FPH_CONTRACTID,
                               file, location, type);
    cr.registerFactoryLocation(PCPH_CLASSID, PCPH_CLASSNAME, PCPH_CONTRACTID,
                               file, location, type);

    
    
    const converterPrefix = "@mozilla.org/streamconv;1?from=";
    var converterContractID = 
        converterPrefix + TYPE_MAYBE_FEED + "&to=" + TYPE_ANY;
    cr.registerFactoryLocation(FC_CLASSID, FC_CLASSNAME, converterContractID,
                               file, location, type);
  },
  
  unregisterSelf: function M_unregisterSelf(cm, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    cr.unregisterFactoryLocation(FPH_CLASSID, location);
    cr.unregisterFactoryLocation(PCPH_CLASSID, location);
  },
  
  canUnload: function M_canUnload(cm) {
    return true;
  }
};

function NSGetModule(cm, file) {
  return Module;
}

#include ../../../../toolkit/content/debug.js
#include GenericFactory.js
