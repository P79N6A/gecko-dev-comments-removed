#ifdef 0



#endif





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

  




  update: function Page_update(aOnlyIfHidden=false) {
    let skipUpdate = aOnlyIfHidden && !document.hidden;
    
    if (gGrid.ready && !skipUpdate) {
      gGrid.refresh();
    }
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

  


  handleEvent: function Page_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "unload":
        gAllPages.unregister(this);
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
        setTimeout(() => this.onPageFirstVisible());
        removeEventListener("visibilitychange", this);
        break;
    }
  },

  onPageFirstVisible: function () {
    
    Services.telemetry.getHistogramById("NEWTAB_PAGE_SHOWN").add(true);

    for (let site of gGrid.sites) {
      if (site) {
        site.captureIfMissing();
      }
    }

    
    let i = 0;
    let checkSizing = _ => setTimeout(_ => {
      if (document.documentElement.clientWidth == 0) {
        checkSizing();
      }
      else {
        this.onPageFirstSized();
      }
    });
    checkSizing();
  },

  onPageFirstSized: function() {
    
    let {sites} = gGrid;
    let lastIndex = sites.length;
    while (lastIndex-- > 0) {
      let site = sites[lastIndex];
      if (site) {
        let {node} = site;
        let rect = node.getBoundingClientRect();
        let target = document.elementFromPoint(rect.x + rect.width / 2,
                                               rect.y + rect.height / 2);
        if (node.contains(target)) {
          break;
        }
      }
    }

    DirectoryLinksProvider.reportSitesAction(gGrid.sites, "view", lastIndex);

    
    gIntro.showIfNecessary();
  }
};
