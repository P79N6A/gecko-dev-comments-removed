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
  


  get node() { return this._node; },

  


  get link() { return this._link; },

  


  get url() { return this.link.url; },

  


  get title() { return this.link.title || this.link.url; },

  


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
      this.node.setAttribute("pinned", true);
      control.setAttribute("title", newTabString("unpin"));
    } else {
      this.node.removeAttribute("pinned");
      control.setAttribute("title", newTabString("pin"));
    }
  },

  _newTabString: function(str, substrArr) {
    let regExp = /%[0-9]\$S/g;
    let matches;
    while (matches = regExp.exec(str)) {
      let match = matches[0];
      let index = match.charAt(1); 
      str = str.replace(match, substrArr[index - 1]);
    }
    return str;
  },

  _getSuggestedTileExplanation: function() {
    let targetedName = `<strong> ${this.link.targetedName} </strong>`;
    let targetedSite = `<strong> ${this.link.targetedSite} </strong>`;
    if (this.link.explanation) {
      return this._newTabString(this.link.explanation, [targetedName, targetedSite]);
    }
    return newTabString("suggested.button", [targetedName]);
  },

  


  _checkLinkEndTime: function Site_checkLinkEndTime() {
    if (this.link.endTime && this.link.endTime < Date.now()) {
       let oldUrl = this.url;
       
       this.link.url = Services.io.newURI(this.url, null, null).resolve("/");
       
       delete this.link.imageURI;
       delete this.link.enhancedImageURI;
       
       delete this.link.endTime;
       
       this._querySelector(".enhanced-content").style.backgroundImage = "";
       gPinnedLinks.replace(oldUrl, this.link);
    }
  },

  


  _render: function Site_render() {
    
    this._checkLinkEndTime();
    
    let enhanced = gAllPages.enhanced && DirectoryLinksProvider.getEnhancedLink(this.link);
    let url = this.url;
    let title = enhanced && enhanced.title ? enhanced.title :
                this.link.type == "history" ? this.link.baseDomain :
                this.title;
    let tooltip = (this.title == url ? this.title : this.title + "\n" + url);

    let link = this._querySelector(".newtab-link");
    link.setAttribute("title", tooltip);
    link.setAttribute("href", url);
    this._querySelector(".newtab-title").textContent = title;
    this.node.setAttribute("type", this.link.type);

    if (this.link.targetedSite) {
      if (this.node.getAttribute("type") != "sponsored") {
        this._querySelector(".newtab-sponsored").textContent =
          newTabString("suggested.tag");
      }

      this.node.setAttribute("suggested", true);
      let explanation = this._getSuggestedTileExplanation();
      this._querySelector(".newtab-suggested").innerHTML =
        `<div class='newtab-suggested-bounds'> ${explanation} </div>`;
    }

    if (this.isPinned())
      this._updateAttributes(true);
    
    
    this.captureIfMissing();
    
    this.refreshThumbnail();
  },

  




  onFirstVisible: function Site_onFirstVisible() {
    if (this.link.endTime && this.link.endTime < Date.now()) {
      
      this._render();
    }
    else {
      this.captureIfMissing();
    }
  },

  



  captureIfMissing: function Site_captureIfMissing() {
    if (!document.hidden && !this.link.imageURI) {
      BackgroundPageThumbs.captureIfMissing(this.url);
    }
  },

  


  refreshThumbnail: function Site_refreshThumbnail() {
    
    let link = gAllPages.enhanced && DirectoryLinksProvider.getEnhancedLink(this.link) ||
               this.link;

    let thumbnail = this._querySelector(".newtab-thumbnail");
    if (link.bgColor) {
      thumbnail.style.backgroundColor = link.bgColor;
    }

    let uri = link.imageURI || PageThumbs.getThumbnailURL(this.url);
    thumbnail.style.backgroundImage = 'url("' + uri + '")';

    if (link.enhancedImageURI) {
      let enhanced = this._querySelector(".enhanced-content");
      enhanced.style.backgroundImage = 'url("' + link.enhancedImageURI + '")';

      if (this.link.type != link.type) {
        this.node.setAttribute("type", "enhanced");
        this.enhancedId = link.directoryId;
      }
    }
  },

  _ignoreHoverEvents: function(element) {
    element.addEventListener("mouseover", () => {
      this.cell.node.setAttribute("ignorehover", "true");
    });
    element.addEventListener("mouseout", () => {
      this.cell.node.removeAttribute("ignorehover");
    });
  },

  


  _addEventHandlers: function Site_addEventHandlers() {
    
    this._node.addEventListener("dragstart", this, false);
    this._node.addEventListener("dragend", this, false);
    this._node.addEventListener("mouseover", this, false);

    
    
    let sponsored = this._querySelector(".newtab-sponsored");
    let suggested = this._querySelector(".newtab-suggested");
    this._ignoreHoverEvents(sponsored);
    this._ignoreHoverEvents(suggested);
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
  },

  _toggleLegalText: function(buttonClass, explanationTextClass) {
    let button = this._querySelector(buttonClass);
    if (button.hasAttribute("active")) {
      let explain = this._querySelector(explanationTextClass);
      explain.parentNode.removeChild(explain);

      button.removeAttribute("active");
    }
    else {
      let explain = document.createElementNS(HTML_NAMESPACE, "div");
      explain.className = explanationTextClass.slice(1); 
      this.node.appendChild(explain);

      let link = '<a href="' + TILES_EXPLAIN_LINK + '">' +
                 newTabString("learn.link") + "</a>";
      let type = (this.node.getAttribute("suggested") && this.node.getAttribute("type") == "affiliate") ?
                  "suggested" : this.node.getAttribute("type");
      let icon = '<input type="button" class="newtab-control newtab-' +
                 (type == "enhanced" ? "customize" : "control-block") + '"/>';
      explain.innerHTML = newTabString(type + (type == "sponsored" ? ".explain2" : ".explain"), [icon, link]);

      button.setAttribute("active", "true");
    }
  },

  


  onClick: function Site_onClick(aEvent) {
    let action;
    let pinned = this.isPinned();
    let tileIndex = this.cell.index;
    let {button, target} = aEvent;

    
    if (target.classList.contains("newtab-link") ||
        target.parentElement.classList.contains("newtab-link")) {
      
      if (button == 0 || button == 1) {
        this._recordSiteClicked(tileIndex);
        action = "click";
      }
    }
    
    else if (target.parentElement.classList.contains("sponsored-explain")) {
      action = "sponsored_link";
    }
    else if (target.parentElement.classList.contains("suggested-explain")) {
      action = "suggested_link";
    }
    
    else if (button == 0) {
      aEvent.preventDefault();
      if (target.classList.contains("newtab-control-block")) {
        this.block();
        action = "block";
      }
      else if (target.classList.contains("sponsored-explain") ||
               target.classList.contains("newtab-sponsored")) {
        this._toggleLegalText(".newtab-sponsored", ".sponsored-explain");
        action = "sponsored";
      }
      else if (pinned && target.classList.contains("newtab-control-pin")) {
        this.unpin();
        action = "unpin";
      }
      else if (!pinned && target.classList.contains("newtab-control-pin")) {
        this.pin();
        action = "pin";
      }
    }

    
    if (action) {
      DirectoryLinksProvider.reportSitesAction(gGrid.sites, action, tileIndex);
    }
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
