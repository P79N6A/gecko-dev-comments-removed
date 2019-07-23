













































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;


XPCOMUtils.defineLazyServiceGetter(this, "bms",
                                   "@mozilla.org/browser/nav-bookmarks-service;1",
                                   "nsINavBookmarksService");
XPCOMUtils.defineLazyServiceGetter(this, "ans",
                                   "@mozilla.org/browser/annotation-service;1",
                                   "nsIAnnotationService");

const LMANNO_FEEDURI = "livemark/feedURI";
const LMANNO_SITEURI = "livemark/siteURI";
const LMANNO_EXPIRATION = "livemark/expiration";
const LMANNO_LOADFAILED = "livemark/loadfailed";
const LMANNO_LOADING = "livemark/loading";

const PS_CONTRACTID = "@mozilla.org/preferences-service;1";
const NH_CONTRACTID = "@mozilla.org/browser/nav-history-service;1";
const OS_CONTRACTID = "@mozilla.org/observer-service;1";
const IO_CONTRACTID = "@mozilla.org/network/io-service;1";
const LG_CONTRACTID = "@mozilla.org/network/load-group;1";
const FP_CONTRACTID = "@mozilla.org/feed-processor;1";
const SEC_CONTRACTID = "@mozilla.org/scriptsecuritymanager;1";
const IS_CONTRACTID = "@mozilla.org/widget/idleservice;1";
const LS_CONTRACTID = "@mozilla.org/browser/livemark-service;2";

const SEC_FLAGS = Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;

const PREF_REFRESH_SECONDS = "browser.bookmarks.livemark_refresh_seconds";
const PREF_REFRESH_LIMIT_COUNT = "browser.bookmarks.livemark_refresh_limit_count";
const PREF_REFRESH_DELAY_TIME = "browser.bookmarks.livemark_refresh_delay_time";


var gExpiration = 3600000;


var gLimitCount = 1;


var gDelayTime  = 3;


const ERROR_EXPIRATION = 600000;


const IDLE_TIMELIMIT = 1800000;



const MAX_REFRESH_TIME = 3600000;

const MIN_REFRESH_TIME = 600000;

function MarkLivemarkLoadFailed(aFolderId) {
  
  if (ans.itemHasAnnotation(aFolderId, LMANNO_LOADFAILED))
    return;

  
  ans.removeItemAnnotation(aFolderId, LMANNO_LOADING);
  ans.setItemAnnotation(aFolderId, LMANNO_LOADFAILED, true,
                        0, ans.EXPIRE_NEVER);
}

function LivemarkService() {
  
  
  this._prefs = Cc[PS_CONTRACTID].getService(Ci.nsIPrefBranch);
  this._loadPrefs();

  
  

  XPCOMUtils.defineLazyServiceGetter(this, "_idleService", IS_CONTRACTID,
                                     "nsIIdleService");

  
  this._ios = Cc[IO_CONTRACTID].getService(Ci.nsIIOService);
  var livemarks = ans.getItemsWithAnnotation(LMANNO_FEEDURI);
  for (let i = 0; i < livemarks.length; i++) {
    let spec = ans.getItemAnnotation(livemarks[i], LMANNO_FEEDURI);
    this._pushLivemark(livemarks[i], this._ios.newURI(spec, null, null));
  }

  
  this._obs = Cc[OS_CONTRACTID].getService(Ci.nsIObserverService);
  this._obs.addObserver(this, "xpcom-shutdown", false);

  
  bms.addObserver(this, false);
}

