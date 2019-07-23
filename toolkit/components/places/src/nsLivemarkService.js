










































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

#include ../../url-classifier/content/js/lang.js
#include ../../url-classifier/content/moz/lang.js
#include ../../url-classifier/content/moz/observer.js
#include ../../url-classifier/content/moz/alarm.js

function LOG(str) {
  dump("*** " + str + "\n");
}

const LS_CLASSID = Components.ID("{dca61eb5-c7cd-4df1-b0fb-d0722baba251}");
const LS_CLASSNAME = "Livemark Service";
const LS_CONTRACTID = "@mozilla.org/browser/livemark-service;2";

const LIVEMARK_TIMEOUT = 15000; 
const LIVEMARK_ICON_URI = "chrome://browser/skin/places/livemarkItem.png";
const PLACES_BUNDLE_URI = 
  "chrome://browser/locale/places/places.properties";
const DEFAULT_LOAD_MSG = "Live Bookmark loading...";
const DEFAULT_FAIL_MSG = "Live Bookmark feed failed to load.";
const LMANNO_FEEDURI = "livemark/feedURI";
const LMANNO_SITEURI = "livemark/siteURI";
const LMANNO_EXPIRATION = "livemark/expiration";
const LMANNO_BMANNO = "livemark/bookmarkFeedURI";

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
const SEC_FLAGS = Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;


const EXPIRATION = 3600000;

const ERROR_EXPIRATION = 600000;

var gIoService = Cc[IO_CONTRACTID].getService(Ci.nsIIOService);
var gStringBundle;
function GetString(name)
{
  try {
    if (!gStringBundle) {
      var bundleService = Cc[SB_CONTRACTID].getService(); 
      bundleService = bundleService.QueryInterface(Ci.nsIStringBundleService);
      gStringBundle = bundleService.createBundle(PLACES_BUNDLE_URI);
    }

    if (gStringBundle)
      return gStringBundle.GetStringFromName(name);
  } catch (ex) {
    LOG("Exception loading string bundle: " + ex.message);
  }

  return null;
}

var gLivemarkService;
function LivemarkService() {
  
  
  this._livemarks = [];

  this._iconURI = gIoService.newURI(LIVEMARK_ICON_URI, null, null);
  this._loading = GetString("bookmarksLivemarkLoading") || DEFAULT_LOAD_MSG;
  this._observerServiceObserver =
    new G_ObserverServiceObserver('xpcom-shutdown',
                                  BindToObject(this._shutdown, this),
                                  true );
  new G_Alarm(BindToObject(this._fireTimer, this), LIVEMARK_TIMEOUT, 
              true );

  
  this._ans = Cc[AS_CONTRACTID].getService(Ci.nsIAnnotationService);

  var livemarks = this._ans.getItemsWithAnnotation(LMANNO_FEEDURI, {});
  for (var i = 0; i < livemarks.length; i++) {
    var feedURI =
      gIoService.newURI(
        this._ans.getItemAnnotationString(livemarks[i], LMANNO_FEEDURI),
        null, null
      );
    this._pushLivemark(livemarks[i], feedURI);
  }
}

