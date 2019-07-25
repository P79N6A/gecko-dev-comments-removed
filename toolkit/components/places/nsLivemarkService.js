



































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "idle",
                                   "@mozilla.org/widget/idleservice;1",
                                   "nsIIdleService");

XPCOMUtils.defineLazyServiceGetter(this, "secMan",
                                   "@mozilla.org/scriptsecuritymanager;1",
                                   "nsIScriptSecurityManager");

const SEC_FLAGS = Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;

const PREF_REFRESH_SECONDS = "browser.bookmarks.livemark_refresh_seconds";
const PREF_REFRESH_LIMIT_COUNT = "browser.bookmarks.livemark_refresh_limit_count";
const PREF_REFRESH_DELAY_TIME = "browser.bookmarks.livemark_refresh_delay_time";


let gExpiration = 3600000;


let gLimitCount = 1;


let gDelayTime  = 3;


const ERROR_EXPIRATION = 600000;


const IDLE_TIMELIMIT = 1800000;



const MAX_REFRESH_TIME = 3600000;

const MIN_REFRESH_TIME = 600000;


const STATUS = {
  IDLE: 0,
  LOADING: 1,
  FAILED: 2,
}

function LivemarkService() {
  this._loadPrefs();

  
  XPCOMUtils.defineLazyGetter(this, "_livemarks", function () {
    let livemarks = {};
    PlacesUtils.annotations
               .getItemsWithAnnotation(PlacesUtils.LMANNO_FEEDURI)
               .forEach(function (aFolderId) {
                  livemarks[aFolderId] = new Livemark(aFolderId);
                });
    return livemarks;
  });

  
  Services.obs.addObserver(this, PlacesUtils.TOPIC_SHUTDOWN, false);

  
  PlacesUtils.bookmarks.addObserver(this, false);
}

