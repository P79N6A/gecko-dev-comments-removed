



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;




Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");




XPCOMUtils.defineLazyServiceGetter(this, "secMan",
                                   "@mozilla.org/scriptsecuritymanager;1",
                                   "nsIScriptSecurityManager");
XPCOMUtils.defineLazyGetter(this, "asyncHistory", function () {
  
  PlacesUtils.history.addObserver(PlacesUtils.livemarks, true);
  return Cc["@mozilla.org/browser/history;1"].getService(Ci.mozIAsyncHistory);
});





const SEC_FLAGS = Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;


const RELOAD_DELAY_MS = 500;

const EXPIRE_TIME_MS = 3600000; 

const ONERROR_EXPIRE_TIME_MS = 300000; 




function LivemarkService()
{
  
  Services.obs.addObserver(this, PlacesUtils.TOPIC_SHUTDOWN, true);
 
  
  PlacesUtils.addLazyBookmarkObserver(this, true);

  
  this._ensureAsynchronousCache();
}

LivemarkService.prototype = {
  
  _livemarks: {},
  
  _guids: {},

  get _populateCacheSQL()
  {
    function getAnnoSQLFragment(aAnnoParam) {
      return "SELECT a.content "
           + "FROM moz_items_annos a "
           + "JOIN moz_anno_attributes n ON n.id = a.anno_attribute_id "
           + "WHERE a.item_id = b.id "
           +   "AND n.name = " + aAnnoParam;
    }

    return "SELECT b.id, b.title, b.parent, b.position, b.guid, b.lastModified, "
         +        "(" + getAnnoSQLFragment(":feedURI_anno") + ") AS feedURI, "
         +        "(" + getAnnoSQLFragment(":siteURI_anno") + ") AS siteURI "
         + "FROM moz_bookmarks b "
         + "JOIN moz_items_annos a ON a.item_id = b.id "
         + "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
         + "WHERE b.type = :folder_type "
         +   "AND n.name = :feedURI_anno ";
  },

  _ensureAsynchronousCache: function LS__ensureAsynchronousCache()
  {
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection;
    let stmt = db.createAsyncStatement(this._populateCacheSQL);
    stmt.params.folder_type = Ci.nsINavBookmarksService.TYPE_FOLDER;
    stmt.params.feedURI_anno = PlacesUtils.LMANNO_FEEDURI;
    stmt.params.siteURI_anno = PlacesUtils.LMANNO_SITEURI;

    let livemarkSvc = this;
    this._pendingStmt = stmt.executeAsync({
      handleResult: function LS_handleResult(aResults)
      {
        for (let row = aResults.getNextRow(); row; row = aResults.getNextRow()) {
          let id = row.getResultByName("id");
          let siteURL = row.getResultByName("siteURI");
          let guid = row.getResultByName("guid");
          livemarkSvc._livemarks[id] =
            new Livemark({ id: id,
                           guid: guid,             
                           title: row.getResultByName("title"),
                           parentId: row.getResultByName("parent"),
                           index: row.getResultByName("position"),
                           lastModified: row.getResultByName("lastModified"),
                           feedURI: NetUtil.newURI(row.getResultByName("feedURI")),
                           siteURI: siteURL ? NetUtil.newURI(siteURL) : null,
            });
          livemarkSvc._guids[guid] = id;
        }
      },
      handleError: function LS_handleError(aErr)
      {
        Cu.reportError("AsyncStmt error (" + aErr.result + "): '" + aErr.message);
      },
      handleCompletion: function LS_handleCompletion() {
        livemarkSvc._pendingStmt = null;
      }
    });
    stmt.finalize();
  },

  _onCacheReady: function LS__onCacheReady(aCallback, aWaitForAsyncWrites)
  {
    if (this._pendingStmt || aWaitForAsyncWrites) {
      
      
      
      
      let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                  .DBConnection;
      let stmt = db.createAsyncStatement("PRAGMA encoding");
      stmt.executeAsync({
        handleError: function () {},
        handleResult: function () {},
        handleCompletion: function ETAT_handleCompletion()
        {
          aCallback();
        }
      });
      stmt.finalize();
    }
    else {
      
      
      Services.tm.mainThread.dispatch(aCallback, Ci.nsIThread.DISPATCH_NORMAL);
    }
  },

  _reloading: false,
  _startReloadTimer: function LS__startReloadTimer()
  {
    if (this._reloadTimer) {
      this._reloadTimer.cancel();
    }
    else {
      this._reloadTimer = Cc["@mozilla.org/timer;1"]
                            .createInstance(Ci.nsITimer);
    }
    this._reloading = true;
    this._reloadTimer.initWithCallback(this._reloadNextLivemark.bind(this),
                                       RELOAD_DELAY_MS,
                                       Ci.nsITimer.TYPE_ONE_SHOT);
  },

  
  

  observe: function LS_observe(aSubject, aTopic, aData)
  {
    if (aTopic == PlacesUtils.TOPIC_SHUTDOWN) {
      if (this._pendingStmt) {
        this._pendingStmt.cancel();
        this._pendingStmt = null;
        
        return;
      }

      if (this._reloadTimer) {
        this._reloading = false;
        this._reloadTimer.cancel();
        delete this._reloadTimer;
      }

      
      for each (let livemark in this._livemarks) {
        livemark.terminate();
      }
      this._livemarks = {};
    }
  },

  
  

  addLivemark: function LS_addLivemark(aLivemarkInfo,
                                       aLivemarkCallback)
  {
    
    if (!aLivemarkInfo ||
        ("parentId" in aLivemarkInfo && aLivemarkInfo.parentId < 1) ||
        !("index" in aLivemarkInfo) || aLivemarkInfo.index < Ci.nsINavBookmarksService.DEFAULT_INDEX ||
        !(aLivemarkInfo.feedURI instanceof Ci.nsIURI) ||
        (aLivemarkInfo.siteURI && !(aLivemarkInfo.siteURI instanceof Ci.nsIURI)) ||
        (aLivemarkInfo.guid && !/^[a-zA-Z0-9\-_]{12}$/.test(aLivemarkInfo.guid))) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    
    
    
    let result = Cr.NS_OK;
    let livemark = null;
    try {
      
      if (aLivemarkInfo.parentId in this._livemarks) {
        throw new Components.Exception("", Cr.NS_ERROR_INVALID_ARG);
      }

      
      livemark = new Livemark({ title:        aLivemarkInfo.title
                              , parentId:     aLivemarkInfo.parentId
                              , index:        aLivemarkInfo.index
                              , feedURI:      aLivemarkInfo.feedURI
                              , siteURI:      aLivemarkInfo.siteURI
                              , guid:         aLivemarkInfo.guid
                              , lastModified: aLivemarkInfo.lastModified
                              });
      if (this._itemAdded && this._itemAdded.id == livemark.id) {
        livemark.index = this._itemAdded.index;
        if (!aLivemarkInfo.guid) {
          livemark.guid = this._itemAdded.guid;
        }
        if (!aLivemarkInfo.lastModified) {
          livemark.lastModified = this._itemAdded.lastModified;
        }
      }

      
      
      this._livemarks[livemark.id] = livemark;
      this._guids[aLivemarkInfo.guid] = livemark.id;
    }
    catch (ex) {
      result = ex.result;
      livemark = null;
    }
    finally {
      if (aLivemarkCallback) {
        this._onCacheReady(function LS_addLivemark_ETAT() {
          try {
            aLivemarkCallback.onCompletion(result, livemark);
          } catch(ex2) {}
        }, true);
      }
    }
  },

  removeLivemark: function LS_removeLivemark(aLivemarkInfo, aLivemarkCallback)
  {
    if (!aLivemarkInfo) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    
    let id = aLivemarkInfo.guid || aLivemarkInfo.id;
    if (("guid" in aLivemarkInfo && !/^[a-zA-Z0-9\-_]{12}$/.test(aLivemarkInfo.guid)) ||
        ("id" in aLivemarkInfo && aLivemarkInfo.id < 1) ||
        !id) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    
    if (id in this._guids) {
      id = this._guids[id];
    }
    let result = Cr.NS_OK;
    try {
      if (!(id in this._livemarks)) {
        throw new Components.Exception("", Cr.NS_ERROR_INVALID_ARG);
      }
      this._livemarks[id].remove();
    }
    catch (ex) {
      result = ex.result;
    }
    finally {
      if (aLivemarkCallback) {
        
        this._onCacheReady(function LS_removeLivemark_ETAT() {
          try {
            aLivemarkCallback.onCompletion(result, null);
          } catch(ex2) {}
        });
      }
    }
  },

  _reloaded: [],
  _reloadNextLivemark: function LS__reloadNextLivemark()
  {
    this._reloading = false;
    
    for (let id in this._livemarks) {
      if (this._reloaded.indexOf(id) == -1) {
        this._reloaded.push(id);
        this._livemarks[id].reload(this._forceUpdate);
        this._startReloadTimer();
        break;
      }
    }
  },

  reloadLivemarks: function LS_reloadLivemarks(aForceUpdate)
  {
    
    let notWorthRestarting =
      this._forceUpdate || 
      !aForceUpdate;       
    if (this._reloading && notWorthRestarting) {
      
      return;
    } 

    this._onCacheReady((function LS_reloadAllLivemarks_ETAT() {
      this._forceUpdate = !!aForceUpdate;
      this._reloaded = [];
      
      
      this._startReloadTimer();
    }).bind(this));
  },

  getLivemark: function LS_getLivemark(aLivemarkInfo, aLivemarkCallback)
  {
    if (!aLivemarkInfo) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }
    
    let id = aLivemarkInfo.guid || aLivemarkInfo.id;
    if (("guid" in aLivemarkInfo && !/^[a-zA-Z0-9\-_]{12}$/.test(aLivemarkInfo.guid)) ||
        ("id" in aLivemarkInfo && aLivemarkInfo.id < 1) ||
        !id) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    this._onCacheReady((function LS_getLivemark_ETAT() {
      
      if (id in this._guids) {
        id = this._guids[id];
      }
      if (id in this._livemarks) {
        try {
          aLivemarkCallback.onCompletion(Cr.NS_OK, this._livemarks[id]);
        } catch (ex) {}
      }
      else {
        try {
          aLivemarkCallback.onCompletion(Cr.NS_ERROR_INVALID_ARG, null);
        } catch (ex) {}
      }
    }).bind(this));
  },

  
  

  onBeginUpdateBatch:  function () {},
  onEndUpdateBatch:    function () {},
  onItemVisited:       function () {},

  _itemAdded: null,
  onItemAdded: function LS_onItemAdded(aItemId, aParentId, aIndex, aItemType,
                                       aURI, aTitle, aDateAdded, aGUID)
  {
    if (aItemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
      this._itemAdded = { id: aItemId
                        , guid: aGUID
                        , index: aIndex
                        , lastModified: aDateAdded
                        };
    }
  },

  onItemChanged: function LS_onItemChanged(aItemId, aProperty, aIsAnno, aValue,
                                           aLastModified, aItemType)
  {
    if (aItemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
      if (this._itemAdded && this._itemAdded.id == aItemId) {
        this._itemAdded.lastModified = aLastModified;
     }
      if (aItemId in this._livemarks) {
        if (aProperty == "title") {
          this._livemarks[aItemId].title = aValue;
        }
        this._livemarks[aItemId].lastModified = aLastModified;
      }
    }
  },

  onItemMoved: function LS_onItemMoved(aItemId, aOldParentId, aOldIndex,
                                      aNewParentId, aNewIndex, aItemType)
  {
    if (aItemType == Ci.nsINavBookmarksService.TYPE_FOLDER &&
        aItemId in this._livemarks) {
      this._livemarks[aItemId].parentId = aNewParentId;
      this._livemarks[aItemId].index = aNewIndex;
    }
  },

  onItemRemoved: function LS_onItemRemoved(aItemId, aParentId, aIndex,
                                           aItemType, aURI, aGUID)
  {
    if (aItemType == Ci.nsINavBookmarksService.TYPE_FOLDER &&
        aItemId in this._livemarks) {
      this._livemarks[aItemId].terminate();
      delete this._livemarks[aItemId];
      delete this._guids[aGUID];
    }
  },

  
  

  onBeginUpdateBatch: function () {},
  onEndUpdateBatch:   function () {},
  onPageChanged:      function () {},
  onTitleChanged:     function () {},
  onDeleteVisits:     function () {},
  onClearHistory:     function () {},

  onDeleteURI: function PS_onDeleteURI(aURI) {
    for each (let livemark in this._livemarks) {
      livemark.updateURIVisitedStatus(aURI, false);
    }
  },

  onVisit: function PS_onVisit(aURI) {
    for each (let livemark in this._livemarks) {
      livemark.updateURIVisitedStatus(aURI, true);
    }
  },

  
  

  classID: Components.ID("{dca61eb5-c7cd-4df1-b0fb-d0722baba251}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(LivemarkService),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.mozIAsyncLivemarks
  , Ci.nsINavBookmarkObserver
  , Ci.nsINavHistoryObserver
  , Ci.nsIObserver
  , Ci.nsISupportsWeakReference
  ])
};













