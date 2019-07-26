



"use strict"

let prefs = Components.classes["@mozilla.org/preferences-service;1"].
      getService(Components.interfaces.nsIPrefBranch);

Cu.import("resource://gre/modules/PageThumbs.jsm");
Cu.import("resource:///modules/colorUtils.jsm");

function TopSitesView(aGrid, aMaxSites) {
  View.call(this, aGrid);

  this._topSitesMax = aMaxSites;

  
  StartUI.chromeWin.addEventListener('MozAppbarDismissing', this, false);
  let history = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  history.addObserver(this, false);

  Services.obs.addObserver(this, "Metro:RefreshTopsiteThumbnail", false);

  NewTabUtils.allPages.register(this);
  TopSites.prepareCache().then(function(){
    this.populateGrid();
  }.bind(this));
}

TopSitesView.prototype = Util.extend(Object.create(View.prototype), {
  _set:null,
  _topSitesMax: null,
  
  _lastSelectedSites: null,
  
  isUpdating: false,

  destruct: function destruct() {
    Services.obs.removeObserver(this, "Metro:RefreshTopsiteThumbnail");
    NewTabUtils.allPages.unregister(this);
    if (StartUI.chromeWin) {
      StartUI.chromeWin.removeEventListener('MozAppbarDismissing', this, false);
    }
    View.prototype.destruct.call(this);
  },

  handleItemClick: function tabview_handleItemClick(aItem) {
    let url = aItem.getAttribute("value");
    StartUI.goToURI(url);
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
        
        aEvent.preventDefault();
        nextContextActions.add('restore');
        TopSites.hideSites(sites);
        break;
      case "restore":
        
        if (this._lastSelectedSites) {
          let selectedUrls = this._lastSelectedSites.map((site) => site.url);
          
          tileGroup.addEventListener("arranged", function _onArranged(aEvent){
            for (let url of selectedUrls) {
              let tileNode = tileGroup.querySelector("richgriditem[value='"+url+"']");
              if (tileNode) {
                tileNode.setAttribute("selected", true);
              }
            }
            tileGroup.removeEventListener("arranged", _onArranged, false);
            
            
            
            let event = tileGroup.ownerDocument.createEvent("Events");
            event.initEvent("selectionchange", true, true);
            tileGroup.dispatchEvent(event);
          }, false);

          TopSites.restoreSites(this._lastSelectedSites);
          
          
          aEvent.preventDefault();
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
      
      setTimeout(function(){
        
        let event = document.createEvent("Events");
        event.actions = [...nextContextActions];
        event.initEvent("MozContextActionsChange", true, false);
        tileGroup.dispatchEvent(event);
      },0);
    }
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type){
      case "MozAppbarDismissing":
        
        this._lastSelectedSites = null;
        break;
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
      
      grid.clearAll(true);
      this.populateGrid();
    }
  },

  updateTile: function(aTileNode, aSite, aArrangeGrid) {
    this._updateFavicon(aTileNode, Util.makeURI(aSite.url));

    Task.spawn(function() {
      let filepath = PageThumbsStorage.getFilePathForURL(aSite.url);
      if (yield OS.File.exists(filepath)) {
        aSite.backgroundImage = 'url("'+PageThumbs.getThumbnailURL(aSite.url)+'")';
        aTileNode.setAttribute("customImage", aSite.backgroundImage);
        if (aTileNode.refresh) {
          aTileNode.refresh()
        }
      }
    });

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
      if ("isBound" in item && item.isBound) {
        item.refreshBackgroundImage();
      }
    }
  },

  isFirstRun: function isFirstRun() {
    return prefs.getBoolPref("browser.firstrun.show.localepicker");
  },

  _adjustDOMforViewState: function _adjustDOMforViewState(aState) {
    if (!this._set)
      return;
    if (!aState)
      aState = this._set.getAttribute("viewstate");

    View.prototype._adjustDOMforViewState.call(this, aState);

    
    if (aState == "snapped") {
      document.getElementById("start-topsites-grid").removeAttribute("tiletype");
    } else {
      document.getElementById("start-topsites-grid").setAttribute("tiletype", "thumbnail");
    }

    
    let tileType = this._set.getAttribute("tiletype");
    for (let item of this._set.children) {
      if (tileType) {
        item.setAttribute("tiletype", tileType);
      } else {
        item.removeAttribute("tiletype");
      }
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

  onDeleteVisits: function (aURI, aVisitTime, aGUID, aReason, aTransitionType) {
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsINavHistoryObserver) ||
        iid.equals(Components.interfaces.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }

});

let TopSitesStartView = {
  _view: null,
  get _grid() { return document.getElementById("start-topsites-grid"); },

  init: function init() {
    this._view = new TopSitesView(this._grid, 8);
    if (this._view.isFirstRun()) {
      let topsitesVbox = document.getElementById("start-topsites");
      topsitesVbox.setAttribute("hidden", "true");
    }
  },

  uninit: function uninit() {
    if (this._view) {
      this._view.destruct();
    }
  },
};
