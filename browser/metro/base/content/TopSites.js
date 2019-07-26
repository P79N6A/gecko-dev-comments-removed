



'use strict';
 let prefs = Components.classes["@mozilla.org/preferences-service;1"].
      getService(Components.interfaces.nsIPrefBranch);
Cu.import("resource://gre/modules/PageThumbs.jsm");
Cu.import("resource:///modules/colorUtils.jsm");




let TopSites = {
  _initialized: false,

  Site: Site,

  prepareCache: function(aForce){
    
    

    
    if (this._promisedCache && !aForce) {
      return this._promisedCache;
    }
    let deferred = Promise.defer();
    this._promisedCache = deferred.promise;

    NewTabUtils.links.populateCache(function () {
      deferred.resolve();
      this._promisedCache = null;
      this._sites = null;  
      this._sitesDirty.clear();
    }.bind(this), true);
    return this._promisedCache;
  },

  _sites: null,
  _sitesDirty: new Set(),
  getSites: function() {
    if (this._sites) {
      return this._sites;
    }

    let links = NewTabUtils.links.getLinks();
    let sites = links.map(function(aLink){
      let site = new Site(aLink);
      return site;
    });

    
    this._sites = sites;
    this._sitesDirty.clear();
    return this._sites;
  },

  



  dirty: function() {
    
    for (let i=0; i<arguments.length; i++) {
      this._sitesDirty.add(arguments[i]);
    }
    return this._sitesDirty;
  },

  


  update: function() {
    NewTabUtils.allPages.update();
    
    this._sitesDirty.clear();
  },

  




  pinSites: function(aSites, aSlotIndices) {
    if (aSites.length !== aSlotIndices.length)
        throw new Error("TopSites.pinSites: Mismatched sites/indices arguments");

    for (let i=0; i<aSites.length && i<aSlotIndices.length; i++){
      let site = aSites[i],
          idx = aSlotIndices[i];
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }
      
      NewTabUtils.pinnedLinks.pin(site, idx);
      this.dirty(site);
    }
    this.update();
  },

  



  unpinSites: function(aSites) {
    for (let site of aSites) {
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }
      
      NewTabUtils.pinnedLinks.unpin(site);
      this.dirty(site);
    }
    this.update();
  },

  



  hideSites: function(aSites) {
    for (let site of aSites) {
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }

      site._restorePinIndex = NewTabUtils.pinnedLinks._indexOfLink(site);
      
      NewTabUtils.blockedLinks.block(site);
    }
    
    this._sites = null;
    this._sitesDirty.clear();
    this.update();
  },

  



  restoreSites: function(aSites) {
    for (let site of aSites) {
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }
      NewTabUtils.blockedLinks.unblock(site);
      let pinIndex = site._restorePinIndex;

      if (!isNaN(pinIndex) && pinIndex > -1) {
        NewTabUtils.pinnedLinks.pin(site, pinIndex);
      }
    }
    
    this._sites = null;
    this._sitesDirty.clear();
    this.update();
  },
  _linkFromNode: function _linkFromNode(aNode) {
    return {
      url: aNode.getAttribute("value"),
      title: aNode.getAttribute("label")
    };
  }
};


function TopSitesView(aGrid, aMaxSites, aUseThumbnails) {
  this._set = aGrid;
  this._set.controller = this;
  this._topSitesMax = aMaxSites;
  this._useThumbs = aUseThumbnails;
  
  window.addEventListener('MozAppbarDismissing', this, false);
  let history = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  history.addObserver(this, false);
  if (this._useThumbs) {
    PageThumbs.addExpirationFilter(this);
    Services.obs.addObserver(this, "Metro:RefreshTopsiteThumbnail", false);
  }

  NewTabUtils.allPages.register(this);
  TopSites.prepareCache().then(function(){
    this.populateGrid();
  }.bind(this));
}

