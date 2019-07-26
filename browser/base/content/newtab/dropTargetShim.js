#ifdef 0



#endif






let gDropTargetShim = {
  


  _cellPositions: null,

  


  _lastDropTarget: null,

  


  init: function () {
    gGrid.node.addEventListener("dragstart", this, true);
  },

  


  _addEventListeners: function () {
    gGrid.node.addEventListener("dragend", this);

    let docElement = document.documentElement;
    docElement.addEventListener("dragover", this);
    docElement.addEventListener("dragenter", this);
    docElement.addEventListener("drop", this);
  },

  


  _removeEventListeners: function () {
    gGrid.node.removeEventListener("dragend", this);

    let docElement = document.documentElement;
    docElement.removeEventListener("dragover", this);
    docElement.removeEventListener("dragenter", this);
    docElement.removeEventListener("drop", this);
  },

  


  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "dragstart":
        this._dragstart(aEvent);
        break;
      case "dragenter":
        aEvent.preventDefault();
        break;
      case "dragover":
        this._dragover(aEvent);
        break;
      case "drop":
        this._drop(aEvent);
        break;
      case "dragend":
        this._dragend(aEvent);
        break;
    }
  },

  



  _dragstart: function (aEvent) {
    if (aEvent.target.classList.contains("newtab-link")) {
      gGrid.lock();
      this._addEventListeners();
    }
  },

  



  _dragover: function (aEvent) {
    
    
    let sourceNode = aEvent.dataTransfer.mozSourceNode.parentNode;
    gDrag.drag(sourceNode._newtabSite, aEvent);

    
    this._updateDropTarget(aEvent);

    
    
    if (this._lastDropTarget) {
      aEvent.preventDefault();
    }
  },

  



  _drop: function (aEvent) {
    
    aEvent.preventDefault();

    
    
    this._updateDropTarget(aEvent);

    
    this._dispatchEvent(aEvent, "drop", this._lastDropTarget);
  },

  



  _dragend: function (aEvent) {
    if (this._lastDropTarget) {
      if (aEvent.dataTransfer.mozUserCancelled) {
        
        this._dispatchEvent(aEvent, "dragexit", this._lastDropTarget);
        this._dispatchEvent(aEvent, "dragleave", this._lastDropTarget);
      }

      
      this._lastDropTarget = null;
      this._cellPositions = null;
    }

    gGrid.unlock();
    this._removeEventListeners();
  },

  




  _updateDropTarget: function (aEvent) {
    
    let target = this._findDropTarget(aEvent);

    if (target != this._lastDropTarget) {
      if (this._lastDropTarget)
        
        this._dispatchEvent(aEvent, "dragexit", this._lastDropTarget);

      if (target)
        
        this._dispatchEvent(aEvent, "dragenter", target);

      if (this._lastDropTarget)
        
        this._dispatchEvent(aEvent, "dragleave", this._lastDropTarget);

      this._lastDropTarget = target;
    }
  },

  




  _findDropTarget: function () {
    
    
    let minWidth = gDrag.cellWidth / 2;
    let minHeight = gDrag.cellHeight / 2;

    let cellPositions = this._getCellPositions();
    let rect = gTransformation.getNodePosition(gDrag.draggedSite.node);

    
    for (let i = 0; i < cellPositions.length; i++) {
      let inter = rect.intersect(cellPositions[i].rect);

      
      if (inter.width >= minWidth && inter.height >= minHeight)
        return cellPositions[i].cell;
    }

    
    return null;
  },

  



  _getCellPositions: function DropTargetShim_getCellPositions() {
    if (this._cellPositions)
      return this._cellPositions;

    return this._cellPositions = gGrid.cells.map(function (cell) {
      return {cell: cell, rect: gTransformation.getNodePosition(cell.node)};
    });
  },

  





  _dispatchEvent: function (aEvent, aType, aTarget) {
    let node = aTarget.node;
    let event = document.createEvent("DragEvents");

    
    event.initDragEvent(aType, false, true, window, 0, 0, 0, 0, 0, false, false,
                        false, false, 0, node, aEvent.dataTransfer);

    node.dispatchEvent(event);
  }
};
