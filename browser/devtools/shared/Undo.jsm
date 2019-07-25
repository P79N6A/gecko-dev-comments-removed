




const Cu = Components.utils;

var EXPORTED_SYMBOLS=["UndoStack"];













function UndoStack(aChange, aMaxUndo)
{
  this.maxUndo = aMaxUndo || 50;
  this._stack = [];
}

UndoStack.prototype = {
  
  _index: 0,

  
  _batchDepth: 0,

  destroy: function Undo_destroy()
  {
    this.uninstallController();
    delete this._stack;
  },

  








  startBatch: function Undo_startBatch()
  {
    if (this._batchDepth++ === 0) {
      this._batch = [];
    }
  },

  



  endBatch: function Undo_endBatch()
  {
    if (--this._batchDepth > 0) {
      return;
    }

    
    let start = Math.max(++this._index - this.maxUndo, 0);
    this._stack = this._stack.slice(start, this._index);

    let batch = this._batch;
    delete this._batch;
    let entry = {
      do: function() {
        for (let item of batch) {
          item.do();
        }
      },
      undo: function() {
        for (let i = batch.length - 1; i >= 0; i--) {
          batch[i].undo();
        }
      }
    };
    this._stack.push(entry);
    entry.do();
    this._change();
  },

  





  do: function Undo_do(aDo, aUndo) {
    this.startBatch();
    this._batch.push({ do: aDo, undo: aUndo });
    this.endBatch();
  },

  


  canUndo: function Undo_canUndo()
  {
    return this._index > 0;
  },

  




  undo: function Undo_canUndo()
  {
    if (!this.canUndo()) {
      return false;
    }
    this._stack[--this._index].undo();
    this._change();
    return true;
  },

  


  canRedo: function Undo_canRedo()
  {
    return this._stack.length >= this._index;
  },

  




  redo: function Undo_canRedo()
  {
    if (!this.canRedo()) {
      return false;
    }
    this._stack[this._index++].do();
    this._change();
    return true;
  },

  _change: function Undo__change()
  {
    if (this._controllerWindow) {
      this._controllerWindow.goUpdateCommand("cmd_undo");
      this._controllerWindow.goUpdateCommand("cmd_redo");
    }
  },

  



  


  installController: function Undo_installController(aControllerWindow)
  {
    this._controllerWindow = aControllerWindow;
    aControllerWindow.controllers.appendController(this);
  },

  


  uninstallController: function Undo_uninstallController()
  {
    if (!this._controllerWindow) {
      return;
    }
    this._controllerWindow.controllers.removeController(this);
  },

  supportsCommand: function Undo_supportsCommand(aCommand)
  {
    return (aCommand == "cmd_undo" ||
            aCommand == "cmd_redo");
  },

  isCommandEnabled: function Undo_isCommandEnabled(aCommand)
  {
    switch(aCommand) {
      case "cmd_undo": return this.canUndo();
      case "cmd_redo": return this.canRedo();
    };
    return false;
  },

  doCommand: function Undo_doCommand(aCommand)
  {
    switch(aCommand) {
      case "cmd_undo": return this.undo();
      case "cmd_redo": return this.redo();
    }
  },

  onEvent: function Undo_onEvent(aEvent) {},
}