TopSitesView.prototype = {
  _set:null,
  _topSitesMax: null,
  
  _lastSelectedSites: null,
  
  isUpdating: false,

  handleItemClick: function tabview_handleItemClick(aItem) {
    let url = aItem.getAttribute("value");
    BrowserUI.goToURI(url);
  },

  doActionOnSelectedTiles: function(aActionName, aEvent) {
    let tileGroup = this._set;
    let selectedTiles = tileGroup.selectedItems;
    let sites = Array.map(selectedTiles, TopSites._linkFromNode);
    let nextContextActions = new Set();

    switch (aActionName){
      case "delete":
        for (let aNode of selectedTiles) {
          
          aNode.contextActions.delete('delete');
          
        }
        this._lastSelectedSites = (this._lastSelectedSites || []).concat(sites);
        nextContextActions.add('restore');
        TopSites.hideSites(sites);
        break;
      case "restore":
        
        if (this._lastSelectedSites) {
          TopSites.restoreSites(this._lastSelectedSites);
        }
        break;
      case "pin":
        let pinIndices = [];
        Array.forEach(selectedTiles, function(aNode) {
          pinIndices.push( Array.indexOf(aNode.control.children, aNode) );
          aNode.contextActions.delete('pin');
          aNode.contextActions.add('unpin');
        });
        TopSites.pinSites(sites, pinIndices);
        break;
      case "unpin":
        Array.forEach(selectedTiles, function(aNode) {
          aNode.contextActions.delete('unpin');
          aNode.contextActions.add('pin');
        });
        TopSites.unpinSites(sites);
        break;
      
    }
    if (nextContextActions.size) {
      
      aEvent.preventDefault();
      
      setTimeout(function(){
        
        let event = document.createEvent("Events");
        event.actions = [...nextContextActions];
        event.noun = tileGroup.contextNoun;
        event.qty = selectedTiles.length;
        event.initEvent("MozContextActionsChange", true, false);
        tileGroup.dispatchEvent(event);
      },0);
    }
  },
  handleEvent: function(aEvent) {
    switch (aEvent.type){
      case "MozAppbarDismissing":
        
        this._lastSelectedSites = null;
    }
  },

  update: function() {
    
    let grid = this._set,
        dirtySites = TopSites.dirty();

    if (dirtySites.size) {
      
      for (let site of dirtySites) {
        let tileNode = grid.querySelector("[value='"+site.url+"']");
        if (tileNode) {
          this.updateTile(tileNode, new Site(site));
        }
      }
    } else {
        
      this.isUpdating = true;
      
      let item;
      while ((item = grid.firstChild)){
        grid.removeChild(item);
      }
      this.populateGrid();
    }
  },

  updateTile: function(aTileNode, aSite, aArrangeGrid) {
    PlacesUtils.favicons.getFaviconURLForPage(Util.makeURI(aSite.url), function(iconURLfromSiteURL) {
      if (!iconURLfromSiteURL) {
        return;
      }
      aTileNode.iconSrc = iconURLfromSiteURL.spec;
      let faviconURL = (PlacesUtils.favicons.getFaviconLinkForIcon(iconURLfromSiteURL)).spec;
      let xpFaviconURI = Util.makeURI(faviconURL.replace("moz-anno:favicon:",""));
      let successAction = function(foreground, background) {
	      aTileNode.style.color = foreground; 
        aTileNode.setAttribute("customColor", background);
        if (aTileNode.refresh) {
          aTileNode.refresh();
        }
      };
      let failureAction = function() {};
      ColorUtils.getForegroundAndBackgroundIconColors(xpFaviconURI, successAction, failureAction);
    });

    if (this._useThumbs) {
      Task.spawn(function() {
        let filepath = PageThumbsStorage.getFilePathForURL(aSite.url);
        if (yield OS.File.exists(filepath)) {
          aSite.backgroundImage = 'url("'+PageThumbs.getThumbnailURL(aSite.url)+'")';
          if ("isBound" in aTileNode && aTileNode.isBound) {
            aTileNode.backgroundImage = aSite.backgroundImage;
          }
        }
      });
    } else {
      delete aSite.backgroundImage;
    }
    aSite.applyToTileNode(aTileNode);
    if (aArrangeGrid) {
      this._set.arrangeItems();
    }
  },

  populateGrid: function populateGrid() {
    this.isUpdating = true;

    let sites = TopSites.getSites();
    let length = Math.min(sites.length, this._topSitesMax || Infinity);
    let tileset = this._set;

    
    
    while (tileset.children.length > length) {
      tileset.removeChild(tileset.children[tileset.children.length -1]);
    }

    for (let idx=0; idx < length; idx++) {
      let isNew = !tileset.children[idx],
          site = sites[idx];
      let item = isNew ? tileset.createItemElement(site.title, site.url) : tileset.children[idx];

      this.updateTile(item, site);
      if (isNew) {
        tileset.appendChild(item);
      }
    }
    tileset.arrangeItems();
    this.isUpdating = false;
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
    window.removeEventListener('MozAppbarDismissing', this, false);
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

  onDeleteVisits: function (aURI, aVisitTime, aGUID, aReason, aTransitionType) {
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
  },
  uninit: function uninit() {
    this._view.destruct();
  },

  show: function show() {
    this._grid.arrangeItems();
  },
};

let TopSitesSnappedView = {
  get _grid() { return document.getElementById("snapped-topsites-grid"); },

  show: function show() {
    this._grid.arrangeItems();
  },

  init: function() {
    this._view = new TopSitesView(this._grid, 8);
    if (this._view.isFirstRun()) {
      let topsitesVbox = document.getElementById("snapped-topsites");
      topsitesVbox.setAttribute("hidden", "true");
    }
    Services.obs.addObserver(this, "metro_viewstate_dom_snapped", false);
  },

  uninit: function uninit() {
    this._view.destruct();
    Services.obs.removeObserver(this, "metro_viewstate_dom_snapped");
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "metro_viewstate_dom_snapped":
          this._grid.arrangeItems();
        break;
    }
  },
};