function Livemark(aLivemarkInfo)
{
  this.title = aLivemarkInfo.title;
  this.parentId = aLivemarkInfo.parentId;
  this.index = aLivemarkInfo.index;

  this._status = Ci.mozILivemark.STATUS_READY;

  
  this._resultObservers = new Map();
  
  
  this._resultObserversList = [];

  
  
  this._children = [];

  
  
  this._nodes = new Map();

  this._guid = "";
  this._lastModified = 0;

  this.loadGroup = null;
  this.feedURI = null;
  this.siteURI = null;
  this.expireTime = 0;

  if (aLivemarkInfo.id) {
    
    this.id = aLivemarkInfo.id;
    this.guid = aLivemarkInfo.guid;
    this.feedURI = aLivemarkInfo.feedURI;
    this.siteURI = aLivemarkInfo.siteURI;
    this.lastModified = aLivemarkInfo.lastModified;
  }
  else {
    
    this.id = PlacesUtils.bookmarks.createFolder(aLivemarkInfo.parentId,
                                                 aLivemarkInfo.title,
                                                 aLivemarkInfo.index);
    PlacesUtils.bookmarks.setFolderReadonly(this.id, true);
    if (aLivemarkInfo.guid) {
      this.writeGuid(aLivemarkInfo.guid);
    }
    this.writeFeedURI(aLivemarkInfo.feedURI);
    if (aLivemarkInfo.siteURI) {
      this.writeSiteURI(aLivemarkInfo.siteURI);
    }
    
    if (aLivemarkInfo.lastModified) {
      this.lastModified = aLivemarkInfo.lastModified;
      PlacesUtils.bookmarks.setItemLastModified(this.id, this.lastModified);
    }
  }
}

