#ifdef 0



#endif




const GRID_BOTTOM_EXTRA = 7; 
const GRID_WIDTH_EXTRA = 1; 




let gGrid = {
  


  _node: null,
  get node() this._node,

  


  _siteFragment: null,

  


  _cells: null,
  get cells() this._cells,

  


  get sites() [cell.site for each (cell in this.cells)],

  
  get ready() !!this._ready,

  



  init: function Grid_init() {
    this._node = document.getElementById("newtab-grid");
    this._createSiteFragment();
    this._renderGrid();
    gLinks.populateCache(() => {
      this._renderSites();
      this._ready = true;
    });
    addEventListener("load", this);
    addEventListener("resize", this);

    
    if (document.readyState == "complete") {
      this.handleEvent({type: "load"});
    }
  },

  





  createSite: function Grid_createSite(aLink, aCell) {
    let node = aCell.node;
    node.appendChild(this._siteFragment.cloneNode(true));
    return new Site(node.firstElementChild, aLink);
  },

  


  handleEvent: function Grid_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "load":
      case "resize":
        this._resizeGrid();
        break;
    }
  },

  


  refresh: function Grid_refresh() {
    
    this.cells.forEach(function (cell) {
      let node = cell.node;
      let child = node.firstElementChild;

      if (child)
        node.removeChild(child);
    }, this);

    
    if (this._shouldRenderGrid()) {
      this._renderGrid();
      this._resizeGrid();
    }
    this._renderSites();
  },

  


  lock: function Grid_lock() {
    this.node.setAttribute("locked", "true");
  },

  


  unlock: function Grid_unlock() {
    this.node.removeAttribute("locked");
  },

  


  _renderGrid: function Grid_renderGrid() {
    let cell = document.createElementNS(HTML_NAMESPACE, "div");
    cell.classList.add("newtab-cell");

    
    this._node.innerHTML = "";

    
    for (let i = 0; i < gGridPrefs.gridColumns * gGridPrefs.gridRows; i++) {
      this._node.appendChild(cell.cloneNode(true));
    }

    
    let cellElements = this.node.querySelectorAll(".newtab-cell");
    this._cells = [new Cell(this, cell) for (cell of cellElements)];
  },

  



  _computeHeight: function Grid_computeHeight(aRows) {
    let {gridRows} = gGridPrefs;
    aRows = aRows === undefined ? gridRows : Math.min(gridRows, aRows);
    return aRows * this._cellHeight + GRID_BOTTOM_EXTRA;
  },

  


  _createSiteFragment: function Grid_createSiteFragment() {
    let site = document.createElementNS(HTML_NAMESPACE, "div");
    site.classList.add("newtab-site");
    site.setAttribute("draggable", "true");

    
    site.innerHTML =
      '<a class="newtab-link">' +
      '  <span class="newtab-thumbnail"/>' +
      '  <span class="newtab-thumbnail enhanced-content"/>' +
      '  <span class="newtab-title"/>' +
      '</a>' +
      '<input type="button" title="' + newTabString("pin") + '"' +
      '       class="newtab-control newtab-control-pin"/>' +
      '<input type="button" title="' + newTabString("block") + '"' +
      '       class="newtab-control newtab-control-block"/>' +
      '<span class="newtab-sponsored">' + newTabString("sponsored.button") + '</span>';

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

  


  _resizeGrid: function Grid_resizeGrid() {
    
    
    
    if (document.readyState != "complete") {
      return;
    }

    
    if (this._cellMargin === undefined) {
      let refCell = document.querySelector(".newtab-cell");
      this._cellMargin = parseFloat(getComputedStyle(refCell).marginTop) * 2;
      this._cellHeight = refCell.offsetHeight + this._cellMargin;
      this._cellWidth = refCell.offsetWidth + this._cellMargin;
    }

    let availSpace = document.documentElement.clientHeight - this._cellMargin -
                     document.querySelector("#newtab-search-container").offsetHeight;
    let visibleRows = Math.floor(availSpace / this._cellHeight);
    this._node.style.height = this._computeHeight() + "px";
    this._node.style.maxHeight = this._computeHeight(visibleRows) + "px";
    this._node.style.maxWidth = gGridPrefs.gridColumns * this._cellWidth +
                                GRID_WIDTH_EXTRA + "px";
  },

  _shouldRenderGrid : function Grid_shouldRenderGrid() {
    let cellsLength = this._node.querySelectorAll(".newtab-cell").length;
    return cellsLength != (gGridPrefs.gridRows * gGridPrefs.gridColumns);
  }
};
