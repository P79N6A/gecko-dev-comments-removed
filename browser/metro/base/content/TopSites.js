



'use strict';
 let prefs = Components.classes["@mozilla.org/preferences-service;1"].
      getService(Components.interfaces.nsIPrefBranch);

Cu.import("resource://gre/modules/PageThumbs.jsm");


let TopSites = {
  pinSite: function(aId, aSlotIndex) {
    Util.dumpLn("TopSites.pinSite: " + aId + ", (TODO)");
    
    return true; 
  },
  unpinSite: function(aId) {
    Util.dumpLn("TopSites.unpinSite: " + aId + ", (TODO)");
    
    return true; 
  },
  hideSite: function(aId) {
    Util.dumpLn("TopSites.hideSite: " + aId + ", (TODO)");
    
    return true; 
  },
  restoreSite: function(aId) {
    Util.dumpLn("TopSites.restoreSite: " + aId + ", (TODO)");
    
    return true; 
  }
};



function TopSitesView(aGrid, aMaxSites, aUseThumbnails) {
  this._set = aGrid;
  this._set.controller = this;
  this._topSitesMax = aMaxSites;
  this._useThumbs = aUseThumbnails;

  
  this._set.addEventListener("context-action", this, false);

  let history = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  history.addObserver(this, false);
  if (this._useThumbs) {
    PageThumbs.addExpirationFilter(this);
    Services.obs.addObserver(this, "Metro:RefreshTopsiteThumbnail", false);
  }
}

TopSitesView.prototype = {
  _set:null,
  _topSitesMax: null,

  handleItemClick: function tabview_handleItemClick(aItem) {
    let url = aItem.getAttribute("value");
    BrowserUI.goToURI(url);
  },

  doActionOnSelectedTiles: function(aActionName) {
    let tileGroup = this._set;
    let selectedTiles = tileGroup.selectedItems;

    switch (aActionName){
      case "delete":
        Array.forEach(selectedTiles, function(aNode) {
          let id = aNode.getAttribute("data-itemid");
          
          if (TopSites.hideSite(id)) {
            
            aNode.contextActions.delete('delete');
            aNode.contextActions.add('restore');
          }
          
        });
        break;
      case "pin":
        Array.forEach(selectedTiles, function(aNode) {
          let id = aNode.getAttribute("data-itemid");
          if (TopSites.pinSite(id)) {
            
            aNode.contextActions.delete('pin');
            aNode.contextActions.add('unpin');
          }
          
          
        });
        break;
      case "unpin":
        Array.forEach(selectedTiles, function(aNode) {
          let id = aNode.getAttribute("data-itemid");
          if (TopSites.unpinSite(id)) {
            
            aNode.contextActions.delete('unpin');
            aNode.contextActions.add('pin');
          }
          
          
        });
        break;
      
    }
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type){
      case "context-action":
        this.doActionOnSelectedTiles(aEvent.action);
        break;
    }
  },

  populateGrid: function populateGrid() {
    let query = gHistSvc.getNewQuery();
    let options = gHistSvc.getNewQueryOptions();
    options.excludeQueries = true;
    options.queryType = options.QUERY_TYPE_HISTORY;
    options.maxResults = this._topSitesMax;
    options.resultType = options.RESULTS_AS_URI;
    options.sortingMode = options.SORT_BY_FRECENCY_DESCENDING;

    let result = gHistSvc.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    let childCount = rootNode.childCount;

    
    
    let identifier = 'uri';

    function isPinned(aNode) {
      
      
      return (aNode.uri.indexOf('google') > -1);
    }

    for (let i = 0; i < childCount; i++) {
      let node = rootNode.getChild(i);
      let uri = node.uri;
      let title = node.title || uri;

      let supportedActions = ['delete'];
      
      if (isPinned(node)) {
        supportedActions.push('unpin');
      } else {
        supportedActions.push('pin');
      }
      let item = this._set.appendItem(title, uri);
      item.setAttribute("iconURI", node.icon);
      item.setAttribute("data-itemid", node[identifier]);
      
      item.setAttribute("data-contextactions", supportedActions.join(','));

      if (this._useThumbs) {
        let thumbnail = PageThumbs.getThumbnailURL(uri);
        let cssthumbnail = 'url("'+thumbnail+'")';
        item.backgroundImage = cssthumbnail;
      }
    }
    rootNode.containerOpen = false;
  },

  forceReloadOfThumbnail: function forceReloadOfThumbnail(url) {
      let nodes = this._set.querySelectorAll('richgriditem[value="'+url+'"]');
      for (let item of nodes) {
        item.refreshBackgroundImage();
      }
  },
  filterForThumbnailExpiration: function filterForThumbnailExpiration(aCallback) {
    aCallback([item.getAttribute("value") for (item of this._set.children)]);
  },

  isFirstRun: function isFirstRun() {
    return prefs.getBoolPref("browser.firstrun.show.localepicker");
  },

  destruct: function destruct() {
    if (this._useThumbs) {
      Services.obs.removeObserver(this, "Metro:RefreshTopsiteThumbnail");
      PageThumbs.removeExpirationFilter(this);
    }
  },

  
  observe: function (aSubject, aTopic, aState) {
    switch(aTopic) {
      case "Metro:RefreshTopsiteThumbnail":
        this.forceReloadOfThumbnail(aState);
        break;
    }
  },
  

  onBeginUpdateBatch: function() {
  },

  onEndUpdateBatch: function() {
  },

  onVisit: function(aURI, aVisitID, aTime, aSessionID,
                    aReferringID, aTransitionType) {
  },

  onTitleChanged: function(aURI, aPageTitle) {
  },

  onDeleteURI: function(aURI) {
  },

  onClearHistory: function() {
    this._set.clearAll();
  },

  onPageChanged: function(aURI, aWhat, aValue) {
  },

  onPageExpired: function(aURI, aVisitTime, aWholeEntry) {
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsINavHistoryObserver) ||
        iid.equals(Components.interfaces.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }

};

let TopSitesStartView = {
  _view: null,
  get _grid() { return document.getElementById("start-topsites-grid"); },

  init: function init() {
    this._view = new TopSitesView(this._grid, 8, true);
    if (this._view.isFirstRun()) {
      let topsitesVbox = document.getElementById("start-topsites");
      topsitesVbox.setAttribute("hidden", "true");
    }
    this._view.populateGrid();
  },

  uninit: function uninit() {
    this._view.destruct();
  },

  show: function show() {
    this._grid.arrangeItems(3, 3);
  },
};

let TopSitesSnappedView = {
  get _grid() { return document.getElementById("snapped-topsite-grid"); },

  show: function show() {
    this._grid.arrangeItems(1, 8);
  },

  init: function() {
    this._view = new TopSitesView(this._grid, 8);
    if (this._view.isFirstRun()) {
      let topsitesVbox = document.getElementById("snapped-topsites");
      topsitesVbox.setAttribute("hidden", "true");
    }
    this._view.populateGrid();
  },

  uninit: function uninit() {
    this._view.destruct();
  },
};