Livemark.prototype = {
  get status() this._status,
  set status(val) {
    if (this._status != val) {
      this._status = val;
      this._invalidateRegisteredContainers();
    }
    return this._status;
  },

  









  _setAnno: function LM__setAnno(aAnnoName, aValue)
  {
    PlacesUtils.annotations
               .setItemAnnotation(this.id, aAnnoName, aValue, 0,
                                  PlacesUtils.annotations.EXPIRE_NEVER);
  },

  writeFeedURI: function LM_writeFeedURI(aFeedURI)
  {
    this._setAnno(PlacesUtils.LMANNO_FEEDURI, aFeedURI.spec);
    this.feedURI = aFeedURI;
  },

  writeSiteURI: function LM_writeSiteURI(aSiteURI)
  {
    if (!aSiteURI) {
      PlacesUtils.annotations.removeItemAnnotation(this.id,
                                                   PlacesUtils.LMANNO_SITEURI)
      this.siteURI = null;
      return;
    }

    
    let feedPrincipal = secMan.getSimpleCodebasePrincipal(this.feedURI);
    try {
      secMan.checkLoadURIWithPrincipal(feedPrincipal, aSiteURI, SEC_FLAGS);
    }
    catch (ex) {
      return;
    }

    this._setAnno(PlacesUtils.LMANNO_SITEURI, aSiteURI.spec)
    this.siteURI = aSiteURI;
  },

  writeGuid: function LM_writeGuid(aGUID)
  {
    
    
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection;
    let stmt = db.createAsyncStatement("UPDATE moz_bookmarks " +
                                       "SET guid = :guid " +
                                       "WHERE id = :item_id");
    stmt.params.guid = aGUID;
    stmt.params.item_id = this.id;
    let livemark = this;
    stmt.executeAsync({
      handleError: function () {},
      handleResult: function () {},
      handleCompletion: function ETAT_handleCompletion(aReason)
      {
        if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          livemark._guid = aGUID;
        }
      }
    });
    stmt.finalize();
  },

  set guid(aGUID) {
    this._guid = aGUID;
    return aGUID;
  },
  get guid() this._guid,

  set lastModified(aLastModified) {
    this._lastModified = aLastModified;
    return aLastModified;
  },
  get lastModified() this._lastModified,

  







  updateChildren: function LM_updateChildren(aForceUpdate)
  {
    
    if (this.status == Ci.mozILivemark.STATUS_LOADING)
      return;

    
    
    if (!aForceUpdate && this.children.length && this.expireTime > Date.now())
      return;

    this.status = Ci.mozILivemark.STATUS_LOADING;

    
    if (this._terminated)
      return;

    try {
      
      
      
      let loadgroup = Cc["@mozilla.org/network/load-group;1"].
                      createInstance(Ci.nsILoadGroup);
      let channel = NetUtil.newChannel(this.feedURI.spec).
                    QueryInterface(Ci.nsIHttpChannel);
      channel.loadGroup = loadgroup;
      channel.loadFlags |= Ci.nsIRequest.LOAD_BACKGROUND |
                           Ci.nsIRequest.LOAD_BYPASS_CACHE;
      channel.requestMethod = "GET";
      channel.setRequestHeader("X-Moz", "livebookmarks", false);

      
      let listener = new LivemarkLoadListener(this);
      channel.notificationCallbacks = listener;
      channel.asyncOpen(listener, null);

      this.loadGroup = loadgroup;
    }
    catch (ex) {
      this.status = Ci.mozILivemark.STATUS_FAILED;
    }
  },

  reload: function LM_reload(aForceUpdate)
  {
    this.updateChildren(aForceUpdate);
  },

  remove: function LM_remove() {
    PlacesUtils.bookmarks.removeItem(this.id);
  },

  get children() this._children,
  set children(val) {
    this._children = val;

    
    for (let i = 0; i < this._resultObserversList.length; i++) {
      let container = this._resultObserversList[i];
      this._nodes.delete(container);
    }

    
    for (let i = 0; i < this._children.length; i++) {
      let child = this._children[i];
      asyncHistory.isURIVisited(child.uri,
        (function(aURI, aIsVisited) {
          this.updateURIVisitedStatus(aURI, aIsVisited);
        }).bind(this));
    }

    return this._children;
  },

  _isURIVisited: function LM__isURIVisited(aURI) {
    for (let i = 0; i < this.children.length; i++) {
      if (this.children[i].uri.equals(aURI)) {
        return this.children[i].visited;
      }
    }
  },

  getNodesForContainer: function LM_getNodesForContainer(aContainerNode)
  {
    if (this._nodes.has(aContainerNode)) {
      return this._nodes.get(aContainerNode);
    }

    let livemark = this;
    let nodes = [];
    let now = Date.now() * 1000;
    for (let i = 0; i < this._children.length; i++) {
      let child = this._children[i];
      let node = {
        
        
        
        get parent()
          aContainerNode.QueryInterface(Ci.nsINavHistoryContainerResultNode),
        get parentResult() this.parent.parentResult,
        get uri() child.uri.spec,
        get type() Ci.nsINavHistoryResultNode.RESULT_TYPE_URI,
        get title() child.title,
        get accessCount()
          Number(livemark._isURIVisited(NetUtil.newURI(this.uri))),
        get time() 0,
        get icon() "",
        get indentLevel() this.parent.indentLevel + 1,
        get bookmarkIndex() -1,
        get itemId() -1,
        get dateAdded() now + i,
        get lastModified() now + i,
        get tags()
          PlacesUtils.tagging.getTagsForURI(NetUtil.newURI(this.uri)).join(", "),
        QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryResultNode])
      };
      nodes.push(node);
    }
    this._nodes.set(aContainerNode, nodes);
    return nodes;
  },

  registerForUpdates: function LM_registerForUpdates(aContainerNode,
                                                     aResultObserver)
  {
    this._resultObservers.set(aContainerNode, aResultObserver);
    this._resultObserversList.push(aContainerNode);
  },

  unregisterForUpdates: function LM_unregisterForUpdates(aContainerNode)
  {
    this._resultObservers.delete(aContainerNode);
    let index = this._resultObserversList.indexOf(aContainerNode);
    this._resultObserversList.splice(index, 1);

    this._nodes.delete(aContainerNode);
  },

  _invalidateRegisteredContainers: function LM__invalidateRegisteredContainers()
  {
    for (let i = 0; i < this._resultObserversList.length; i++) {
      let container = this._resultObserversList[i];
      let observer = this._resultObservers.get(container);
      observer.invalidateContainer(container);
    }
  },

  updateURIVisitedStatus:
  function LM_updateURIVisitedStatus(aURI, aVisitedStatus)
  {
    for (let i = 0; i < this.children.length; i++) {
      if (this.children[i].uri.equals(aURI)) {
        this.children[i].visited = aVisitedStatus;
      }
    }

    for (let i = 0; i < this._resultObserversList.length; i++) {
      let container = this._resultObserversList[i];
      let observer = this._resultObservers.get(container);
      if (this._nodes.has(container)) {
        let nodes = this._nodes.get(container);
        for (let j = 0; j < nodes.length; j++) {
          let node = nodes[j];
          if (node.uri == aURI.spec) {
            Services.tm.mainThread.dispatch((function () {
              observer.nodeHistoryDetailsChanged(node, 0, aVisitedStatus);
            }).bind(this), Ci.nsIThread.DISPATCH_NORMAL);
          }
        }
      }
    }
  },

  



  terminate: function LM_terminate()
  {
    
    this._terminated = true;
    
    
    this._resultObserversList = [];
    this.abort();
  },

  


  abort: function LM_abort()
  {
    this.status = Ci.mozILivemark.STATUS_FAILED;
    if (this.loadGroup) {
      this.loadGroup.cancel(Cr.NS_BINDING_ABORTED);
      this.loadGroup = null;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.mozILivemark
  ])
}










