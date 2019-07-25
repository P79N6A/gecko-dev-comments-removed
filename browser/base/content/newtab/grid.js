#ifdef 0



#endif




let gGrid = {
  


  _node: null,
  get node() this._node,

  


  _siteFragment: null,

  


  get cells() {
    let cells = [];
    let children = this.node.querySelectorAll(".newtab-cell");
    for (let i = 0; i < children.length; i++)
      cells.push(new Cell(this, children[i]));

    
    Object.defineProperty(this, "cells", {value: cells, enumerable: true});

    return cells;
  },

  


  get sites() [cell.site for each (cell in this.cells)],

  



  init: function Grid_init() {
    this._node = document.getElementById("newtab-grid");
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
    let site = document.createElementNS(HTML_NAMESPACE, "div");
    site.classList.add("newtab-site");
    site.setAttribute("draggable", "true");

    
    site.innerHTML =
      '<a class="newtab-link">' +
      '  <span class="newtab-thumbnail"/>' +
      '  <span class="newtab-title"/>' +
      '</a>' +
      '<input type="button" title="' + newTabString("pin") + '"' +
      '       class="newtab-control newtab-control-pin"/>' +
      '<input type="button" title="' + newTabString("block") + '"' +
      '       class="newtab-control newtab-control-block"/>';

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
