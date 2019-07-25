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

  



  unpin: function Site_unpin(aCallback) {
    if (this.isPinned()) {
      this._updateAttributes(false);
      gPinnedLinks.unpin(this._link);
      gUpdater.updateGrid(aCallback);
    }
  },

  



  isPinned: function Site_isPinned() {
    return gPinnedLinks.isPinned(this._link);
  },

  




  block: function Site_block(aCallback) {
    gBlockedLinks.block(this._link);
    gUpdater.updateGrid(aCallback);
    gPage.updateModifiedFlag();
  },

  




  _querySelector: function Site_querySelector(aSelector) {
    return this.node.querySelector(aSelector);
  },

  




  _updateAttributes: function (aPinned) {
    let buttonPin = this._querySelector(".strip-button-pin");

    if (aPinned) {
      this.node.setAttribute("pinned", true);
      buttonPin.setAttribute("title", newTabString("unpin"));
    } else {
      this.node.removeAttribute("pinned");
      buttonPin.setAttribute("title", newTabString("pin"));
    }
  },

  


  _render: function Site_render() {
    let title = this.title || this.url;
    this.node.setAttribute("title", title);
    this.node.setAttribute("href", this.url);
    this._querySelector(".site-title").textContent = title;

    if (this.isPinned())
      this._updateAttributes(true);

    this._renderThumbnail();
  },

  


  _renderThumbnail: function Site_renderThumbnail() {
    let img = this._querySelector(".site-img")
    img.setAttribute("alt", this.title || this.url);
    img.setAttribute("loading", "true");

    
    img.addEventListener("load", function onLoad() {
      img.removeEventListener("load", onLoad, false);
      img.removeAttribute("loading");
    }, false);

    
    img.setAttribute("src", PageThumbs.getThumbnailURL(this.url));
  },

  


  _addEventHandlers: function Site_addEventHandlers() {
    
    ["DragStart",  "DragEnd"].forEach(function (aType) {
      let method = "_on" + aType;
      this[method] = this[method].bind(this);
      this._node.addEventListener(aType.toLowerCase(), this[method], false);
    }, this);

    let self = this;

    function pin(aEvent) {
      if (aEvent)
        aEvent.preventDefault();

      if (self.isPinned())
        self.unpin();
      else
        self.pin();
    }

    function block(aEvent) {
      if (aEvent)
        aEvent.preventDefault();

      self.block();
    }

    this._querySelector(".strip-button-pin").addEventListener("click", pin, false);
    this._querySelector(".strip-button-block").addEventListener("click", block, false);
  },

  



  _onDragStart: function Site_onDragStart(aEvent) {
    gDrag.start(this, aEvent);
  },

  



  _onDrag: function Site_onDrag(aEvent) {
    gDrag.drag(this, aEvent);
  },

  



  _onDragEnd: function Site_onDragEnd(aEvent) {
    gDrag.end(this, aEvent);
  }
};
