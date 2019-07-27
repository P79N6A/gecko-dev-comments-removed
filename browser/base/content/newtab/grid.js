#ifdef 0



#endif




const GRID_BOTTOM_EXTRA = 7; 
const GRID_WIDTH_EXTRA = 1; 




let gGrid = {
  


  _node: null,
  get node() { return this._node; },

  


  _siteFragment: null,

  


  _cells: [],
  get cells() { return this._cells; },

  


  get sites() { return [for (cell of this.cells) cell.site]; },

  
  get ready() { return !!this._ready; },

  
  get isDocumentLoaded() { return document.readyState == "complete"; },

  



  init: function Grid_init() {
    this._node = document.getElementById("newtab-grid");
    this._createSiteFragment();

    gLinks.populateCache(() => {
      this.refresh();
      this._ready = true;

      
      
      
      
      this._resizeGrid();

      addEventListener("resize", this);
    });

    
    if (!this.isDocumentLoaded) {
      addEventListener("load", this);
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

  


  lock: function Grid_lock() {
    this.node.setAttribute("locked", "true");
  },

  


  unlock: function Grid_unlock() {
    this.node.removeAttribute("locked");
  },

  


  refresh() {
    let cell = document.createElementNS(HTML_NAMESPACE, "div");
    cell.classList.add("newtab-cell");

    
    let fragment = document.createDocumentFragment();
    for (let i = 0; i < gGridPrefs.gridColumns * gGridPrefs.gridRows; i++) {
      fragment.appendChild(cell.cloneNode(true));
    }

    
    let cells = [new Cell(this, cell) for (cell of fragment.childNodes)];

    
    let links = gLinks.getLinks();

    
    let numLinks = Math.min(links.length, cells.length);
    for (let i = 0; i < numLinks; i++) {
      if (links[i]) {
        this.createSite(links[i], cells[i]);
      }
    }

    this._cells = cells;
    this._node.innerHTML = "";
    this._node.appendChild(fragment);
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
      '<span class="newtab-sponsored">' + newTabString("sponsored.button") + '</span>' +
      '<a class="newtab-link">' +
      '  <span class="newtab-thumbnail"/>' +
      '  <span class="newtab-thumbnail enhanced-content"/>' +
      '  <span class="newtab-title"/>' +
      '</a>' +
      '<input type="button" title="' + newTabString("pin") + '"' +
      '       class="newtab-control newtab-control-pin"/>' +
      '<input type="button" title="' + newTabString("block") + '"' +
      '       class="newtab-control newtab-control-block"/>' +
      '<span class="newtab-suggested"/>';

    this._siteFragment = document.createDocumentFragment();
    this._siteFragment.appendChild(site);
  },

  


  _resizeGrid: function Grid_resizeGrid() {
    
    
    
    
    if (!this.isDocumentLoaded || !this._ready) {
      return;
    }

    
    if (this._cellMargin === undefined) {
      let refCell = document.querySelector(".newtab-cell");
      this._cellMargin = parseFloat(getComputedStyle(refCell).marginTop);
      this._cellHeight = refCell.offsetHeight + this._cellMargin +
        parseFloat(getComputedStyle(refCell).marginBottom);
      this._cellWidth = refCell.offsetWidth + this._cellMargin;
    }

    let availSpace = document.documentElement.clientHeight - this._cellMargin -
                     document.querySelector("#newtab-search-container").offsetHeight;
    let visibleRows = Math.floor(availSpace / this._cellHeight);
    this._node.style.height = this._computeHeight() + "px";
    this._node.style.maxHeight = this._computeHeight(visibleRows) + "px";
    this._node.style.maxWidth = gGridPrefs.gridColumns * this._cellWidth +
                                GRID_WIDTH_EXTRA + "px";
  }
};
