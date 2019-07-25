#ifdef 0



#endif






function Cell(aGrid, aNode) {
  this._grid = aGrid;
  this._node = aNode;
  this._node._newtabCell = this;

  
  ["dragenter", "dragover", "dragexit", "drop"].forEach(function (aType) {
    this._node.addEventListener(aType, this, false);
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

  


  handleEvent: function Cell_handleEvent(aEvent) {
    if (aEvent.type != "dragexit" && !gDrag.isValid(aEvent))
      return;

    switch (aEvent.type) {
      case "dragenter":
        aEvent.preventDefault();
        gDrop.enter(this, aEvent);
        break;
      case "dragover":
        aEvent.preventDefault();
        break;
      case "dragexit":
        gDrop.exit(this, aEvent);
        break;
      case "drop":
        aEvent.preventDefault();
        gDrop.drop(this, aEvent);
        break;
    }
  }
};
