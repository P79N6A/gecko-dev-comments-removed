#ifdef 0



#endif





let gPage = {
  


  init: function Page_init() {
    
    gAllPages.register(this);

    
    addEventListener("unload", this, false);

    
    let button = document.getElementById("newtab-toggle");
    button.addEventListener("click", this, false);

    
    this._sponsoredPanel = document.getElementById("sponsored-panel");
    let link = this._sponsoredPanel.querySelector(".text-link");
    link.addEventListener("click", () => this._sponsoredPanel.hidePopup());

    
    let enabled = gAllPages.enabled;
    if (enabled)
      this._init();

    this._updateAttributes(enabled);
  },

  



  get allowBackgroundCaptures() {
    
    
    
    return inPrivateBrowsingMode() ||
           document.documentElement.getAttribute("allow-background-captures") ==
           "true";
  },

  


  observe: function Page_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed") {
      let enabled = gAllPages.enabled;
      this._updateAttributes(enabled);

      
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
    let skipUpdate = aOnlyIfHidden && this.allowBackgroundCaptures;
    
    if (gGrid.ready && !skipUpdate) {
      gGrid.refresh();
    }
  },

  


  showSponsoredPanel: function Page_showSponsoredPanel(aTarget) {
    if (this._sponsoredPanel.state == "closed") {
      let self = this;
      this._sponsoredPanel.addEventListener("popuphidden", function onPopupHidden(aEvent) {
        self._sponsoredPanel.removeEventListener("popuphidden", onPopupHidden, false);
        aTarget.removeAttribute("panelShown");
      });
    }
    aTarget.setAttribute("panelShown", "true");
    this._sponsoredPanel.openPopup(aTarget);
  },

  



  _init: function Page_init() {
    if (this._initialized)
      return;

    this._initialized = true;

    this._mutationObserver = new MutationObserver(() => {
      if (this.allowBackgroundCaptures) {
        Services.telemetry.getHistogramById("NEWTAB_PAGE_SHOWN").add(true);

        
        let directoryCount = {};
        for (let type of DirectoryLinksProvider.linkTypes) {
          directoryCount[type] = 0;
        }

        for (let site of gGrid.sites) {
          if (site) {
            site.captureIfMissing();
            let {type} = site.link;
            if (type in directoryCount) {
              directoryCount[type]++;
            }
          }
        }

        
        
        for (let [type, count] of Iterator(directoryCount)) {
          let shownId = "NEWTAB_PAGE_DIRECTORY_" + type.toUpperCase() + "_SHOWN";
          let shownCount = Math.min(10, count);
          Services.telemetry.getHistogramById(shownId).add(shownCount);
        }
      }
    });
    this._mutationObserver.observe(document.documentElement, {
      attributes: true,
      attributeFilter: ["allow-background-captures"],
    });

    gLinks.populateCache(function () {
      
      gGrid.init();

      
      gDropTargetShim.init();

#ifdef XP_MACOSX
      
      document.addEventListener("dragover", this, false);
      document.addEventListener("drop", this, false);
#endif
    }.bind(this));
  },

  



  _updateAttributes: function Page_updateAttributes(aValue) {
    
    let nodeSelector = "#newtab-scrollbox, #newtab-toggle, #newtab-grid";
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

    
    let toggle = document.getElementById("newtab-toggle");
    toggle.setAttribute("title", newTabString(aValue ? "hide" : "show"));
  },

  


  handleEvent: function Page_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "unload":
        if (this._mutationObserver)
          this._mutationObserver.disconnect();
        gAllPages.unregister(this);
        break;
      case "click":
        gAllPages.enabled = !gAllPages.enabled;
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
    }
  }
};
