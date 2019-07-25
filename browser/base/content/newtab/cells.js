#ifdef 0



#endif






function Cell(aGrid, aNode) {
  this._grid = aGrid;
  this._node = aNode;
  this._node._newtabCell = this;

  
  ["DragEnter", "DragOver", "DragExit", "Drop"].forEach(function (aType) {
    let method = "on" + aType;
    this[method] = this[method].bind(this);
    this._node.addEventListener(aType.toLowerCase(), this[method], false);
  }, this);
}

Cell.prototype = {
  


  _grid: null,

  


  get node() this._node,

  


  get index() {
    let index = this._grid.cells.indexOf(this);

    
    Object.defineProperty(this, "index", {value: index, enumerable: true});

    return index;
  },

  


  get previousSibling() {
    let prev = this.node.previousElementSibling;
    prev = prev && prev._newtabCell;

    
    Object.defineProperty(this, "previousSibling", {value: prev, enumerable: true});

    return prev;
  },

  


  get nextSibling() {
    let next = this.node.nextElementSibling;
    next = next && next._newtabCell;

    
    Object.defineProperty(this, "nextSibling", {value: next, enumerable: true});

    return next;
  },

  


  get site() {
    let firstChild = this.node.firstElementChild;
    return firstChild && firstChild._newtabSite;
  },

  



  containsPinnedSite: function Cell_containsPinnedSite() {
    let site = this.site;
    return site && site.isPinned();
  },

  



  isEmpty: function Cell_isEmpty() {
    return !this.site;
  },

  



  onDragEnter: function Cell_onDragEnter(aEvent) {
    if (gDrag.isValid(aEvent)) {
      aEvent.preventDefault();
      gDrop.enter(this, aEvent);
    }
  },

  



  onDragOver: function Cell_onDragOver(aEvent) {
    if (gDrag.isValid(aEvent))
      aEvent.preventDefault();
  },

  



  onDragExit: function Cell_onDragExit(aEvent) {
    gDrop.exit(this, aEvent);
  },

  



  onDrop: function Cell_onDrop(aEvent) {
    if (gDrag.isValid(aEvent)) {
      aEvent.preventDefault();
      gDrop.drop(this, aEvent);
    }
  }
};