LivemarkService.prototype = {
  
  _livemarks: [],

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

  _loadPrefs: function LS__loadPrefs() {
    try {
      let livemarkRefresh = this._prefs.getIntPref(PREF_REFRESH_SECONDS);
      
      gExpiration = Math.max(livemarkRefresh * 1000, MIN_REFRESH_TIME);
    }
    catch (ex) {  }

    try {
      let limitCount = this._prefs.getIntPref(PREF_REFRESH_LIMIT_COUNT);
      
      gLimitCount = Math.max(limitCount, gLimitCount);
    }
    catch (ex) {  }

    try {
      let delayTime = this._prefs.getIntPref(PREF_REFRESH_DELAY_TIME);
      
      gDelayTime = Math.max(delayTime, gDelayTime);
    }
    catch (ex) {  }
  },

  
  observe: function LS_observe(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      this._obs.removeObserver(this, "xpcom-shutdown");

      
      bms.removeObserver(this);
      
      this.stopUpdateLivemarks();
    }
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

    let refresh_time = gDelayTime * 1000;
    if (this._nextUpdateStartIndex >= this._livemarks.length) {
      
      this._nextUpdateStartIndex = 0;
      refresh_time = Math.min(Math.floor(gExpiration / 4), MAX_REFRESH_TIME);
    }
    this._newTimer(refresh_time);
  },

  _newTimer: function LS__newTimer(aTime) {
    if (this._updateTimer)
      this._updateTimer.cancel();
    this._updateTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let self = this;
    this._updateTimer.initWithCallback({
      notify: function LS_T_notify() {
        self._checkAllLivemarks();
      },
      QueryInterface: XPCOMUtils.generateQI([
        Ci.nsITimerCallback
      ]),
    }, aTime, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  deleteLivemarkChildren: function LS_deleteLivemarkChildren(aFolderId) {
    bms.removeFolderChildren(aFolderId);
  },

  _updateLivemarkChildren:
  function LS__updateLivemarkChildren(aIndex, aForceUpdate) {
    if (this._livemarks[aIndex].locked)
      return false;

    var livemark = this._livemarks[aIndex];
    livemark.locked = true;
    try {
      
      
      
      
      var expireTime = ans.getItemAnnotation(livemark.folderId,
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
      
      ans.removeItemAnnotation(livemark.folderId, LMANNO_LOADFAILED);
      ans.setItemAnnotation(livemark.folderId, LMANNO_LOADING, true,
                            0, ans.EXPIRE_NEVER);
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
    var folderId = bms.createFolder(aParentId, aName, aIndex);
    bms.setFolderReadonly(folderId, true);

    
    ans.setItemAnnotation(folderId, LMANNO_FEEDURI, aFeedURI.spec,
                          0, ans.EXPIRE_NEVER);

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

    if (ans.itemHasAnnotation(aFolderId, LMANNO_SITEURI)) {
      var siteURIString = ans.getItemAnnotation(aFolderId, LMANNO_SITEURI);

      return this._ios.newURI(siteURIString, null, null);
    }
    return null;
  },

  setSiteURI: function LS_setSiteURI(aFolderId, aSiteURI) {
    this._ensureLivemark(aFolderId);

    if (!aSiteURI) {
      ans.removeItemAnnotation(aFolderId, LMANNO_SITEURI);
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
    ans.setItemAnnotation(aFolderId, LMANNO_SITEURI, aSiteURI.spec,
                          0, ans.EXPIRE_NEVER);
  },

  getFeedURI: function LS_getFeedURI(aFolderId) {
    if (ans.itemHasAnnotation(aFolderId, LMANNO_FEEDURI))
      return this._ios.newURI(ans.getItemAnnotation(aFolderId, LMANNO_FEEDURI),
                              null, null);
    return null;
  },

  setFeedURI: function LS_setFeedURI(aFolderId, aFeedURI) {
    if (!aFeedURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    ans.setItemAnnotation(aFolderId, LMANNO_FEEDURI, aFeedURI.spec,
                          0, ans.EXPIRE_NEVER);

    
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

  onItemRemoved: function(aItemId, aParentId, aIndex, aItemType) {
    
    
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

  
  classDescription: "Livemark Service",
  contractID: LS_CONTRACTID,
  classID: Components.ID("{dca61eb5-c7cd-4df1-b0fb-d0722baba251}"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsILivemarkService
  , Ci.nsINavBookmarkObserver
  , Ci.nsIObserver
  ])
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
      
      bms.runInBatchMode(this, aResult);
    }
    finally {
      this._processor.listener = null;
      this._processor = null;
      this._livemark.locked = false;
      ans.removeItemAnnotation(this._livemark.folderId, LMANNO_LOADING);
    }
  },

  deleteLivemarkChildren: LivemarkService.prototype.deleteLivemarkChildren,

  insertLivemarkChild:
  function LS_insertLivemarkChild(aFolderId, aUri, aTitle) {
    bms.insertBookmark(aFolderId, aUri, bms.DEFAULT_INDEX, aTitle);
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
    ans.setItemAnnotation(this._livemark.folderId, LMANNO_EXPIRATION,
                          expireTime, 0, ans.EXPIRE_NEVER);
  },

  
  notifyCertProblem:
  function LLL_certProblem(aSocketInfo, aStatus, aTargetSite) {
    return true;
  },

  
  notifySSLError: function LLL_SSLError(aSocketInfo, aError, aTargetSite) {
    return true;
  },

  
  getInterface: function LLL_getInterface(aIID) {
    return this.QueryInterface(aIID);
  },

  
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIFeedResultListener
  , Ci.nsIStreamListener
  , Ci.nsIRequestObserver
  , Ci.nsINavHistoryBatchCallback
  , Ci.nsIBadCertListener2
  , Ci.nsISSLErrorListener
  , Ci.nsIInterfaceRequestor
  ])
}

let component = [LivemarkService];
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(component);
}
