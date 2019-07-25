#ifdef 0



#endif




let gGrid = {
  


  _node: null,
  get node() this._node,

  


  _siteFragment: null,

  


  _cells: null,
  get cells() this._cells,

  


  get sites() [cell.site for each (cell in this.cells)],

  



  init: function Grid_init() {
    this._node = document.getElementById("newtab-grid");
    this._createSiteFragment();
    this._render();
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

    
    this._render();
  },

  


  lock: function Grid_lock() {
    this.node.setAttribute("locked", "true");
  },

  


  unlock: function Grid_unlock() {
    this.node.removeAttribute("locked");
  },

  


  _renderGrid: function Grid_renderGrid() {
    let row = document.createElementNS(HTML_NAMESPACE, "div");
    let cell = document.createElementNS(HTML_NAMESPACE, "div");
    row.classList.add("newtab-row");
    cell.classList.add("newtab-cell");

    
    this._node.innerHTML = "";

    
    for (let i = 0; i < gGridPrefs.gridColumns; i++) {
      row.appendChild(cell.cloneNode(true));
    }
    
    for (let j = 0; j < gGridPrefs.gridRows; j++) {
      this._node.appendChild(row.cloneNode(true));
    }

    
    let cellElements = this.node.querySelectorAll(".newtab-cell");
    this._cells = [new Cell(this, cell) for (cell of cellElements)];
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

  


  _renderSites: function Grid_renderSites() {
    let cells = this.cells;
    
    let links = gLinks.getLinks();
    let length = Math.min(links.length, cells.length);

    for (let i = 0; i < length; i++) {
      if (links[i])
        this.createSite(links[i], cells[i]);
    }
  },

  


  _render: function Grid_render() {
    if (this._shouldRenderGrid()) {
      this._renderGrid();
    }

    this._renderSites();
  },

  _shouldRenderGrid : function Grid_shouldRenderGrid() {
    let rowsLength = this._node.querySelectorAll(".newtab-row").length;
    let cellsLength = this._node.querySelectorAll(".newtab-cell").length;

    return (rowsLength != gGridPrefs.gridRows ||
            cellsLength != (gGridPrefs.gridRows * gGridPrefs.gridColumns));
  }
};
