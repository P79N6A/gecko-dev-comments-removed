













































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

#include ../../url-classifier/content/moz/lang.js
#include ../../url-classifier/content/moz/observer.js
#include ../../url-classifier/content/moz/alarm.js

const LS_CLASSID = Components.ID("{dca61eb5-c7cd-4df1-b0fb-d0722baba251}");
const LS_CLASSNAME = "Livemark Service";
const LS_CONTRACTID = "@mozilla.org/browser/livemark-service;2";

const LMANNO_FEEDURI = "livemark/feedURI";
const LMANNO_SITEURI = "livemark/siteURI";
const LMANNO_EXPIRATION = "livemark/expiration";
const LMANNO_LOADFAILED = "livemark/loadfailed";
const LMANNO_LOADING = "livemark/loading";

const PS_CONTRACTID = "@mozilla.org/preferences-service;1";
const NH_CONTRACTID = "@mozilla.org/browser/nav-history-service;1";
const AS_CONTRACTID = "@mozilla.org/browser/annotation-service;1";
const OS_CONTRACTID = "@mozilla.org/observer-service;1";
const SB_CONTRACTID = "@mozilla.org/intl/stringbundle;1";
const IO_CONTRACTID = "@mozilla.org/network/io-service;1";
const BMS_CONTRACTID = "@mozilla.org/browser/nav-bookmarks-service;1";
const FAV_CONTRACTID = "@mozilla.org/browser/favicon-service;1";
const LG_CONTRACTID = "@mozilla.org/network/load-group;1";
const FP_CONTRACTID = "@mozilla.org/feed-processor;1";
const SEC_CONTRACTID = "@mozilla.org/scriptsecuritymanager;1";
const IS_CONTRACTID = "@mozilla.org/widget/idleservice;1";
const SEC_FLAGS = Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;


var gExpiration = 3600000;


var gLimitCount = 1;


var gDelayTime  = 3;


const ERROR_EXPIRATION = 600000;


const IDLE_TIMELIMIT = 1800000;



const MAX_REFRESH_TIME = 3600000;

function MarkLivemarkLoadFailed(aFolderId) {
  var ans = Cc[AS_CONTRACTID].getService(Ci.nsIAnnotationService);
  
  if (ans.itemHasAnnotation(aFolderId, LMANNO_LOADFAILED))
    return;

  
  ans.removeItemAnnotation(aFolderId, LMANNO_LOADING);
  ans.setItemAnnotation(aFolderId, LMANNO_LOADFAILED, true,
                        0, ans.EXPIRE_NEVER);
}

function LivemarkService() {

  try {
    var prefs = Cc[PS_CONTRACTID].getService(Ci.nsIPrefBranch);
    var livemarkRefresh =
      prefs.getIntPref("browser.bookmarks.livemark_refresh_seconds");
    
    
    gExpiration = Math.max(livemarkRefresh * 1000, 60000);
  }
  catch (ex) { }

  try {
    gLimitCount = prefs.getIntPref("browser.bookmarks.livemark_refresh_limit_count");
    if ( gLimitCount < 1 ) gLimitCount = 1;
  }
  catch (ex) { }

  try {
    gDelayTime = prefs.getIntPref("browser.bookmarks.livemark_refresh_delay_time");
    if ( gDelayTime < 1 ) gDelayTime = 1;
  }
  catch (ex) { }

  
  

  XPCOMUtils.defineLazyServiceGetter(this, "_bms", BMS_CONTRACTID,
                                     "nsINavBookmarksService");

  XPCOMUtils.defineLazyServiceGetter(this, "_history", NH_CONTRACTID,
                                     "nsINavHistoryService");

  XPCOMUtils.defineLazyServiceGetter(this, "_ans", AS_CONTRACTID,
                                     "nsIAnnotationService");

  XPCOMUtils.defineLazyServiceGetter(this, "_ios", IO_CONTRACTID,
                                     "nsIIOService");

  XPCOMUtils.defineLazyServiceGetter(this, "_idleService", IS_CONTRACTID,
                                     "nsIIdleService");


  
  this._livemarks = [];

  this._observerServiceObserver =
    new G_ObserverServiceObserver('xpcom-shutdown',
                                  BindToObject(this._shutdown, this),
                                  true );

  var livemarks = this._ans.getItemsWithAnnotation(LMANNO_FEEDURI, {});
  for (var i = 0; i < livemarks.length; i++) {
    var feedURI = this._ios.newURI(this._ans.getItemAnnotation(livemarks[i],
                                                               LMANNO_FEEDURI),
                                   null, null);
    this._pushLivemark(livemarks[i], feedURI);
  }

  this._bms.addObserver(this, false);
}