LivemarkService.prototype = {
  _updateTimer: null,
  start: function LS_start() {
    if (this._updateTimer) {
      return;
    }

    
    
    
    this._checkAllLivemarks();
  },

  stopUpdateLivemarks: function LS_stopUpdateLivemarks() {
    for each (let livemark in this._livemarks) {
      livemark.abort();
    }
    
    if (this._updateTimer) {
      this._updateTimer.cancel();
      this._updateTimer = null;
    }
  },

  _loadPrefs: function LS__loadPrefs() {
    try {
      let livemarkRefresh = Services.prefs.getIntPref(PREF_REFRESH_SECONDS);
      
      gExpiration = Math.max(livemarkRefresh * 1000, MIN_REFRESH_TIME);
    }
    catch (ex) {  }

    try {
      let limitCount = Services.prefs.getIntPref(PREF_REFRESH_LIMIT_COUNT);
      
      gLimitCount = Math.max(limitCount, gLimitCount);
    }
    catch (ex) {  }

    try {
      let delayTime = Services.prefs.getIntPref(PREF_REFRESH_DELAY_TIME);
      
      gDelayTime = Math.max(delayTime, gDelayTime);
    }
    catch (ex) {  }
  },

  
  observe: function LS_observe(aSubject, aTopic, aData) {
    if (aTopic == PlacesUtils.TOPIC_SHUTDOWN) {
      Services.obs.removeObserver(this, aTopic);
      
      PlacesUtils.bookmarks.removeObserver(this);
      
      this.stopUpdateLivemarks();
    }
  },

  
  
  _updatedLivemarks: [],
  _checkAllLivemarks: function LS__checkAllLivemarks() {
    let updateCount = 0;
    for each (let livemark in this._livemarks) {
      if (this._updatedLivemarks.indexOf(livemark.folderId) == -1) {
        this._updatedLivemarks.push(livemark.folderId);
        
        if (livemark.updateChildren()) {
          if (++updateCount == gLimitCount) {
            break;
          }
        }
      }
    }

    let refresh_time = gDelayTime * 1000;
    if (this._updatedLivemarks.length >= Object.keys(this._livemarks).length) {
      
      this._updatedLivemarks.length = 0;
      refresh_time = Math.min(Math.floor(gExpiration / 4), MAX_REFRESH_TIME);
    }
    this._newTimer(refresh_time);
  },

  _newTimer: function LS__newTimer(aTime) {
    if (this._updateTimer) {
      this._updateTimer.cancel();
    }
    this._updateTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._updateTimer.initWithCallback(this._checkAllLivemarks.bind(this),
                                       aTime, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  createLivemark: function LS_createLivemark(aParentId, aName, aSiteURI,
                                             aFeedURI, aIndex) {
    let folderId = this.createLivemarkFolderOnly(aParentId, aName, aSiteURI,
                                                 aFeedURI, aIndex);
    this._livemarks[folderId].updateChildren();
    return folderId;
  },

  createLivemarkFolderOnly:
  function LS_createLivemarkFolderOnly(aParentId, aName, aSiteURI,
                                       aFeedURI, aIndex) {
    if (!aParentId || aParentId < 1 || !aFeedURI ||
        this.isLivemark(aParentId)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let livemark = new Livemark({ parentId: aParentId,
                                  index: aIndex,
                                  title: aName });

    
    
    this._livemarks[livemark.folderId] = livemark;

    livemark.feedURI = aFeedURI;
    if (aSiteURI) {
      livemark.siteURI = aSiteURI;
    }

    return livemark.folderId;
  },

  isLivemark: function LS_isLivemark(aFolderId) {
    if (aFolderId < 1) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    return aFolderId in this._livemarks;
  },

  getLivemarkIdForFeedURI: function LS_getLivemarkIdForFeedURI(aFeedURI) {
    if (!(aFeedURI instanceof Ci.nsIURI)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    for each (livemark in this._livemarks) {
      if (livemark.feedURI.equals(aFeedURI)) {
        return livemark.folderId;
      }
    }

    return -1;
  },

  _ensureLivemark: function LS__ensureLivemark(aFolderId) {
    if (!this.isLivemark(aFolderId)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }
  },

  getSiteURI: function LS_getSiteURI(aFolderId) {
    this._ensureLivemark(aFolderId);

    return this._livemarks[aFolderId].siteURI;
  },

  setSiteURI: function LS_setSiteURI(aFolderId, aSiteURI) {
    if (aSiteURI && !(aSiteURI instanceof Ci.nsIURI)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }
    this._ensureLivemark(aFolderId);

    this._livemarks[aFolderId].siteURI = aSiteURI;
  },

  getFeedURI: function LS_getFeedURI(aFolderId) {
    this._ensureLivemark(aFolderId);

    return this._livemarks[aFolderId].feedURI;
  },

  setFeedURI: function LS_setFeedURI(aFolderId, aFeedURI) {
    if (!aFeedURI || !(aFeedURI instanceof Ci.nsIURI)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }
    this._ensureLivemark(aFolderId);

    this._livemarks[aFolderId].feedURI = aFeedURI;
  },

  reloadAllLivemarks: function LS_reloadAllLivemarks() {
    for each (let livemark in this._livemarks) {
      livemark.updateChildren(true);
    }
  },

  reloadLivemarkFolder: function LS_reloadLivemarkFolder(aFolderId) {
    this._ensureLivemark(aFolderId);

    this._livemarks[aFolderId].updateChildren(true);
  },

  
  onBeginUpdateBatch: function() { },
  onEndUpdateBatch: function() { },
  onItemAdded: function() { },
  onItemChanged: function() { },
  onItemVisited: function() { },
  onItemMoved: function() { },
  onBeforeItemRemoved: function() { },

  onItemRemoved: function(aItemId, aParentId, aIndex, aItemType) {
    
    
    if (!this.isLivemark(aItemId)) {
      return;
    }

    this._livemarks[aItemId].terminate();
    delete this._livemarks[aItemId];
  },

  
  classID: Components.ID("{dca61eb5-c7cd-4df1-b0fb-d0722baba251}"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsILivemarkService
  , Ci.nsINavBookmarkObserver
  , Ci.nsIObserver
  ])
};











function Livemark(aFolderIdOrCreationInfo)
{
  if (typeof(aFolderIdOrCreationInfo) == "number") {
    this.folderId = aFolderIdOrCreationInfo;
  }
  else if (typeof(aFolderIdOrCreationInfo) == "object"){
    this.folderId =
      PlacesUtils.bookmarks.createFolder(aFolderIdOrCreationInfo.parentId,
                                         aFolderIdOrCreationInfo.title,
                                         aFolderIdOrCreationInfo.index);
    PlacesUtils.bookmarks.setFolderReadonly(this.folderId, true);

    
    this._loadStatus = STATUS.IDLE;
  }
  else {
    throw Cr.NS_ERROR_UNEXPECTED;
  }
}

Livemark.prototype = {
  loadGroup: null,
  locked: null,

  


  get alive() !!this.folderId,

  









  _setAnno: function LM__setAnno(aName, aValue)
  {
    if (this.alive) {
      PlacesUtils.annotations
                 .setItemAnnotation(this.folderId, aName, aValue, 0,
                                    PlacesUtils.annotations.EXPIRE_NEVER);
    }
    return aValue;
  },

  







  _getAnno: function LM__getAnno(aName)
  {
    if (this.alive) {
      return PlacesUtils.annotations.getItemAnnotation(this.folderId, aName);
    }
    return null;
  },

  






  _removeAnno: function LM__removeAnno(aName)
  {
    if (this.alive) {
      return PlacesUtils.annotations.removeItemAnnotation(this.folderId, aName);
    }
  },

  set feedURI(aFeedURI)
  {
    this._setAnno(PlacesUtils.LMANNO_FEEDURI, aFeedURI.spec);
    this._feedURI = aFeedURI;
  },
  get feedURI()
  {
    if (this._feedURI === undefined) {
      this._feedURI = NetUtil.newURI(this._getAnno(PlacesUtils.LMANNO_FEEDURI));
    }
    return this._feedURI;
  },

  set siteURI(aSiteURI)
  {
    if (!aSiteURI) {
      this._removeAnno(PlacesUtils.LMANNO_SITEURI);
      this._siteURI = null;
      return;
    }

    
    let feedPrincipal = secMan.getCodebasePrincipal(this.feedURI);
    try {
      secMan.checkLoadURIWithPrincipal(feedPrincipal, aSiteURI, SEC_FLAGS);
    }
    catch (ex) {
      return;
    }

    this._setAnno(PlacesUtils.LMANNO_SITEURI, aSiteURI.spec)
    this._siteURI = aSiteURI;
  },
  get siteURI()
  {
    if (this._siteURI === undefined) {
      try {
        this._siteURI = NetUtil.newURI(this._getAnno(PlacesUtils.LMANNO_SITEURI));
      } catch (ex if ex.result == Cr.NS_ERROR_NOT_AVAILABLE) {
        
        return this._siteURI = null;
      }
    }
    return this._siteURI;
  },

  set expireTime(aExpireTime)
  {
    this._expireTime = this._setAnno(PlacesUtils.LMANNO_EXPIRATION,
                                     aExpireTime);
  },
  get expireTime()
  {
    if (this._expireTime === undefined) {
      try {
        this._expireTime = this._getAnno(PlacesUtils.LMANNO_EXPIRATION);
      } catch (ex if ex.result == Cr.NS_ERROR_NOT_AVAILABLE) {
        
        return this._expireTime = 0;
      }
    }
    return this._expireTime;
  },

  set loadStatus(aStatus)
  {
    
    if (this.loadStatus == aStatus) {
      return;
    }

    switch (aStatus) {
      case STATUS.FAILED:
        if (this.loadStatus == STATUS.LOADING) {
          this._removeAnno(PlacesUtils.LMANNO_LOADING);
        }
        this._setAnno(PlacesUtils.LMANNO_LOADFAILED, true);
        break;
      case STATUS.LOADING:
        if (this.loadStatus == STATUS.FAILED) {
          this._removeAnno(PlacesUtils.LMANNO_LOADFAILED);
        }
        this._setAnno(PlacesUtils.LMANNO_LOADING, true);
        break;
      default:
        if (this.loadStatus == STATUS.LOADING) {
          this._removeAnno(PlacesUtils.LMANNO_LOADING);
        }
        else if (this.loadStatus == STATUS.FAILED) {
          this._removeAnno(PlacesUtils.LMANNO_LOADFAILED);
        }
        break;
    }

    this._loadStatus = aStatus;
  },
  get loadStatus()
  {
    if (this._loadStatus === undefined) {
      if (PlacesUtils.annotations.itemHasAnnotation(this.folderId,
                                                    PlacesUtils.LMANNO_LOADFAILED)) {
        this._loadStatus = STATUS.FAILED;
      }
      else if (PlacesUtils.annotations.itemHasAnnotation(this.folderId,
                                                         PlacesUtils.LMANNO_LOADING)) {
        this._loadStatus = STATUS.LOADING;
      }
      else {
        this._loadStatus = STATUS.IDLE;
      }
    }
    return this._loadStatus;
  },

  








  updateChildren: function LM_updateChildren(aForceUpdate)
  {
    if (this.locked) {
      return false;
    }

    this.locked = true;

    
    
    
    
    if (!aForceUpdate && this.expireTime > Date.now()) {
      
      this.locked = false;
      return false;
    }

    
    
    
    
    try {
      let idleTime = idle.idleTime;
      if (idleTime > IDLE_TIMELIMIT) {
        this.locked = false;
        return false;
      }
    }
    catch (ex) {}

    let loadgroup;
    try {
      
      
      
      loadgroup = Cc["@mozilla.org/network/load-group;1"].
                  createInstance(Ci.nsILoadGroup);
      let channel = NetUtil.newChannel(this.feedURI.spec).
                    QueryInterface(Ci.nsIHttpChannel);
      channel.loadGroup = loadgroup;
      channel.loadFlags |= Ci.nsIRequest.LOAD_BACKGROUND |
                           Ci.nsIRequest.VALIDATE_ALWAYS;
      channel.requestMethod = "GET";
      channel.setRequestHeader("X-Moz", "livebookmarks", false);

      
      let listener = new LivemarkLoadListener(this);
      channel.notificationCallbacks = listener;

      this.loadStatus = STATUS.LOADING;
      channel.asyncOpen(listener, null);
    }
    catch (ex) {
      this.loadStatus = STATUS.FAILED;
      this.locked = false;
      return false;
    }
    this.loadGroup = loadgroup;
    return true;
  },

  





  replaceChildren: function LM_replaceChildren(aChildren) {
    let self = this;
    PlacesUtils.bookmarks.runInBatchMode({
      QueryInterface: XPCOMUtils.generateQI([
        Ci.nsINavHistoryBatchCallback
      ]),
      runBatched: function LM_runBatched(aUserData) {
        PlacesUtils.bookmarks.removeFolderChildren(self.folderId);
        aChildren.forEach(function (aChild) {
          PlacesUtils.bookmarks.insertBookmark(self.folderId,
                                               aChild.uri,
                                               PlacesUtils.bookmarks.DEFAULT_INDEX,
                                               aChild.title);
        });
      }
    }, null);
  },

  



  terminate: function LM_terminate()
  {
    this.folderId = null;
    this.abort();
  },

  


  abort: function LM_abort() {
    this.locked = false;
    if (this.loadGroup) {
      this.loadStatus = STATUS.FAILED;
      this.loadGroup.cancel(Cr.NS_BINDING_ABORTED);
      this.loadGroup = null;
    }
  },
}







function LivemarkLoadListener(aLivemark) {
  this._livemark = aLivemark;
  this._processor = null;
  this._isAborted = false;
  this._ttl = gExpiration;
}

LivemarkLoadListener.prototype = {
  abort: function LLL_abort() {
    this._isAborted = true;
    this._livemark.abort();
  },

  
  handleResult: function LLL_handleResult(aResult) {
    if (this._isAborted) {
      return;
    }

    try {
      
      let feedPrincipal = secMan.getCodebasePrincipal(this._livemark.feedURI);

      
      if (!aResult || !aResult.doc || aResult.bozo) {
        this.abort();
        this._ttl = gExpiration;
        throw Cr.NS_ERROR_FAILURE;
      }

      let feed = aResult.doc.QueryInterface(Ci.nsIFeed);
      if (feed.link) {
        let oldSiteURI = this._livemark.siteURI;
        if (!oldSiteURI || !feed.link.equals(oldSiteURI)) {
          this._livemark.siteURI = feed.link;
        }
      }

      
      let livemarkChildren = [];
      for (let i = 0; i < feed.items.length; ++i) {
        let entry = feed.items.queryElementAt(i, Ci.nsIFeedEntry);
        let href = entry.link;
        if (!href) {
          continue;
        }

        try {
          secMan.checkLoadURIWithPrincipal(feedPrincipal, href, SEC_FLAGS);
        }
        catch(ex) {
          continue;
        }

        let title = entry.title ? entry.title.plainText() : "";
        livemarkChildren.push({ uri: href, title: title });
      }

      this._livemark.replaceChildren(livemarkChildren);
    }
    catch (ex) {
      this.abort();
    }
    finally {
      this._processor.listener = null;
      this._processor = null;
    }
  },

  onDataAvailable: function LLL_onDataAvailable(aRequest, aContext, aInputStream,
                                                aSourceOffset, aCount) {
    if (this._processor) {
      this._processor.onDataAvailable(aRequest, aContext, aInputStream,
                                      aSourceOffset, aCount);
    }
  },

  onStartRequest: function LLL_onStartRequest(aRequest, aContext) {
    if (this._isAborted) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._livemark.loadStatus = STATUS.LOADING;

    let channel = aRequest.QueryInterface(Ci.nsIChannel);

    
    this._processor = Cc["@mozilla.org/feed-processor;1"].
                      createInstance(Ci.nsIFeedProcessor);
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
      this.abort();
      this._setResourceTTL(ERROR_EXPIRATION);
      return;
    }

    if (this._livemark.loadStatus == STATUS.LOADING) {
      this._livemark.loadStatus = STATUS.IDLE;
    }

    
    try {
      if (this._processor) {
        this._processor.onStopRequest(aRequest, aContext, aStatus);
      }

      
      let channel = aRequest.QueryInterface(Ci.nsICachingChannel);
      if (channel) {
        let entryInfo = channel.cacheToken.QueryInterface(Ci.nsICacheEntryInfo);
        if (entryInfo) {
          
          
          let expireTime = entryInfo.expirationTime * 1000;
          let nowTime = Date.now();

          
          if (expireTime > nowTime) {
            this._setResourceTTL(Math.max((expireTime - nowTime), gExpiration));
            return;
          }
        }
      }
    }
    catch (ex) {
      this.abort();
    }
    finally {
      this._livemark.locked = false;
      this._livemark.loadGroup = null;
    }
    this._setResourceTTL(this._ttl);
  },

  _setResourceTTL: function LLL__setResourceTTL(aMilliseconds) {
    this._livemark.expireTime = Date.now() + aMilliseconds;
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
  , Ci.nsIBadCertListener2
  , Ci.nsISSLErrorListener
  , Ci.nsIInterfaceRequestor
  ])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([LivemarkService]);
