#ifdef 0



#endif




let gGrid = {
  


  _node: null,
  get node() this._node,

  


  _siteFragment: null,

  


  get cells() {
    let children = this.node.querySelectorAll("li");
    let cells = [new Cell(this, child) for each (child in children)];

    
    Object.defineProperty(this, "cells", {value: cells, enumerable: true});

    return cells;
  },

  


  get sites() [cell.site for each (cell in this.cells)],

  



  init: function Grid_init(aSelector) {
    this._node = document.querySelector(aSelector);
    this._createSiteFragment();
    this._draw();
  },

  





  createSite: function Grid_createSite(aLink, aCell) {
    let node = aCell.node;
    node.appendChild(this._siteFragment.cloneNode(true));
    return new Site(node.firstElementChild, aLink);
  },

  


  refresh: function Grid_refresh() {
    
    this.cells.forEach(function (cell) {
      let node = cell.node;
      let child = node.firstElementChild;

      if (child)
        node.removeChild(child);
    }, this);

    
    this._draw();
  },

  


  lock: function Grid_lock() {
    this.node.setAttribute("locked", "true");
  },

  


  unlock: function Grid_unlock() {
    this.node.removeAttribute("locked");
  },

  


  _createSiteFragment: function Grid_createSiteFragment() {
    let site = document.createElementNS(HTML_NAMESPACE, "a");
    site.classList.add("site");
    site.setAttribute("draggable", "true");

    
    site.innerHTML =
      '<img class="site-img" width="' + THUMB_WIDTH +'" ' +
      ' height="' + THUMB_HEIGHT + '" alt=""/>' +
      '<span class="site-title"/>' +
      '<span class="site-strip">' +
      '  <input class="button strip-button strip-button-pin" type="button"' +
      '   tabindex="-1" title="' + newTabString("pin") + '"/>' +
      '  <input class="button strip-button strip-button-block" type="button"' +
      '   tabindex="-1" title="' + newTabString("block") + '"/>' +
      '</span>';

    this._siteFragment = document.createDocumentFragment();
    this._siteFragment.appendChild(site);
  },

  


  _draw: function Grid_draw() {
    let cells = this.cells;

    
    let links = gLinks.getLinks();
    let length = Math.min(links.length, cells.length);

    for (let i = 0; i < length; i++) {
      if (links[i])
        this.createSite(links[i], cells[i]);
    }
  }
};