LivemarkService.prototype = {

  _updateTimer: null,
  start: function LS_start() {
    if (this._updateTimer)
      return;
    
    
    
    this._checkAllLivemarks();
  },

  stopUpdateLivemarks: function LS_stopUpdateLivemarks() {
    for (var livemark in this._livemarks) {
      if (livemark.loadGroup)
        livemark.loadGroup.cancel(Components.results.NS_BINDING_ABORTED);
    }
    
    if (this._updateTimer) {
      this._updateTimer.cancel();
      this._updateTimer = null;
    }
  },

  _pushLivemark: function LS__pushLivemark(aFolderId, aFeedURI) {
    
    return this._livemarks.push({folderId: aFolderId, feedURI: aFeedURI});
  },

  _getLivemarkIndex: function LS__getLivemarkIndex(aFolderId) {
    for (var i = 0; i < this._livemarks.length; ++i) {
      if (this._livemarks[i].folderId == aFolderId)
        return i;
    }
    throw Cr.NS_ERROR_INVALID_ARG;
  },

  _shutdown: function LS__shutdown() {
    
    this._bms.removeObserver(this);

    
    this.stopUpdateLivemarks();
  },

  
  
  _nextUpdateStartIndex : 0,
  _checkAllLivemarks: function LS__checkAllLivemarks() {
    var startNo = this._nextUpdateStartIndex;
    var count = 0;
    for (var i = startNo; (i < this._livemarks.length) && (count < gLimitCount); ++i ) {
      
      try {
        if (this._updateLivemarkChildren(i, false)) count++;
      }
      catch (ex) { }
      this._nextUpdateStartIndex = i+1;
    }
    if ( this._nextUpdateStartIndex >= this._livemarks.length ) {
      
      this._nextUpdateStartIndex = 0;
      var refresh_time = Math.min(Math.floor(gExpiration / 4), MAX_REFRESH_TIME);
      this._updateTimer = new G_Alarm(BindToObject(this._checkAllLivemarks, this),
                                      refresh_time);
    } else {
      
      this._updateTimer = new G_Alarm(BindToObject(this._checkAllLivemarks, this),
                                      gDelayTime*1000);
    }
  },

  deleteLivemarkChildren: function LS_deleteLivemarkChildren(aFolderId) {
    this._bms.removeFolderChildren(aFolderId);
  },

  _updateLivemarkChildren:
  function LS__updateLivemarkChildren(aIndex, aForceUpdate) {
    if (this._livemarks[aIndex].locked)
      return false;

    var livemark = this._livemarks[aIndex];
    livemark.locked = true;
    try {
      
      
      
      
      var expireTime = this._ans.getItemAnnotation(livemark.folderId,
                                                   LMANNO_EXPIRATION);
      if (!aForceUpdate && expireTime > Date.now()) {
        
        livemark.locked = false;
        return false;
      }

      
      
      
      
      var idleTime = 0;
      try {
        idleTime = this._idleService.idleTime;
      }
      catch (ex) {  }
      if (idleTime > IDLE_TIMELIMIT) {
        livemark.locked = false;
        return false;
      }
    }
    catch (ex) {
      
    }

    var loadgroup;
    try {
      
      
      
      loadgroup = Cc[LG_CONTRACTID].createInstance(Ci.nsILoadGroup);
      var uriChannel = this._ios.newChannel(livemark.feedURI.spec, null, null);
      uriChannel.loadGroup = loadgroup;
      uriChannel.loadFlags |= Ci.nsIRequest.LOAD_BACKGROUND |
                              Ci.nsIRequest.VALIDATE_ALWAYS;
      var httpChannel = uriChannel.QueryInterface(Ci.nsIHttpChannel);
      httpChannel.requestMethod = "GET";
      httpChannel.setRequestHeader("X-Moz", "livebookmarks", false);

      
      var listener = new LivemarkLoadListener(livemark);
      
      this._ans.removeItemAnnotation(livemark.folderId, LMANNO_LOADFAILED);
      this._ans.setItemAnnotation(livemark.folderId, LMANNO_LOADING, true,
                                  0, this._ans.EXPIRE_NEVER);
      httpChannel.notificationCallbacks = listener;
      httpChannel.asyncOpen(listener, null);
    }
    catch (ex) {
      MarkLivemarkLoadFailed(livemark.folderId);
      livemark.locked = false;
      return false;
    }
    livemark.loadGroup = loadgroup;
    return true;
  },

  createLivemark: function LS_createLivemark(aParentId, aName, aSiteURI,
                                             aFeedURI, aIndex) {
    if (!aParentId || !aFeedURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    
    if (this.isLivemark(aParentId))
      throw Cr.NS_ERROR_INVALID_ARG;

    var folderId = this._createFolder(aParentId, aName, aSiteURI,
                                      aFeedURI, aIndex);

    
    this._updateLivemarkChildren(this._pushLivemark(folderId, aFeedURI) - 1,
                                 false);

    return folderId;
  },

  createLivemarkFolderOnly:
  function LS_createLivemarkFolderOnly(aParentId, aName, aSiteURI,
                                       aFeedURI, aIndex) {
    if (aParentId < 1 || !aFeedURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    
    if (this.isLivemark(aParentId))
      throw Cr.NS_ERROR_INVALID_ARG;

    var folderId = this._createFolder(aParentId, aName, aSiteURI,
                                      aFeedURI, aIndex);

    var livemarkIndex = this._pushLivemark(folderId, aFeedURI) - 1;
    var livemark = this._livemarks[livemarkIndex];
    return folderId;
  },

  _createFolder:
  function LS__createFolder(aParentId, aName, aSiteURI, aFeedURI, aIndex) {
    var folderId = this._bms.createFolder(aParentId, aName, aIndex);
    this._bms.setFolderReadonly(folderId, true);

    
    this._ans.setItemAnnotation(folderId, LMANNO_FEEDURI, aFeedURI.spec,
                                0, this._ans.EXPIRE_NEVER);

    if (aSiteURI) {
      
      this._setSiteURISecure(folderId, aFeedURI, aSiteURI);
    }

    return folderId;
  },

  isLivemark: function LS_isLivemark(aFolderId) {
    if (aFolderId < 1)
      throw Cr.NS_ERROR_INVALID_ARG;
    try {
      this._getLivemarkIndex(aFolderId);
      return true;
    }
    catch (ex) {}
    return false;
  },

  getLivemarkIdForFeedURI: function LS_getLivemarkIdForFeedURI(aFeedURI) {
    if (!(aFeedURI instanceof Ci.nsIURI))
      throw Cr.NS_ERROR_INVALID_ARG;

    for (var i = 0; i < this._livemarks.length; ++i) {
      if (this._livemarks[i].feedURI.equals(aFeedURI))
        return this._livemarks[i].folderId;
    }

    return -1;
  },

  _ensureLivemark: function LS__ensureLivemark(aFolderId) {
    if (!this.isLivemark(aFolderId))
      throw Cr.NS_ERROR_INVALID_ARG;
  },

  getSiteURI: function LS_getSiteURI(aFolderId) {
    this._ensureLivemark(aFolderId);

    if (this._ans.itemHasAnnotation(aFolderId, LMANNO_SITEURI)) {
      var siteURIString =
        this._ans.getItemAnnotation(aFolderId, LMANNO_SITEURI);

      return this._ios.newURI(siteURIString, null, null);
    }
    return null;
  },

  setSiteURI: function LS_setSiteURI(aFolderId, aSiteURI) {
    this._ensureLivemark(aFolderId);

    if (!aSiteURI) {
      this._ans.removeItemAnnotation(aFolderId, LMANNO_SITEURI);
      return;
    }

    var livemarkIndex = this._getLivemarkIndex(aFolderId);
    var livemark = this._livemarks[livemarkIndex];
    this._setSiteURISecure(aFolderId, livemark.feedURI, aSiteURI);
  },

  _setSiteURISecure:
  function LS__setSiteURISecure(aFolderId, aFeedURI, aSiteURI) {
    var secMan = Cc[SEC_CONTRACTID].getService(Ci.nsIScriptSecurityManager);
    var feedPrincipal = secMan.getCodebasePrincipal(aFeedURI);
    try {
      secMan.checkLoadURIWithPrincipal(feedPrincipal, aSiteURI, SEC_FLAGS);
    }
    catch (e) {
      return;
    }
    this._ans.setItemAnnotation(aFolderId, LMANNO_SITEURI, aSiteURI.spec,
                                0, this._ans.EXPIRE_NEVER);
  },

  getFeedURI: function LS_getFeedURI(aFolderId) {
    if (this._ans.itemHasAnnotation(aFolderId, LMANNO_FEEDURI))
      return this._ios.newURI(this._ans.getItemAnnotation(aFolderId,
                                                          LMANNO_FEEDURI),
                              null, null);
    return null;
  },

  setFeedURI: function LS_setFeedURI(aFolderId, aFeedURI) {
    if (!aFeedURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    this._ans.setItemAnnotation(aFolderId, LMANNO_FEEDURI, aFeedURI.spec, 0,
                                this._ans.EXPIRE_NEVER);

    
    var livemarkIndex = this._getLivemarkIndex(aFolderId);
    this._livemarks[livemarkIndex].feedURI = aFeedURI;
  },

  reloadAllLivemarks: function LS_reloadAllLivemarks() {
    for (var i = 0; i < this._livemarks.length; ++i) {
      this._updateLivemarkChildren(i, true);
    }
  },

  reloadLivemarkFolder: function LS_reloadLivemarkFolder(aFolderId) {
    var livemarkIndex = this._getLivemarkIndex(aFolderId);
    this._updateLivemarkChildren(livemarkIndex, true);
  },

  
  onBeginUpdateBatch: function() { },
  onEndUpdateBatch: function() { },
  onItemAdded: function() { },
  onItemChanged: function() { },
  onItemVisited: function() { },
  onItemMoved: function() { },
  onBeforeItemRemoved: function() { },

  onItemRemoved: function(aItemId, aParentId, aIndex) {
    
    
    try {
      var livemarkIndex = this._getLivemarkIndex(aItemId);
    }
    catch(ex) {
      
      return;
    }
    var livemark = this._livemarks[livemarkIndex];

    
    this._livemarks.splice(livemarkIndex, 1);

    if (livemark.loadGroup)
      livemark.loadGroup.cancel(Components.results.NS_BINDING_ABORTED);
  },

  createInstance: function LS_createInstance(aOuter, aIID) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return this.QueryInterface(aIID);
  },

  QueryInterface: function LS_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsILivemarkService) ||
        aIID.equals(Ci.nsIFactory) ||
        aIID.equals(Ci.nsINavBookmarkObserver) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
};

function LivemarkLoadListener(aLivemark) {
  this._livemark = aLivemark;
  this._processor = null;
  this._isAborted = false;
  this._ttl = gExpiration;
}

LivemarkLoadListener.prototype = {

  abort: function LLL_abort() {
    this._isAborted = true;
  },

  get _bms() {
    if (!this.__bms)
      this.__bms = Cc[BMS_CONTRACTID].getService(Ci.nsINavBookmarksService);
    return this.__bms;
  },

  get _history() {
    if (!this.__history)
      this.__history = Cc[NH_CONTRACTID].getService(Ci.nsINavHistoryService);
    return this.__history;
  },

  get _ans() {
    if (!this.__ans)
      this.__ans = Cc[AS_CONTRACTID].getService(Ci.nsIAnnotationService);
    return this.__ans;
  },

  
  runBatched: function LLL_runBatched(aUserData) {
    var result = aUserData.QueryInterface(Ci.nsIFeedResult);

    
    var secMan = Cc[SEC_CONTRACTID].getService(Ci.nsIScriptSecurityManager);
    var feedPrincipal = secMan.getCodebasePrincipal(this._livemark.feedURI);

    var lmService = Cc[LS_CONTRACTID].getService(Ci.nsILivemarkService);

    
    if (!result || !result.doc || result.bozo) {
      MarkLivemarkLoadFailed(this._livemark.folderId);
      this._ttl = gExpiration;
      throw Cr.NS_ERROR_FAILURE;
    }

    
    
    this.deleteLivemarkChildren(this._livemark.folderId);
    var feed = result.doc.QueryInterface(Ci.nsIFeed);
    if (feed.link) {
      var oldSiteURI = lmService.getSiteURI(this._livemark.folderId);
      if (!oldSiteURI || !feed.link.equals(oldSiteURI))
        lmService.setSiteURI(this._livemark.folderId, feed.link);
    }
    
    
    for (var i = 0; i < feed.items.length; ++i) {
      let entry = feed.items.queryElementAt(i, Ci.nsIFeedEntry);
      let href = entry.link;
      if (!href)
        continue;

      let title = entry.title ? entry.title.plainText() : "";

      try {
        secMan.checkLoadURIWithPrincipal(feedPrincipal, href, SEC_FLAGS);
      }
      catch(ex) {
        continue;
      }

      this.insertLivemarkChild(this._livemark.folderId, href, title);
    }
  },

  


  handleResult: function LLL_handleResult(aResult) {
    if (this._isAborted) {
      MarkLivemarkLoadFailed(this._livemark.folderId);
      this._livemark.locked = false;
      return;
    }
    try {
      
      this._bms.runInBatchMode(this, aResult);
    }
    finally {
      this._processor.listener = null;
      this._processor = null;
      this._livemark.locked = false;
      this._ans.removeItemAnnotation(this._livemark.folderId, LMANNO_LOADING);
    }
  },

  deleteLivemarkChildren: LivemarkService.prototype.deleteLivemarkChildren,

  insertLivemarkChild:
  function LS_insertLivemarkChild(aFolderId, aUri, aTitle) {
    this._bms.insertBookmark(aFolderId, aUri, this._bms.DEFAULT_INDEX, aTitle);
  },

  


  onDataAvailable: function LLL_onDataAvailable(aRequest, aContext, aInputStream,
                                                aSourceOffset, aCount) {
    if (this._processor)
      this._processor.onDataAvailable(aRequest, aContext, aInputStream,
                                      aSourceOffset, aCount);
  },

  


  onStartRequest: function LLL_onStartRequest(aRequest, aContext) {
    if (this._isAborted)
      throw Cr.NS_ERROR_UNEXPECTED;

    var channel = aRequest.QueryInterface(Ci.nsIChannel);

    
    this._processor = Cc[FP_CONTRACTID].createInstance(Ci.nsIFeedProcessor);
    this._processor.listener = this;
    this._processor.parseAsync(null, channel.URI);

    try {
      this._processor.onStartRequest(aRequest, aContext);
    }
    catch (ex) {
      Components.utils.reportError("Livemark Service: feed processor received an invalid channel for " + channel.URI.spec);
    }
  },

  


  onStopRequest: function LLL_onStopRequest(aRequest, aContext, aStatus) {
    if (!Components.isSuccessCode(aStatus)) {
      this._isAborted = true;
      this._livemark.locked = false;
      var lmService = Cc[LS_CONTRACTID].getService(Ci.nsILivemarkService);
      
      
      if (lmService.isLivemark(this._livemark.folderId)) {
        
        this._setResourceTTL(ERROR_EXPIRATION);
        MarkLivemarkLoadFailed(this._livemark.folderId);
      }
      return;
    }
    
    try {
      if (this._processor)
        this._processor.onStopRequest(aRequest, aContext, aStatus);

      
      var channel = aRequest.QueryInterface(Ci.nsICachingChannel);
      if (channel) {
        var entryInfo = channel.cacheToken.QueryInterface(Ci.nsICacheEntryInfo);
        if (entryInfo) {
          
          
          var expireTime = entryInfo.expirationTime * 1000;
          var nowTime = Date.now();

          
          if (expireTime > nowTime) {
            this._setResourceTTL(Math.max((expireTime - nowTime),
                                 gExpiration));
            return;
          }
        }
      }
    }
    catch (ex) { }
    this._setResourceTTL(this._ttl);
  },

  _setResourceTTL: function LLL__setResourceTTL(aMilliseconds) {
    var expireTime = Date.now() + aMilliseconds;
    this._ans.setItemAnnotation(this._livemark.folderId, LMANNO_EXPIRATION,
                                expireTime, 0,
                                Ci.nsIAnnotationService.EXPIRE_NEVER);
  },

  


  notifyCertProblem: function LLL_certProblem(aSocketInfo, aStatus, aTargetSite) {
    return true;
  },

  


  notifySSLError: function LLL_SSLError(aSocketInfo, aError, aTargetSite) {
    return true;
  },

  


  getInterface: function LLL_getInterface(aIID) {
    return this.QueryInterface(aIID);
  },

  


  QueryInterface: function LLL_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIFeedResultListener) ||
        aIID.equals(Ci.nsIStreamListener) ||
        aIID.equals(Ci.nsIRequestObserver)||
        aIID.equals(Ci.nsINavHistoryBatchCallback) ||
        aIID.equals(Ci.nsIBadCertListener2) ||
        aIID.equals(Ci.nsISSLErrorListener) ||
        aIID.equals(Ci.nsIInterfaceRequestor) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
}

function GenericComponentFactory(aCtor) {
  this._ctor = aCtor;
}

GenericComponentFactory.prototype = {

  _ctor: null,

  
  createInstance: function(aOuter, aIID) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return (new this._ctor()).QueryInterface(aIID);
  },

  
  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIFactory) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

};

var Module = {
  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIModule) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  getClassObject: function M_getClassObject(aCompMgr, aCID, aIID) {
    if (!aIID.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    if (aCID.equals(LS_CLASSID))
      return new GenericComponentFactory(LivemarkService);

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  registerSelf: function(aCompMgr, aFile, aLocation, aType) {
    var cr = aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);

    cr.registerFactoryLocation(LS_CLASSID, LS_CLASSNAME,
      LS_CONTRACTID, aFile, aLocation, aType);
  },

  unregisterSelf: function M_unregisterSelf(aCompMgr, aLocation, aType) {
    var cr = aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    cr.unregisterFactoryLocation(LS_CLASSID, aLocation);
  },

  canUnload: function M_canUnload(aCompMgr) {
    return true;
  }
};

function NSGetModule(aCompMgr, aFile) {
  return Module;
}
