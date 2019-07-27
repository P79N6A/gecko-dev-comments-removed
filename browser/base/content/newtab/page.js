#ifdef 0



#endif


const SCHEDULE_UPDATE_TIMEOUT_MS = 1000;





let gPage = {
  


  init: function Page_init() {
    
    gAllPages.register(this);

    
    addEventListener("unload", this, false);

    
    
    
    addEventListener("click", this, false);

    
    let enabled = gAllPages.enabled;
    if (enabled)
      this._init();

    this._updateAttributes(enabled);

    
    gCustomize.init();

    
    gIntro.init();
  },

  


  observe: function Page_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed") {
      gCustomize.updateSelected();

      let enabled = gAllPages.enabled;
      this._updateAttributes(enabled);

      
      if (aData == "browser.newtabpage.enhanced") {
        this.update();
        gIntro.showIfNecessary();
      }

      
      if (enabled) {
        this._init();
      } else {
        gUndoDialog.hide();
      }
    } else if (aTopic == "page-thumbnail:create" && gGrid.ready) {
      for (let site of gGrid.sites) {
        if (site && site.url === aData) {
          site.refreshThumbnail();
        }
      }
    }
  },

  







  update(reason = "") {
    
    if (!document.hidden) {
      
      
      
      if (reason != "links-changed" && gGrid.ready) {
        gGrid.refresh();
      }

      return;
    }

    
    if (this._scheduleUpdateTimeout) {
      return;
    }

    this._scheduleUpdateTimeout = setTimeout(() => {
      
      if (gGrid.ready) {
        gGrid.refresh();
      }

      this._scheduleUpdateTimeout = null;
    }, SCHEDULE_UPDATE_TIMEOUT_MS);
  },

  



  _init: function Page_init() {
    if (this._initialized)
      return;

    this._initialized = true;

    
    gSearch.init();

    if (document.hidden) {
      addEventListener("visibilitychange", this);
    } else {
      setTimeout(_ => this.onPageFirstVisible());
    }

    
    gGrid.init();

    
    gDropTargetShim.init();

#ifdef XP_MACOSX
    
    document.addEventListener("dragover", this, false);
    document.addEventListener("drop", this, false);
#endif
  },

  



  _updateAttributes: function Page_updateAttributes(aValue) {
    
    let nodeSelector = "#newtab-scrollbox, #newtab-grid, #newtab-search-container";
    for (let node of document.querySelectorAll(nodeSelector)) {
      if (aValue)
        node.removeAttribute("page-disabled");
      else
        node.setAttribute("page-disabled", "true");
    }

    
    let inputSelector = ".newtab-control, .newtab-link";
    for (let input of document.querySelectorAll(inputSelector)) {
      if (aValue) 
        input.removeAttribute("tabindex");
      else
        input.setAttribute("tabindex", "-1");
    }
  },

  


  _handleUnloadEvent: function Page_handleUnloadEvent() {
    gAllPages.unregister(this);
    
    
    
    let delta = Math.round((Date.now() - this._firstVisibleTime) / 500);
    if (this._suggestedTilePresent) {
      Services.telemetry.getHistogramById("NEWTAB_PAGE_LIFE_SPAN_SUGGESTED").add(delta);
    }
    else {
      Services.telemetry.getHistogramById("NEWTAB_PAGE_LIFE_SPAN").add(delta);
    }
  },

  


  handleEvent: function Page_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "load":
        this.onPageVisibleAndLoaded();
        break;
      case "unload":
        this._handleUnloadEvent();
        break;
      case "click":
        let {button, target} = aEvent;
        
        while (target) {
          if (target.hasOwnProperty("_newtabSite")) {
            target._newtabSite.onClick(aEvent);
            break;
          }
          target = target.parentNode;
        }
        break;
      case "dragover":
        if (gDrag.isValid(aEvent) && gDrag.draggedSite)
          aEvent.preventDefault();
        break;
      case "drop":
        if (gDrag.isValid(aEvent) && gDrag.draggedSite) {
          aEvent.preventDefault();
          aEvent.stopPropagation();
        }
        break;
      case "visibilitychange":
        
        if (this._scheduleUpdateTimeout) {
          clearTimeout(this._scheduleUpdateTimeout);
          this._scheduleUpdateTimeout = null;

          
          this.update();
        }

        setTimeout(() => this.onPageFirstVisible());
        removeEventListener("visibilitychange", this);
        break;
    }
  },

  onPageFirstVisible: function () {
    
    Services.telemetry.getHistogramById("NEWTAB_PAGE_SHOWN").add(true);

    for (let site of gGrid.sites) {
      if (site) {
        
        
        
        site.onFirstVisible();
      }
    }

    
    this._firstVisibleTime = Date.now();

    if (document.readyState == "complete") {
      this.onPageVisibleAndLoaded();
    } else {
      addEventListener("load", this);
    }
  },

  onPageVisibleAndLoaded() {
    
    this.reportLastVisibleTileIndex();

    
    gIntro.showIfNecessary();
  },

  reportLastVisibleTileIndex() {
    let cwu = window.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIDOMWindowUtils);

    let rect = cwu.getBoundsWithoutFlushing(gGrid.node);
    let nodes = cwu.nodesFromRect(rect.left, rect.top, 0, rect.width,
                                  rect.height, 0, true, false);

    let i = -1;
    let lastIndex = -1;
    let sites = gGrid.sites;

    for (let node of nodes) {
      if (node.classList && node.classList.contains("newtab-cell")) {
        if (sites[++i]) {
          lastIndex = i;
          if (sites[i].link.targetedSite) {
            
            this._suggestedTilePresent = true;
          }
        }
      }
    }

    DirectoryLinksProvider.reportSitesAction(sites, "view", lastIndex);
  }
};
