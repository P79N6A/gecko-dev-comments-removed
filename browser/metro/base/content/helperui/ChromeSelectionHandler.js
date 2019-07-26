







const kCaretMode = 1;
const kSelectionMode = 2;

var ChromeSelectionHandler = {
  _mode: kSelectionMode,

  



  sendAsync: function sendAsync(aMsg, aJson) {
    SelectionHelperUI.receiveMessage({ 
      name: aMsg,
      json: aJson
    });
  },

  



  


  _onSelectionAttach: function _onSelectionAttach(aJson) {
    this._domWinUtils = Util.getWindowUtils(window);
    this._contentWindow = window;
    this._targetElement = this._domWinUtils.elementFromPoint(aJson.xPos, aJson.yPos, true, false);

    this._targetIsEditable = this._targetElement instanceof Components.interfaces.nsIDOMXULTextBoxElement;
    if (!this._targetIsEditable) {
      this._onFail("not an editable?");
      return;
    }

    let selection = this._getSelection();
    if (!selection) {
      this._onFail("no selection.");
      return;
    }

    if (!selection.isCollapsed) {
      this._mode = kSelectionMode;
      this._updateSelectionUI("start", true, true);
    } else {
      this._mode = kCaretMode;
      this._updateSelectionUI("caret", false, false, true);
    }

    this._targetElement.addEventListener("blur", this, false);
  },

  


  _onSelectionMoveStart: function _onSelectionMoveStart(aMsg) {
    if (!this.targetIsEditable) {
      this._onFail("_onSelectionMoveStart with bad targetElement.");
      return;
    }

    if (this._selectionMoveActive) {
      this._onFail("mouse is already down on drag start?");
      return;
    }

    
    this._selectionMoveActive = true;

    if (this._targetIsEditable) {
      
      
      
      
      this._updateInputFocus(aMsg.change);
    }

    
    this._updateSelectionUI("update", true, true);
  },
  
  


  _onSelectionMove: function _onSelectionMove(aMsg) {
    if (!this.targetIsEditable) {
      this._onFail("_onSelectionMove with bad targetElement.");
      return;
    }

    if (!this._selectionMoveActive) {
      this._onFail("mouse isn't down for drag move?");
      return;
    }

    
    let pos = null;
    if (aMsg.change == "start") {
      pos = aMsg.start;
    } else {
      pos = aMsg.end;
    }
    this._handleSelectionPoint(aMsg.change, pos, false);
  },

  


  _onSelectionMoveEnd: function _onSelectionMoveComplete(aMsg) {
    if (!this.targetIsEditable) {
      this._onFail("_onSelectionMoveEnd with bad targetElement.");
      return;
    }

    if (!this._selectionMoveActive) {
      this._onFail("mouse isn't down for drag move?");
      return;
    }

    
    let pos = null;
    if (aMsg.change == "start") {
      pos = aMsg.start;
    } else {
      pos = aMsg.end;
    }

    this._handleSelectionPoint(aMsg.change, pos, true);
    this._selectionMoveActive = false;
    
    
    this._clearTimers();

    
    this._updateSelectionUI("end", true, true);
  },

  _onSelectionUpdate: function _onSelectionUpdate() {
    if (!this._targetHasFocus()) {
      this._closeSelection();
      return;
    }
    this._updateSelectionUI("update",
                            this._mode == kSelectionMode,
                            this._mode == kSelectionMode,
                            this._mode == kCaretMode);
  },

  



  _onSwitchMode: function _onSwitchMode(aMode, aMarker, aX, aY) {
    if (aMode != "selection") {
      this._onFail("unsupported mode switch");
      return;
    }
    
    
    if (!this._targetElement) {
      this._onFail("not initialized");
      return;
    }

    
    
    let framePoint = this._clientPointToFramePoint({ xPos: aX, yPos: aY });
    if (!this._domWinUtils.selectAtPoint(framePoint.xPos, framePoint.yPos,
                                         Ci.nsIDOMWindowUtils.SELECT_CHARACTER)) {
      this._onFail("failed to set selection at point");
      return;
    }

    
    this._selectionMoveActive = true;
    this._mode = kSelectionMode;

    
    
    this._updateSelectionUI("update", aMarker == "end", aMarker == "start");
  },

  




  _onSelectionClose: function _onSelectionClose(aClearSelection) {
    if (aClearSelection) {
      this._clearSelection();
    }
    this._closeSelection();
  },

  



  _onFail: function _onFail(aDbgMessage) {
    if (aDbgMessage && aDbgMessage.length > 0)
      Util.dumpLn(aDbgMessage);
    this.sendAsync("Content:SelectionFail");
    this._clearSelection();
    this._closeSelection();
  },

  





  ptClientToBrowser: function ptClientToBrowser(aX, aY, aIgnoreScroll, aIgnoreScale) {
    return { x: aX, y: aY }
  },

  rectBrowserToClient: function rectBrowserToClient(aRect, aIgnoreScroll, aIgnoreScale) {
    return {
      left: aRect.left,
      right: aRect.right,
      top: aRect.top,
      bottom: aRect.bottom
    }
  },

  ptBrowserToClient: function ptBrowserToClient(aX, aY, aIgnoreScroll, aIgnoreScale) {
    return { x: aX, y: aY }
  },

  ctobx: function ctobx(aCoord) {
    return aCoord;
  },

  ctoby: function ctoby(aCoord) {
    return aCoord;
  },

  btocx: function btocx(aCoord) {
    return aCoord;
  },

  btocy: function btocy(aCoord) {
    return aCoord;
  },


  



  




  _clearSelection: function _clearSelection() {
    let selection = this._getSelection();
    if (selection) {
      selection.removeAllRanges();
    }
  },

  




  _closeSelection: function _closeSelection() {
    this._clearTimers();
    this._cache = null;
    this._contentWindow = null;
    if (this._targetElement) {
      this._targetElement.removeEventListener("blur", this, true);
      this._targetElement = null;
    }
    this._selectionMoveActive = false;
    this._domWinUtils = null;
    this._targetIsEditable = false;
    this.sendAsync("Content:HandlerShutdown", {});
  },

  get hasSelection() {
    if (!this._targetElement) {
      return false;
    }
    let selection = this._getSelection();
    return (selection && !selection.isCollapsed);
  },

  _targetHasFocus: function() {
    if (!this._targetElement || !document.commandDispatcher.focusedElement) {
      return false;
    }
    let bindingParent = this._contentWindow.document.getBindingParent(document.commandDispatcher.focusedElement);
    return (bindingParent && this._targetElement == bindingParent);
  },

  



  



  scrollTimerCallback: function scrollTimerCallback() {
    let result = ChromeSelectionHandler.updateTextEditSelection();
    
    if (result.trigger) {
      ChromeSelectionHandler._updateSelectionUI("update", result.start, result.end);
    }
  },

  handleEvent: function handleEvent(aEvent) {
    if (aEvent.type == "blur" && !this._targetHasFocus()) {
      this._closeSelection();
    }
  },

  msgHandler: function msgHandler(aMsg, aJson) {
    if (this._debugEvents && "Browser:SelectionMove" != aMsg) {
      Util.dumpLn("ChromeSelectionHandler:", aMsg);
    }
    switch(aMsg) {
      case "Browser:SelectionDebug":
        this._onSelectionDebug(aJson);
        break;

      case "Browser:SelectionAttach":
        this._onSelectionAttach(aJson);
      break;

      case "Browser:CaretAttach":
        this._onSelectionAttach(aJson);
        break;

      case "Browser:SelectionClose":
        this._onSelectionClose(aJson.clearSelection);
      break;

      case "Browser:SelectionUpdate":
        this._onSelectionUpdate();
      break;

      case "Browser:SelectionMoveStart":
        this._onSelectionMoveStart(aJson);
        break;

      case "Browser:SelectionMove":
        this._onSelectionMove(aJson);
        break;

      case "Browser:SelectionMoveEnd":
        this._onSelectionMoveEnd(aJson);
        break;

      case "Browser:CaretUpdate":
        this._onCaretPositionUpdate(aJson.caret.xPos, aJson.caret.yPos);
        break;

      case "Browser:CaretMove":
        this._onCaretMove(aJson.caret.xPos, aJson.caret.yPos);
        break;

      case "Browser:SelectionSwitchMode":
        this._onSwitchMode(aJson.newMode, aJson.change, aJson.xPos, aJson.yPos);
        break;
    }
  },

  



  _getSelection: function _getSelection() {
    if (this._targetElement instanceof Ci.nsIDOMXULTextBoxElement) {
      return this._targetElement
                 .QueryInterface(Components.interfaces.nsIDOMXULTextBoxElement)
                 .editor.selection;
    }
    return null;
  },

  _getSelectController: function _getSelectController() {
    return this._targetElement
                .QueryInterface(Components.interfaces.nsIDOMXULTextBoxElement)
                .editor.selectionController;
  },
};

ChromeSelectionHandler.__proto__ = new SelectionPrototype();
ChromeSelectionHandler.type = 1; 