LivemarkService.prototype = {

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

  
  _pushLivemark: function LS__pushLivemark(folderId, feedURI) {
    return this._livemarks.push({folderId: folderId, feedURI: feedURI});
  },

  _getLivemarkIndex: function LS__getLivemarkIndex(folderId) {
    for (var i=0; i < this._livemarks.length; ++i) {
      if (this._livemarks[i].folderId == folderId)
        return i;
    }
    throw Cr.NS_ERROR_INVALID_ARG;
  },

  _shutdown: function LS__shutdown() {
    for (var livemark in this._livemarks) {
      if (livemark.loadGroup) 
        livemark.loadGroup.cancel(Cr.NS_BINDING_ABORTED);
    }
  },

  _fireTimer: function LS__fireTimer() {
    for (var i=0; i < this._livemarks.length; ++i) {
      this._updateLivemarkChildren(i, false);
    }
  },

  deleteLivemarkChildren: function LS_deleteLivemarkChildren(folderId) {
    this._bms.removeFolderChildren(folderId);
  },

  insertLivemarkLoadingItem: function LS_insertLivemarkLoading(bms, folderId) {
    var loadingURI = gIoService.newURI("about:livemark-loading", null, null);
    var id = bms.insertBookmark(folderId, loadingURI, bms.DEFAULT_INDEX,
                                this._loading);
  },

  _updateLivemarkChildren:
  function LS__updateLivemarkChildren(index, forceUpdate) {
    if (this._livemarks[index].locked)
      return;
    
    var livemark = this._livemarks[index];
    livemark.locked = true;
    try {
      
      
      
      
      var exprTime = this._ans.getPageAnnotationInt64(livemark.feedURI,
                                                      LMANNO_EXPIRATION);
      if (!forceUpdate && exprTime > Date.now()) {
        
        livemark.locked = false;
        return;
      }
    } 
    catch (ex) {
      
      this.insertLivemarkLoadingItem(this._bms, livemark.folderId);
    }

    var loadgroup;
    try {
      
      
      
      loadgroup = Cc[LG_CONTRACTID].createInstance(Ci.nsILoadGroup);
      var uriChannel = gIoService.newChannel(livemark.feedURI.spec, null, null);
      uriChannel.loadGroup = loadgroup;
      uriChannel.loadFlags |= Ci.nsIRequest.LOAD_BACKGROUND | 
                              Ci.nsIRequest.VALIDATE_ALWAYS;
      var httpChannel = uriChannel.QueryInterface(Ci.nsIHttpChannel);
      httpChannel.requestMethod = "GET";
      httpChannel.setRequestHeader("X-Moz", "livebookmarks", false);

      
      var listener = new LivemarkLoadListener(livemark);
      httpChannel.asyncOpen(listener, null);
    }
    catch (ex) {
      livemark.locked = false;
      LOG("exception: " + ex);
      throw ex;
    }
    livemark.loadGroup = loadgroup;
  },

  createLivemark: function LS_createLivemark(folder, name, siteURI,
                                             feedURI, index) {
    
    if (this.isLivemark(folder))
      throw Cr.NS_ERROR_INVALID_ARG;
    var livemarkID = this._createFolder(this._bms, folder, name, siteURI,
                                        feedURI, index);
  
    
    this._updateLivemarkChildren(
      this._pushLivemark(livemarkID, feedURI) - 1,
      false
    );

    return livemarkID;
  },

  createLivemarkFolderOnly:
  function LS_createLivemarkFolderOnly(bms, folder, name, siteURI,
                                       feedURI, index) {
    var livemarkID = this._createFolder(bms, folder, name, siteURI, feedURI,
                                        index);
    this.insertLivemarkLoadingItem(bms, livemarkID);
    this._pushLivemark(livemarkID, feedURI);
    
    return livemarkID;
  },

  _createFolder:
  function LS__createFolder(bms, folder, name, siteURI, feedURI, index) {
    var livemarkID = bms.createContainer(folder, name, LS_CONTRACTID, index);

    
    this._ans.setItemAnnotationString(livemarkID, LMANNO_FEEDURI, feedURI.spec, 0,
                                      this._ans.EXPIRE_NEVER);
    
    var faviconService = Cc[FAV_CONTRACTID].getService(Ci.nsIFaviconService);
    var livemarkURI = bms.getFolderURI(livemarkID);
    faviconService.setFaviconUrlForPage(livemarkURI, this._iconURI);

    if (siteURI) {
      
      this._ans.setItemAnnotationString(livemarkID, LMANNO_SITEURI, siteURI.spec,
                                        0, this._ans.EXPIRE_NEVER);
    }

    return livemarkID;
  },

  isLivemark: function LS_isLivemark(folder) {
    return this._ans.itemHasAnnotation(folder, LMANNO_FEEDURI);
  },

  _ensureLivemark: function LS__ensureLivemark(container) {
    if (!this.isLivemark(container)) 
      throw Cr.NS_ERROR_INVALID_ARG;
  },

  





  getSiteURI: function LS_getSiteURI(container) {
    this._ensureLivemark(container);
    var siteURIString;
    try { 
      siteURIString =
        this._ans.getItemAnnotationString(container, LMANNO_SITEURI);
    }
    catch (ex) {
      return null;
    }

    return gIoService.newURI(siteURIString, null, null);
  },

  setSiteURI: function LS_setSiteURI(container, siteURI) {
    this._ensureLivemark(container);
    
    if (!siteURI) {
      this._ans.removeItemAnnotation(container, LMANNO_SITEURI);
      return;
    }

    this._ans.setItemAnnotationString(container, LMANNO_SITEURI, siteURI.spec,
                                      0, this._ans.EXPIRE_NEVER);
  },

  getFeedURI: function LS_getFeedURI(container) {
    return gIoService.newURI(this._ans.getItemAnnotationString(container,
                                                               LMANNO_FEEDURI),
                             null, null);
  },

  setFeedURI: function LS_setFeedURI(container, feedURI) {
    if (!feedURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    this._ans.setItemAnnotationString(container, LMANNO_FEEDURI, feedURI.spec,
                                      0, this._ans.EXPIRE_NEVER);

    
    var livemarkIndex = this._getLivemarkIndex(container);  
    this._livemarks[livemarkIndex].feedURI = feedURI;
  },

  reloadAllLivemarks: function LS_reloadAllLivemarks() {
    for (var i = 0; i < this._livemarks.length; ++i) {
      this._updateLivemarkChildren(i, true);
    }
  },

  reloadLivemarkFolder: function LS_reloadLivemarkFolder(folderID) {
    var livemarkIndex = this._getLivemarkIndex(folderID);  
    this._updateLivemarkChildren(livemarkIndex, true);
  },

  
  onContainerRemoving: function LS_onContainerRemoving(container) {
    var livemarkIndex = this._getLivemarkIndex(container);
    var livemark = this._livemarks[livemarkIndex];

    var stillInUse = false;
    stillInUse = this._livemarks.some(
                 function(mark) { return mark.feedURI.equals(livemark.feedURI) } 
                 );
    if (!stillInUse) {
      
      
      this._ans.removePageAnnotation(livemark.feedURI, LMANNO_EXPIRATION);
    }

    if (livemark.loadGroup) 
      livemark.loadGroup.cancel(Cr.NS_BINDING_ABORTED);
    this._livemarks.splice(livemarkIndex, 1);
    this.deleteLivemarkChildren(container);
  },

  onContainerMoved:
  function LS_onContainerMoved(container, newFolder, newIndex) { },

  childrenReadOnly: true,

  createInstance: function LS_createInstance(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return this.QueryInterface(iid);
  },
  
  QueryInterface: function LS_QueryInterface(iid) {
    if (iid.equals(Ci.nsILivemarkService) ||
        iid.equals(Ci.nsIRemoteContainer) ||
        iid.equals(Ci.nsIFactory) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
};

function LivemarkLoadListener(livemark) {
  this._livemark = livemark;
  this._processor = null;
  this._isAborted = false;
  this._ttl = EXPIRATION;
  this._ans = Cc[AS_CONTRACTID].getService(Ci.nsIAnnotationService);
}

LivemarkLoadListener.prototype = {

  get _failed() {
    return GetString("bookmarksLivemarkFailed") || DEFAULT_FAIL_MSG;
  },

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

  


  handleResult: function LLL_handleResult(result) {
    if (this._isAborted) {
      this._livemark.locked = false;
      return;
    }

    try {  
      
      var secMan = Cc[SEC_CONTRACTID].getService(Ci.nsIScriptSecurityManager);
      
      
      
      var lmService = Cc[LS_CONTRACTID].getService(Ci.nsILivemarkService);
      this.deleteLivemarkChildren(this._livemark.folderId);

      
      if (!result || !result.doc || result.bozo) {
        this.insertLivemarkFailedItem(this._livemark.folderId);
        this._ttl = EXPIRATION;
        throw Cr.NS_ERROR_FAILURE;
      }

      var title, href, entry;
      var feed = result.doc.QueryInterface(Ci.nsIFeed);
      
      
      for (var i = 0; i < feed.items.length; ++i) {
        entry = feed.items.queryElementAt(i, Ci.nsIFeedEntry);
        if (entry.title)
          title = entry.title.plainText();
        else if (entry.updated)
          title = entry.updated;
        
        if (entry.link) {
          try {
              secMan.checkLoadURIStr(this._livemark.feedURI.spec,
                                     entry.link.spec, SEC_FLAGS);
              href = entry.link;
          } 
          catch (ex) {
          }
        }
        
        if (href && title) {
          this.insertLivemarkChild(this._livemark.folderId,
                                   href, title);
        }
      }
    } 
    finally {
      this._processor.listener = null;
      this._processor = null;
      this._livemark.locked = false;
    }
  },
  
  deleteLivemarkChildren: LivemarkService.prototype.deleteLivemarkChildren,

  insertLivemarkFailedItem: function LS_insertLivemarkFailed(folderId) {
    var failedURI = gIoService.newURI("about:livemark-failed", null, null);
    var id = this._bms.insertBookmark(folderId, failedURI, this._bms.DEFAULT_INDEX,
                                      this._failed);
  },

  insertLivemarkChild:
  function LS_insertLivemarkChild(folderId, uri, title) {
    var id = this._bms.insertBookmark(folderId, uri, this._bms.DEFAULT_INDEX,
                                      title);
    this._ans.setItemAnnotationString(id, LMANNO_BMANNO, uri.spec, 0,
                                      this._ans.EXPIRE_NEVER);
  },

  


  onDataAvailable: function LLL_onDataAvailable(request, context, inputStream, 
                                                sourceOffset, count) {
    this._processor.onDataAvailable(request, context, inputStream,
                                    sourceOffset, count);
  },
  
  


  onStartRequest: function LLL_onStartRequest(request, context) {
    if (this._isAborted)
      throw Cr.NS_ERROR_UNEXPECTED;

    var channel = request.QueryInterface(Ci.nsIChannel);
 
    
    this._processor = Cc[FP_CONTRACTID].createInstance(Ci.nsIFeedProcessor);
    this._processor.listener = this;
    this._processor.parseAsync(null, channel.URI);
    
    this._processor.onStartRequest(request, context);
  },
  
  


  onStopRequest: function LLL_onStopReqeust(request, context, status) {
    if (!Components.isSuccessCode(status)) {
      
      this._setResourceTTL(ERROR_EXPIRATION);
      this._isAborted = true;
      return;
    }

    
    try { 
      this._processor.onStopRequest(request, context, status);

      
      var channel = request.QueryInterface(Ci.nsICachingChannel);
      if (channel) {
        var entryInfo = channel.cacheToken.QueryInterface(Ci.nsICacheEntryInfo);
        if (entryInfo) {
          var exprtime = entryInfo.expirationTime;
          var nowtime = Date.now();
          
          if (nowtime >= exprtime) {
            expiresTime -= nowtime;
            if (expiresTime > EXPIRATION)
              this._setResourceTTL(expiresTime);
            else
              this._setResourceTTL(EXPIRATION);
          }
        }
      }
    }
    catch (ex) { 
      this._setResourceTTL(this._ttl);
    }
    
  },

  _setResourceTTL: function LLL__setResourceTTL(seconds) {
    var exptime = Date.now() + seconds;
    this._ans.setPageAnnotationInt64(this._livemark.feedURI,
                                     LMANNO_EXPIRATION, exptime, 0,
                                     Ci.nsIAnnotationService.EXPIRE_NEVER);
  },
  
  


  QueryInterface: function LLL_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFeedResultListener) ||
        iid.equals(Ci.nsIStreamListener) ||
        iid.equals(Ci.nsIRequestObserver)||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
}

function GenericComponentFactory(ctor) {
  this._ctor = ctor;
}

GenericComponentFactory.prototype = {

  _ctor: null,

  
  createInstance: function(outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return (new this._ctor()).QueryInterface(iid);
  },

  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIFactory) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

};

var Module = {
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIModule) || 
        iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  getClassObject: function M_getClassObject(cm, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    if (cid.equals(LS_CLASSID))
      return new GenericComponentFactory(LivemarkService);

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  registerSelf: function(cm, file, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
 
    cr.registerFactoryLocation(LS_CLASSID, LS_CLASSNAME,
      LS_CONTRACTID, file, location, type);    
  },

  unregisterSelf: function M_unregisterSelf(cm, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    cr.unregisterFactoryLocation(LS_CLASSID, location);
  },
  
  canUnload: function M_canUnload(cm) {
    return true;
  }
};

function NSGetModule(cm, file) {
  return Module;
}
