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
    if (gBlockedLinks.isBlocked(this._link)) {
      if (aCallback)
        aCallback();
    } else {
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
    let title = this.title || this.url;
    let link = this._querySelector(".newtab-link");
    link.setAttribute("title", title);
    link.setAttribute("href", this.url);
    this._querySelector(".newtab-title").textContent = title;

    if (this.isPinned())
      this._updateAttributes(true);

    let thumbnailURL = PageThumbs.getThumbnailURL(this.url);
    let thumbnail = this._querySelector(".newtab-thumbnail");
    thumbnail.style.backgroundImage = "url(" + thumbnailURL + ")";
  },

  


  _addEventHandlers: function Site_addEventHandlers() {
    
    this._node.addEventListener("dragstart", this, false);
    this._node.addEventListener("dragend", this, false);

    let controls = this.node.querySelectorAll(".newtab-control");
    for (let i = 0; i < controls.length; i++)
      controls[i].addEventListener("click", this, false);
  },

  


  handleEvent: function Site_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "click":
        aEvent.preventDefault();
        if (aEvent.target.classList.contains("newtab-control-block"))
          this.block();
        else if (this.isPinned())
          this.unpin();
        else
          this.pin();
        break;
      case "dragstart":
        gDrag.start(this, aEvent);
        break;
      case "drag":
        gDrag.drag(this, aEvent);
        break;
      case "dragend":
        gDrag.end(this, aEvent);
        break;
    }
  }
};