function LivemarkLoadListener(aLivemark)
{
  this._livemark = aLivemark;
  this._processor = null;
  this._isAborted = false;
  this._ttl = EXPIRE_TIME_MS;
}

LivemarkLoadListener.prototype = {
  abort: function LLL_abort(aException)
  {
    if (!this._isAborted) {
      this._isAborted = true;
      this._livemark.abort();
      this._setResourceTTL(ONERROR_EXPIRE_TIME_MS);
    }
  },

  
  handleResult: function LLL_handleResult(aResult)
  {
    if (this._isAborted) {
      return;
    }

    try {
      
      let feedPrincipal =
        secMan.getSimpleCodebasePrincipal(this._livemark.feedURI);

      
      if (!aResult || !aResult.doc || aResult.bozo) {
        throw new Components.Exception("", Cr.NS_ERROR_FAILURE);
      }

      let feed = aResult.doc.QueryInterface(Ci.nsIFeed);
      let siteURI = this._livemark.siteURI;
      if (feed.link && (!siteURI || !feed.link.equals(siteURI))) {
        siteURI = feed.link;
        this._livemark.writeSiteURI(siteURI);
      }

      
      let livemarkChildren = [];
      for (let i = 0; i < feed.items.length; ++i) {
        let entry = feed.items.queryElementAt(i, Ci.nsIFeedEntry);
        let uri = entry.link || siteURI;
        if (!uri) {
          continue;
        }

        try {
          secMan.checkLoadURIWithPrincipal(feedPrincipal, uri, SEC_FLAGS);
        }
        catch(ex) {
          continue;
        }

        let title = entry.title ? entry.title.plainText() : "";
        livemarkChildren.push({ uri: uri, title: title, visited: false });
      }

      this._livemark.children = livemarkChildren;
    }
    catch (ex) {
      this.abort(ex);
    }
    finally {
      this._processor.listener = null;
      this._processor = null;
    }
  },

  onDataAvailable: function LLL_onDataAvailable(aRequest, aContext,
                                                aInputStream, aSourceOffset,
                                                aCount)
  {
    if (this._processor) {
      this._processor.onDataAvailable(aRequest, aContext, aInputStream,
                                      aSourceOffset, aCount);
    }
  },

  onStartRequest: function LLL_onStartRequest(aRequest, aContext)
  {
    if (this._isAborted) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    let channel = aRequest.QueryInterface(Ci.nsIChannel);
    try {
      
      this._processor = Cc["@mozilla.org/feed-processor;1"].
                        createInstance(Ci.nsIFeedProcessor);
      this._processor.listener = this;
      this._processor.parseAsync(null, channel.URI);
      this._processor.onStartRequest(aRequest, aContext);
    }
    catch (ex) {
      Components.utils.reportError("Livemark Service: feed processor received an invalid channel for " + channel.URI.spec);
      this.abort(ex);
    }
  },

  onStopRequest: function LLL_onStopRequest(aRequest, aContext, aStatus)
  {
    if (!Components.isSuccessCode(aStatus)) {
      this.abort();
      return;
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
            this._setResourceTTL(Math.max((expireTime - nowTime),
                                          EXPIRE_TIME_MS));
            return;
          }
        }
      }
      this._setResourceTTL(EXPIRE_TIME_MS);
    }
    catch (ex) {
      this.abort(ex);
    }
    finally {
      if (this._livemark.status == Ci.mozILivemark.STATUS_LOADING) {
        this._livemark.status = Ci.mozILivemark.STATUS_READY;
      }
      this._livemark.locked = false;
      this._livemark.loadGroup = null;
    }
  },

  _setResourceTTL: function LLL__setResourceTTL(aMilliseconds)
  {
    this._livemark.expireTime = Date.now() + aMilliseconds;
  },

  
  getInterface: function LLL_getInterface(aIID)
  {
    return this.QueryInterface(aIID);
  },

  
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIFeedResultListener
  , Ci.nsIStreamListener
  , Ci.nsIRequestObserver
  , Ci.nsIInterfaceRequestor
  ])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([LivemarkService]);
