#ifdef 0



#endif





function Site(aNode, aLink) {
  this._node = aNode;
  this._node._newtabSite = this;

  this._link = aLink;

  this._render();
  this._addEventHandlers();
}

Site.prototype = {
  


  get node() this._node,

  


  get link() this._link,

  


  get url() this.link.url,

  


  get title() this.link.title,

  


  get cell() {
    let parentNode = this.node.parentNode;
    return parentNode && parentNode._newtabCell;
  },

  



  pin: function Site_pin(aIndex) {
    if (typeof aIndex == "undefined")
      aIndex = this.cell.index;

    this._updateAttributes(true);
    gPinnedLinks.pin(this._link, aIndex);
  },

  


  unpin: function Site_unpin() {
    if (this.isPinned()) {
      this._updateAttributes(false);
      gPinnedLinks.unpin(this._link);
      gUpdater.updateGrid();
    }
  },

  



  isPinned: function Site_isPinned() {
    return gPinnedLinks.isPinned(this._link);
  },

  



  block: function Site_block() {
    if (!gBlockedLinks.isBlocked(this._link)) {
      gUndoDialog.show(this);
      gBlockedLinks.block(this._link);
      gUpdater.updateGrid();
    }
  },

  




  _querySelector: function Site_querySelector(aSelector) {
    return this.node.querySelector(aSelector);
  },

  




  _updateAttributes: function (aPinned) {
    let control = this._querySelector(".newtab-control-pin");

    if (aPinned) {
      control.setAttribute("pinned", true);
      control.setAttribute("title", newTabString("unpin"));
    } else {
      control.removeAttribute("pinned");
      control.setAttribute("title", newTabString("pin"));
    }
  },

  


  _render: function Site_render() {
    let url = this.url;
    let title = this.title || url;
    let tooltip = (title == url ? title : title + "\n" + url);

    let link = this._querySelector(".newtab-link");
    link.setAttribute("title", tooltip);
    link.setAttribute("href", url);
    this._querySelector(".newtab-title").textContent = title;
    this.node.setAttribute("type", this.link.type);

    if (this.isPinned())
      this._updateAttributes(true);
    
    
    this.captureIfMissing();
    
    this.refreshThumbnail();
  },

  



  captureIfMissing: function Site_captureIfMissing() {
    if (!document.hidden && !this.link.imageURI) {
      BackgroundPageThumbs.captureIfMissing(this.url);
    }
  },

  


  refreshThumbnail: function Site_refreshThumbnail() {
    let thumbnail = this._querySelector(".newtab-thumbnail");
    if (this.link.bgColor) {
      thumbnail.style.backgroundColor = this.link.bgColor;
    }
    let uri = this.link.imageURI || PageThumbs.getThumbnailURL(this.url);
    thumbnail.style.backgroundImage = "url(" + uri + ")";
  },

  


  _addEventHandlers: function Site_addEventHandlers() {
    
    this._node.addEventListener("dragstart", this, false);
    this._node.addEventListener("dragend", this, false);
    this._node.addEventListener("mouseover", this, false);

    
    let sponsored = this._querySelector(".newtab-control-sponsored");
    sponsored.addEventListener("mouseover", () => {
      this.cell.node.setAttribute("ignorehover", "true");
    });
    sponsored.addEventListener("mouseout", () => {
      this.cell.node.removeAttribute("ignorehover");
    });
  },

  


  _speculativeConnect: function Site_speculativeConnect() {
    let sc = Services.io.QueryInterface(Ci.nsISpeculativeConnect);
    let uri = Services.io.newURI(this.url, null, null);
    sc.speculativeConnect(uri, null);
  },

  


  _recordSiteClicked: function Site_recordSiteClicked(aIndex) {
    if (Services.prefs.prefHasUserValue("browser.newtabpage.rows") ||
        Services.prefs.prefHasUserValue("browser.newtabpage.columns") ||
        aIndex > 8) {
      
      
      aIndex = 9;
    }
    Services.telemetry.getHistogramById("NEWTAB_PAGE_SITE_CLICKED")
                      .add(aIndex);

    
    let typeIndex = DirectoryLinksProvider.linkTypes.indexOf(this.link.type);
    if (typeIndex != -1) {
      Services.telemetry.getHistogramById("NEWTAB_PAGE_DIRECTORY_TYPE_CLICKED")
                        .add(typeIndex);
    }
  },

  


  onClick: function Site_onClick(aEvent) {
    let {button, target} = aEvent;
    if (target.classList.contains("newtab-link") ||
        target.parentElement.classList.contains("newtab-link")) {
      
      if (button == 0 || button == 1) {
        this._recordSiteClicked(this.cell.index);
      }
      return;
    }

    
    if (button != 0) {
      return;
    }

    aEvent.preventDefault();
    if (aEvent.target.classList.contains("newtab-control-block"))
      this.block();
    else if (target.classList.contains("newtab-control-sponsored"))
      gPage.showSponsoredPanel(target);
    else if (this.isPinned())
      this.unpin();
    else
      this.pin();
  },

  


  handleEvent: function Site_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "mouseover":
        this._node.removeEventListener("mouseover", this, false);
        this._speculativeConnect();
        break;
      case "dragstart":
        gDrag.start(this, aEvent);
        break;
      case "dragend":
        gDrag.end(this, aEvent);
        break;
    }
  }
};
