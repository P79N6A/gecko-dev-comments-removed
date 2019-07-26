



dump("### SelectionHandler.js loaded\n");

var SelectionHandler = {
  init: function init() {
    this.type = kContentSelector;
    this.snap = true;
    addMessageListener("Browser:SelectionStart", this);
    addMessageListener("Browser:SelectionAttach", this);
    addMessageListener("Browser:SelectionEnd", this);
    addMessageListener("Browser:SelectionMoveStart", this);
    addMessageListener("Browser:SelectionMove", this);
    addMessageListener("Browser:SelectionMoveEnd", this);
    addMessageListener("Browser:SelectionUpdate", this);
    addMessageListener("Browser:SelectionClose", this);
    addMessageListener("Browser:SelectionCopy", this);
    addMessageListener("Browser:SelectionDebug", this);
    addMessageListener("Browser:CaretAttach", this);
    addMessageListener("Browser:CaretMove", this);
    addMessageListener("Browser:CaretUpdate", this);
    addMessageListener("Browser:SelectionSwitchMode", this);
    addMessageListener("Browser:RepositionInfoRequest", this);
    addMessageListener("Browser:SelectionHandlerPing", this);
  },

  shutdown: function shutdown() {
    removeMessageListener("Browser:SelectionStart", this);
    removeMessageListener("Browser:SelectionAttach", this);
    removeMessageListener("Browser:SelectionEnd", this);
    removeMessageListener("Browser:SelectionMoveStart", this);
    removeMessageListener("Browser:SelectionMove", this);
    removeMessageListener("Browser:SelectionMoveEnd", this);
    removeMessageListener("Browser:SelectionUpdate", this);
    removeMessageListener("Browser:SelectionClose", this);
    removeMessageListener("Browser:SelectionCopy", this);
    removeMessageListener("Browser:SelectionDebug", this);
    removeMessageListener("Browser:CaretAttach", this);
    removeMessageListener("Browser:CaretMove", this);
    removeMessageListener("Browser:CaretUpdate", this);
    removeMessageListener("Browser:SelectionSwitchMode", this);
    removeMessageListener("Browser:RepositionInfoRequest", this);
    removeMessageListener("Browser:SelectionHandlerPing", this);
  },

  sendAsync: function sendAsync(aMsg, aJson) {
    sendAsyncMessage(aMsg, aJson);
  },

  



  


  _onSelectionStart: function _onSelectionStart(aJson) {
    
    if (!this._initTargetInfo(aJson.xPos, aJson.yPos)) {
      this._onFail("failed to get target information");
      return;
    }

    
    
    if (aJson.setFocus && this._targetIsEditable) {
      this._targetElement.focus();
    }

    
    let selection = this._contentWindow.getSelection();
    selection.removeAllRanges();

    
    let framePoint = this._clientPointToFramePoint({ xPos: aJson.xPos, yPos: aJson.yPos });
    if (!this._domWinUtils.selectAtPoint(framePoint.xPos, framePoint.yPos,
                                         Ci.nsIDOMWindowUtils.SELECT_WORDNOSPACE)) {
      this._onFail("failed to set selection at point");
      return;
    }

    
    this._updateSelectionUI("start", true, true);
  },

  _onSelectionAttach: function _onSelectionAttach(aX, aY) {
    
    if (!this._initTargetInfo(aX, aY)) {
      this._onFail("failed to get frame offset");
      return;
    }

    
    this._updateSelectionUI("start", true, true);
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

    
    
    this._updateSelectionUI("update", aMarker == "end", aMarker == "start");
  },

  


  _onSelectionMoveStart: function _onSelectionMoveStart(aMsg) {
    if (!this._contentWindow) {
      this._onFail("_onSelectionMoveStart was called without proper view set up");
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
    if (!this._contentWindow) {
      this._onFail("_onSelectionMove was called without proper view set up");
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
    if (!this._contentWindow) {
      this._onFail("_onSelectionMove was called without proper view set up");
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

   






  _onCaretAttach: function _onCaretAttach(aX, aY) {
    
    if (!this._initTargetInfo(aX, aY)) {
      this._onFail("failed to get target information");
      return;
    }

    
    if (!this._targetIsEditable) {
      this._onFail("Coordiates didn't find a text input element.");
      return;
    }

    
    let selection = this._getSelection();
    if (!selection || !selection.isCollapsed) {
      this._onFail("No selection or selection is not collapsed.");
      return;
    }

    
    this._updateSelectionUI("caret", false, false, true);
  },

  






  _onSelectionCopy: function _onSelectionCopy(aMsg) {
    let tap = {
      xPos: aMsg.xPos,
      yPos: aMsg.yPos,
    };

    let tapInSelection = (tap.xPos > this._cache.selection.left &&
                          tap.xPos < this._cache.selection.right) &&
                         (tap.yPos > this._cache.selection.top &&
                          tap.yPos < this._cache.selection.bottom);
    
    
    
    
    let success = false;
    let selectedText = this._getSelectedText();
    if (tapInSelection && selectedText.length) {
      let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"]
                        .getService(Ci.nsIClipboardHelper);
      clipboard.copyString(selectedText, this._contentWindow.document);
      success = true;
    }
    sendSyncMessage("Content:SelectionCopied", { succeeded: success });
  },

  




  _onSelectionClose: function _onSelectionClose(aClearSelection) {
    if (aClearSelection) {
      this._clearSelection();
    }
    this.closeSelection();
  },

  



  _onSelectionUpdate: function _onSelectionUpdate() {
    if (!this._contentWindow) {
      this._onFail("_onSelectionUpdate was called without proper view set up");
      return;
    }

    
    this._updateSelectionUI("update", true, true);
  },

  



  _onFail: function _onFail(aDbgMessage) {
    if (aDbgMessage && aDbgMessage.length > 0)
      Util.dumpLn(aDbgMessage);
    this.sendAsync("Content:SelectionFail");
    this._clearSelection();
    this.closeSelection();
  },

  




  _repositionInfoRequest: function _repositionInfoRequest(aJsonMsg) {
    let result = this._calcNewContentPosition(aJsonMsg.viewHeight);

    
    if (result == 0) {
      this.sendAsync("Content:RepositionInfoResponse", { reposition: false });
      return;
    }

    this.sendAsync("Content:RepositionInfoResponse", {
      reposition: true,
      raiseContent: result,
    });
  },

  _onPing: function _onPing(aId) {
    this.sendAsync("Content:SelectionHandlerPong", { id: aId });
  },

  



  




  _clearSelection: function _clearSelection() {
    this._clearTimers();
    if (this._contentWindow) {
      let selection = this._getSelection();
      if (selection)
        selection.removeAllRanges();
    } else {
      let selection = content.getSelection();
      if (selection)
        selection.removeAllRanges();
    }
  },

  




  closeSelection: function closeSelection() {
    this._clearTimers();
    this._cache = null;
    this._contentWindow = null;
    this._targetElement = null;
    this._selectionMoveActive = false;
    this._contentOffset = null;
    this._domWinUtils = null;
    this._targetIsEditable = false;
    sendSyncMessage("Content:HandlerShutdown", {});
  },

  



  _initTargetInfo: function _initTargetInfo(aX, aY) {
    
    let { element: element,
          contentWindow: contentWindow,
          offset: offset,
          utils: utils } =
      this.getCurrentWindowAndOffset(aX, aY);
    if (!contentWindow) {
      return false;
    }
    this._targetElement = element;
    this._contentWindow = contentWindow;
    this._contentOffset = offset;
    this._domWinUtils = utils;
    this._targetIsEditable = Util.isEditable(this._targetElement);
    return true;
  },

  








  _calcNewContentPosition: function _calcNewContentPosition(aNewViewHeight) {
    
    
    if (!this._cache || !this._cache.element) {
      return Services.metro.keyboardHeight;
    }

    let position = Util.centerElementInView(aNewViewHeight, this._cache.element);
    if (position !== undefined) {
      return position;
    }

    
    
    let rect =
      this._domWinUtils.sendQueryContentEvent(this._domWinUtils.QUERY_CARET_RECT,
                                              this._targetElement.selectionEnd,
                                              0, 0, 0);
    if (!rect || !rect.succeeded) {
      Util.dumpLn("no caret was present, unexpected.");
      return 0;
    }

    
    
    
    let caretLocation = Math.max(Math.min(Math.round(rect.top + (rect.height * .5)),
                                          viewBottom), 0);

    
    if (caretLocation <= aNewViewHeight) {
      return 0;
    }

    
    return caretLocation - aNewViewHeight;
  },

  



  



  scrollTimerCallback: function scrollTimerCallback() {
    let result = SelectionHandler.updateTextEditSelection();
    
    if (result.trigger) {
      SelectionHandler._updateSelectionUI("update", result.start, result.end);
    }
  },

  receiveMessage: function sh_receiveMessage(aMessage) {
    if (this._debugEvents && aMessage.name != "Browser:SelectionMove") {
      Util.dumpLn("SelectionHandler:", aMessage.name);
    }
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Browser:SelectionStart":
        this._onSelectionStart(json);
        break;

      case "Browser:SelectionAttach":
        this._onSelectionAttach(json.xPos, json.yPos);
        break;

      case "Browser:CaretAttach":
        this._onCaretAttach(json.xPos, json.yPos);
        break;

      case "Browser:CaretMove":
        this._onCaretMove(json.caret.xPos, json.caret.yPos);
        break;

      case "Browser:CaretUpdate":
        this._onCaretPositionUpdate(json.caret.xPos, json.caret.yPos);
        break;

      case "Browser:SelectionSwitchMode":
        this._onSwitchMode(json.newMode, json.change, json.xPos, json.yPos);
        break;

      case "Browser:SelectionClose":
        this._onSelectionClose(json.clearSelection);
        break;

      case "Browser:SelectionMoveStart":
        this._onSelectionMoveStart(json);
        break;

      case "Browser:SelectionMove":
        this._onSelectionMove(json);
        break;

      case "Browser:SelectionMoveEnd":
        this._onSelectionMoveEnd(json);
        break;

      case "Browser:SelectionCopy":
        this._onSelectionCopy(json);
        break;

      case "Browser:SelectionDebug":
        this._onSelectionDebug(json);
        break;

      case "Browser:SelectionUpdate":
        this._onSelectionUpdate();
        break;

      case "Browser:RepositionInfoRequest":
        this._repositionInfoRequest(json);
        break;

      case "Browser:SelectionHandlerPing":
        this._onPing(json.id);
        break;
    }
  },

  



  





  getCurrentWindowAndOffset: function(x, y) {
    
    
    
    let utils = Util.getWindowUtils(content);
    let element = utils.elementFromPoint(x, y, true, false);
    let offset = { x:0, y:0 };

    while (element && (element instanceof HTMLIFrameElement ||
                       element instanceof HTMLFrameElement)) {
      
      let rect = element.getBoundingClientRect();

      
      

      
      scrollOffset = ContentScroll.getScrollOffset(element.contentDocument.defaultView);
      
      x -= rect.left + scrollOffset.x;
      y -= rect.top + scrollOffset.y;

      

      
      offset.x += rect.left;
      offset.y += rect.top;

      
      utils = element.contentDocument
                     .defaultView
                     .QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils);

      
      element = utils.elementFromPoint(x, y, true, false);
    }

    if (!element)
      return {};

    return {
      element: element,
      contentWindow: element.ownerDocument.defaultView,
      offset: offset,
      utils: utils
    };
  },

  _getDocShell: function _getDocShell(aWindow) {
    if (aWindow == null)
      return null;
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsIDocShell);
  },

  _getSelectedText: function _getSelectedText() {
    let selection = this._getSelection();
    if (selection)
      return selection.toString();
    return "";
  },

  _getSelection: function _getSelection() {
    if (this._targetElement instanceof Ci.nsIDOMNSEditableElement) {
      return this._targetElement
                 .QueryInterface(Ci.nsIDOMNSEditableElement)
                 .editor.selection;
    } else if (this._contentWindow)
      return this._contentWindow.getSelection();
    return null;
  },

  _getSelectController: function _getSelectController() {
    if (this._targetElement instanceof Ci.nsIDOMNSEditableElement) {
      return this._targetElement
                 .QueryInterface(Ci.nsIDOMNSEditableElement)
                 .editor.selectionController;
    } else {
      let docShell = this._getDocShell(this._contentWindow);
      if (docShell == null)
        return null;
      return docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsISelectionDisplay)
                     .QueryInterface(Ci.nsISelectionController);
    }
  },
};

SelectionHandler.__proto__ = new SelectionPrototype();
SelectionHandler.init();

