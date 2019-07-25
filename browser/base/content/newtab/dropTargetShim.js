#ifdef 0



#endif






let gDropTargetShim = {
  


  _cellPositions: null,

  


  _lastDropTarget: null,

  


  init: function DropTargetShim_init() {
    let node = gGrid.node;

    this._dragover = this._dragover.bind(this);

    
    node.addEventListener("dragstart", this._start.bind(this), true);
    
    
    node.addEventListener("dragend", this._end.bind(this), true);
  },

  



  _start: function DropTargetShim_start(aEvent) {
    gGrid.lock();

    
    document.documentElement.addEventListener("dragover", this._dragover, false);
  },

  



  _drag: function DropTargetShim_drag(aEvent) {
    
    let target = this._findDropTarget(aEvent);

    if (target == this._lastDropTarget) {
      
      


    } else {
      if (this._lastDropTarget)
        
        this._dispatchEvent(aEvent, "dragexit", this._lastDropTarget);

      if (target)
        
        this._dispatchEvent(aEvent, "dragenter", target);

      if (this._lastDropTarget)
        
        this._dispatchEvent(aEvent, "dragleave", this._lastDropTarget);

      this._lastDropTarget = target;
    }
  },

  




  _dragover: function DropTargetShim_dragover(aEvent) {
    let sourceNode = aEvent.dataTransfer.mozSourceNode;
    gDrag.drag(sourceNode._newtabSite, aEvent);

    this._drag(aEvent);
  },

  



  _end: function DropTargetShim_end(aEvent) {
    
    
    this._drag(aEvent);

    if (this._lastDropTarget) {
      if (aEvent.dataTransfer.mozUserCancelled) {
        
        this._dispatchEvent(aEvent, "dragexit", this._lastDropTarget);
        this._dispatchEvent(aEvent, "dragleave", this._lastDropTarget);
      } else {
        
        this._dispatchEvent(aEvent, "drop", this._lastDropTarget);
      }

      
      this._lastDropTarget = null;
      this._cellPositions = null;
    }

    gGrid.unlock();

    
    document.documentElement.removeEventListener("dragover", this._dragover, false);
  },

  




  _findDropTarget: function DropTargetShim_findDropTarget() {
    
    
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

  





  _dispatchEvent:
    function DropTargetShim_dispatchEvent(aEvent, aType, aTarget) {

    let node = aTarget.node;
    let event = document.createEvent("DragEvents");

    event.initDragEvent(aType, true, true, window, 0, 0, 0, 0, 0, false, false,
                        false, false, 0, node, aEvent.dataTransfer);

    node.dispatchEvent(event);
  }
};
