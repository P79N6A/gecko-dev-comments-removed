



dump("### SelectionHandler.js loaded\n");






























const kSelectionNodeAnchor = 1;
const kSelectionNodeFocus = 2;

var SelectionHandler = {
  _debugEvents: false,
  _cache: {},
  _targetElement: null,
  _targetIsEditable: false,
  _contentWindow: null,
  _contentOffset: { x:0, y:0 },
  _domWinUtils: null,
  _selectionMoveActive: false,
  _lastMarker: "",
  _debugOptions: { dumpRanges: false, displayRanges: false },
  _snap: true,

  init: function init() {
    addMessageListener("Browser:SelectionStart", this);
    addMessageListener("Browser:SelectionAttach", this);
    addMessageListener("Browser:SelectionEnd", this);
    addMessageListener("Browser:SelectionMoveStart", this);
    addMessageListener("Browser:SelectionMove", this);
    addMessageListener("Browser:SelectionMoveEnd", this);
    addMessageListener("Browser:SelectionUpdate", this);
    addMessageListener("Browser:SelectionClose", this);
    addMessageListener("Browser:SelectionClear", this);
    addMessageListener("Browser:SelectionCopy", this);
    addMessageListener("Browser:SelectionDebug", this);
    addMessageListener("Browser:CaretAttach", this);
    addMessageListener("Browser:CaretMove", this);
    addMessageListener("Browser:CaretUpdate", this);
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
    removeMessageListener("Browser:SelectionClear", this);
    removeMessageListener("Browser:SelectionCopy", this);
    removeMessageListener("Browser:SelectionDebug", this);
    removeMessageListener("Browser:CaretAttach", this);
    removeMessageListener("Browser:CaretMove", this);
    removeMessageListener("Browser:CaretUpdate", this);
  },

  



  




  get snap() {
    return this._snap;
  },

  



  


  _onSelectionStart: function _onSelectionStart(aX, aY) {
    
    if (!this._initTargetInfo(aX, aY)) {
      this._onFail("failed to get target information");
      return;
    }

    
    let selection = this._contentWindow.getSelection();
    selection.removeAllRanges();

    
    let framePoint = this._clientPointToFramePoint({ xPos: aX, yPos: aY });
    if (!this._domWinUtils.selectAtPoint(framePoint.xPos, framePoint.yPos,
                                         Ci.nsIDOMWindowUtils.SELECT_WORDNOSPACE)) {
      this._onFail("failed to set selection at point");
      return;
    }

    
    this._updateSelectionUI(true, true);
  },

  _onSelectionAttach: function _onSelectionAttach(aX, aY) {
    
    if (!this._initTargetInfo(aX, aY)) {
      this._onFail("failed to get frame offset");
      return;
    }

    
    this._updateSelectionUI(true, true);
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

    
    this._updateSelectionUI(true, true);
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

    
    this._updateSelectionUI(true, true);
  },

   






  _onCaretAttach: function _onCaretAttach(aX, aY) {
    
    if (!this._initTargetInfo(aX, aY)) {
      this._onFail("failed to get target information");
      return;
    }

    
    if (!this._targetIsEditable || !Util.isTextInput(this._targetElement)) {
      this._onFail("Unexpected, coordiates didn't find a text input element.");
      return;
    }

    
    let selection = this._getSelection();
    if (!selection || !selection.isCollapsed) {
      this._onFail("Unexpected, No selection or selection is not collapsed.");
      return;
    }

    
    this._updateSelectionUI(false, false, true);
  },

   






  _onCaretPositionUpdate: function _onCaretPositionUpdate(aX, aY) {
    this._onCaretMove(aX, aY);

    
    this._updateSelectionUI(false, false, true);
  },

   





  _onCaretMove: function _onCaretMove(aX, aY) {
    if (!this._targetIsEditable) {
      this._onFail("Unexpected, caret position isn't supported with non-inputs.");
      return;
    }
    let cp = this._contentWindow.document.caretPositionFromPoint(aX, aY);
    let input = cp.offsetNode;
    let offset = cp.offset;
    input.selectionStart = input.selectionEnd = offset;
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

  


  _onSelectionClose: function _onSelectionClose() {
    this._closeSelection();
  },

  




  _onSelectionClear: function _onSelectionClear(aClearFocus) {
    this._clearSelection();
    if (aClearFocus && this._targetElement) {
      this._targetElement.blur();
    }
  },

  



  _onSelectionUpdate: function _onSelectionUpdate() {
    if (!this._contentWindow) {
      this._onFail("_onSelectionUpdate was called without proper view set up");
      return;
    }

    
    this._updateSelectionUI(true, true);
  },

  



  _onFail: function _onFail(aDbgMessage) {
    if (aDbgMessage && aDbgMessage.length > 0)
      Util.dumpLn(aDbgMessage);
    sendAsyncMessage("Content:SelectionFail");
    this._clearSelection();
    this._closeSelection();
  },

  


  _onSelectionDebug: function _onSelectionDebug(aMsg) {
    this._debugOptions = aMsg;
    this._debugEvents = aMsg.dumpEvents;
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
    this.selectedText = "";
  },

  




  _closeSelection: function _closeSelection() {
    this._clearTimers();
    this._cache = null;
    this._contentWindow = null;
    this.selectedText = "";
    this._selectionMoveActive = false;
  },

  










  _updateSelectionUI: function _updateSelectionUI(aUpdateStart, aUpdateEnd,
                                                  aUpdateCaret) {
    let selection = this._getSelection();

    
    if (!selection) {
      this._onFail("no selection was present");
      return;
    }

    
    
    
    
    try {
      this._updateUIMarkerRects(selection);
    } catch (ex) {
      Util.dumpLn(ex.message);
      return;
    }

    this._cache.updateStart = aUpdateStart;
    this._cache.updateEnd = aUpdateEnd;
    this._cache.updateCaret = aUpdateCaret || false;
    this._cache.targetIsEditable = this._targetIsEditable;

    
    sendAsyncMessage("Content:SelectionRange", this._cache);
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
    this._targetIsEditable = false;
    if (this._isTextInput(this._targetElement)) {
      this._targetIsEditable = true;
      
      
      
      
      
      
      this._targetElement.focus();
    }
    return true;
  },

  






  _updateUIMarkerRects: function _updateUIMarkerRects(aSelection) {
    this._cache = this._extractClientRectFromRange(aSelection.getRangeAt(0));
    if (this. _debugOptions.dumpRanges)  {
       Util.dumpLn("start:", "(" + this._cache.start.xPos + "," +
                   this._cache.start.yPos + ")");
       Util.dumpLn("end:", "(" + this._cache.end.xPos + "," +
                   this._cache.end.yPos + ")");
       Util.dumpLn("caret:", "(" + this._cache.caret.xPos + "," +
                   this._cache.caret.yPos + ")");
    }
    this._restrictSelectionRectToEditBounds();
  },

  




  _restrictSelectionRectToEditBounds: function _restrictSelectionRectToEditBounds() {
    if (!this._targetIsEditable)
      return;
    let bounds = this._getTargetClientRect();
    if (this._cache.start.xPos < bounds.left)
      this._cache.start.xPos = bounds.left;
    if (this._cache.end.xPos < bounds.left)
      this._cache.end.xPos = bounds.left;
    if (this._cache.caret.xPos < bounds.left)
      this._cache.caret.xPos = bounds.left;
    if (this._cache.start.xPos > bounds.right)
      this._cache.start.xPos = bounds.right;
    if (this._cache.end.xPos > bounds.right)
      this._cache.end.xPos = bounds.right;
    if (this._cache.caret.xPos > bounds.right)
      this._cache.caret.xPos = bounds.right;

    if (this._cache.start.yPos < bounds.top)
      this._cache.start.yPos = bounds.top;
    if (this._cache.end.yPos < bounds.top)
      this._cache.end.yPos = bounds.top;
    if (this._cache.caret.yPos < bounds.top)
      this._cache.caret.yPos = bounds.top;
    if (this._cache.start.yPos > bounds.bottom)
      this._cache.start.yPos = bounds.bottom;
    if (this._cache.end.yPos > bounds.bottom)
      this._cache.end.yPos = bounds.bottom;
    if (this._cache.caret.yPos > bounds.bottom)
      this._cache.caret.yPos = bounds.bottom;
  },

  










  _handleSelectionPoint: function _handleSelectionPoint(aMarker, aClientPoint,
                                                        aEndOfSelection) {
    let selection = this._getSelection();

    let clientPoint = { xPos: aClientPoint.xPos, yPos: aClientPoint.yPos };

    if (selection.rangeCount == 0) {
      this._onFail("selection.rangeCount == 0");
      return;
    }

    
    if (selection.rangeCount > 1) {
      this._setContinuousSelection();
    }

    
    
    
    
    let halfLineHeight = this._queryHalfLineHeight(aMarker, selection);
    clientPoint.yPos -= halfLineHeight;

    if (this._targetIsEditable) {
      
      
      
      let result = this.updateTextEditSelection(clientPoint);

      
      
      
      
      
      clientPoint =
       this._constrainPointWithinControl(clientPoint, halfLineHeight);

      
      
      
      if (result.trigger) {
        
        
        
        
        if (!this._scrollTimer)
          this._scrollTimer = new Util.Timeout();
        this._setTextEditUpdateInterval(result.speed);

        
        this._setContinuousSelection();

        
        this._updateSelectionUI(result.start, result.end);

        return;
      }
    }

    this._lastMarker = aMarker;

    
    this._clearTimers();

    
    this._adjustSelection(aMarker, clientPoint, aEndOfSelection);

    
    
    
    if (aMarker == "start") {
      this._updateSelectionUI(false, true);
    } else {
      this._updateSelectionUI(true, false);
    }
  },

  



  










  _adjustSelection: function _adjustSelection(aMarker, aClientPoint,
                                              aEndOfSelection) {
    
    this._backupRangeList();

    
    let framePoint = this._clientPointToFramePoint(aClientPoint);

    
    
    
    let shrunk = this._shrinkSelectionFromPoint(aMarker, framePoint);

    let selectResult = false;
    try {
      
      let type = ((aEndOfSelection && this._snap) ?
        Ci.nsIDOMWindowUtils.SELECT_WORD :
        Ci.nsIDOMWindowUtils.SELECT_CHARACTER);

      
      selectResult = 
        this._domWinUtils.selectAtPoint(framePoint.xPos,
                                        framePoint.yPos,
                                        type);
    } catch (ex) {
    }

    
    
    if (!selectResult) {
      this._restoreRangeList();
    }

    this._freeRangeList();

    
    this._setContinuousSelection();
  },

  





  _backupRangeList: function _backupRangeList() {
    this._rangeBackup = new Array();
    for (let idx = 0; idx < this._getSelection().rangeCount; idx++) {
      this._rangeBackup.push(this._getSelection().getRangeAt(idx).cloneRange());
    }
  },

  _restoreRangeList: function _restoreRangeList() {
    if (this._rangeBackup == null)
      return;
    for (let idx = 0; idx < this._rangeBackup.length; idx++) {
      this._getSelection().addRange(this._rangeBackup[idx]);
    }
    this._freeRangeList();
  },

  _freeRangeList: function _restoreRangeList() {
    this._rangeBackup = null;
  },

  






  _constrainPointWithinControl: function _cpwc(aPoint, aHalfLineHeight) {
    let bounds = this._getTargetClientRect();
    let point = { xPos: aPoint.xPos, yPos: aPoint.yPos };
    if (point.xPos <= bounds.left)
      point.xPos = bounds.left + 2;
    if (point.xPos >= bounds.right)
      point.xPos = bounds.right - 2;
    if (point.yPos <= (bounds.top + aHalfLineHeight))
      point.yPos = (bounds.top + aHalfLineHeight);
    if (point.yPos >= (bounds.bottom - aHalfLineHeight))
      point.yPos = (bounds.bottom - aHalfLineHeight);
    return point;
  },

  






  _pointOrientationToRect: function _pointOrientationToRect(aPoint) {
    let bounds = this._targetElement.getBoundingClientRect();
    let result = { left: 0, right: 0, top: 0, bottom: 0 };
    if (aPoint.xPos <= bounds.left)
      result.left = bounds.left - aPoint.xPos;
    if (aPoint.xPos >= bounds.right)
      result.right = aPoint.xPos - bounds.right;
    if (aPoint.yPos <= bounds.top)
      result.top = bounds.top - aPoint.yPos;
    if (aPoint.yPos >= bounds.bottom)
      result.bottom = aPoint.yPos - bounds.bottom;
    return result;
  },

  












  updateTextEditSelection: function updateTextEditSelection(aClientPoint) {
    if (aClientPoint == undefined) {
      aClientPoint = this._rawSelectionPoint;
    }
    this._rawSelectionPoint = aClientPoint;

    let orientation = this._pointOrientationToRect(aClientPoint);
    let result = { speed: 1, trigger: false, start: false, end: false };

    if (orientation.left || orientation.top) {
      this._addEditSelection(kSelectionNodeAnchor);
      result.speed = orientation.left + orientation.top;
      result.trigger = true;
      result.end = true;
    } else if (orientation.right || orientation.bottom) {
      this._addEditSelection(kSelectionNodeFocus);
      result.speed = orientation.right + orientation.bottom;
      result.trigger = true;
      result.start = true;
    }

    
    
    if (result.speed > 100)
      result.speed = 100;
    if (result.speed < 1)
      result.speed = 1;
    result.speed /= 100;
    return result;
  },

  _setTextEditUpdateInterval: function _setTextEditUpdateInterval(aSpeedValue) {
    let timeout = (75 - (aSpeedValue * 75));
    this._scrollTimer.interval(timeout, this.scrollTimerCallback);
  },

  _clearTimers: function _clearTimers() {
    if (this._scrollTimer) {
      this._scrollTimer.clear();
    }
  },

  







  _addEditSelection: function _addEditSelection(aLocation) {
    let selCtrl = this._getSelectController();
    try {
      if (aLocation == kSelectionNodeAnchor) {
        this._targetElement.selectionStart = this._targetElement.selectionStart - 1;
        selCtrl.scrollSelectionIntoView(Ci.nsISelectionController.SELECTION_NORMAL,
                                        Ci.nsISelectionController.SELECTION_ANCHOR_REGION,
                                        Ci.nsISelectionController.SCROLL_SYNCHRONOUS);
      } else {
        this._targetElement.selectionEnd = this._targetElement.selectionEnd + 1;
        selCtrl.scrollSelectionIntoView(Ci.nsISelectionController.SELECTION_NORMAL,
                                        Ci.nsISelectionController.SELECTION_FOCUS_REGION,
                                        Ci.nsISelectionController.SCROLL_SYNCHRONOUS);
      }
    } catch (ex) {}
  },

  








  _queryHalfLineHeight: function _queryHalfLineHeight(aMarker, aSelection) {
    let rects = aSelection.getRangeAt(0).getClientRects();
    if (!rects.length) {
      return 0;
    }

    
    
    let height = 0;
    if (aMarker == "start") {
      
      height = rects[0].bottom - rects[0].top;
    } else {
      
      let len = rects.length - 1;
      height = rects[len].bottom - rects[len].top;
    }
    return height / 2;
  },

  





  _setContinuousSelection: function _setContinuousSelection() {
    let selection = this._getSelection();
    try {
      if (selection.rangeCount > 1) {
        let startRange = selection.getRangeAt(0);
        if (this. _debugOptions.displayRanges) {
          let clientRect = startRange.getBoundingClientRect();
          this._setDebugRect(clientRect, "red", false);
        }
        let newStartNode = null;
        let newStartOffset = 0;
        let newEndNode = null;
        let newEndOffset = 0;
        for (let idx = 1; idx < selection.rangeCount; idx++) {
          let range = selection.getRangeAt(idx);
          switch (startRange.compareBoundaryPoints(Ci.nsIDOMRange.START_TO_START, range)) {
            case -1: 
              newStartNode = startRange.startContainer;
              newStartOffset = startRange.startOffset;
              break;
            case 0: 
              newStartNode = startRange.startContainer;
              newStartOffset = startRange.startOffset;
              break;
            case 1: 
              newStartNode = range.startContainer;
              newStartOffset = range.startOffset;
              break;
          }
          switch (startRange.compareBoundaryPoints(Ci.nsIDOMRange.END_TO_END, range)) {
            case -1: 
              newEndNode = range.endContainer;
              newEndOffset = range.endOffset;
              break;
            case 0: 
              newEndNode = range.endContainer;
              newEndOffset = range.endOffset;
              break;
            case 1: 
              newEndNode = startRange.endContainer;
              newEndOffset = startRange.endOffset;
              break;
          }
          if (this. _debugOptions.displayRanges) {
            let clientRect = range.getBoundingClientRect();
            this._setDebugRect(clientRect, "orange", false);
          }
        }
        let range = content.document.createRange();
        range.setStart(newStartNode, newStartOffset);
        range.setEnd(newEndNode, newEndOffset);
        selection.addRange(range);
      }
    } catch (ex) {
      Util.dumpLn("exception while modifying selection:", ex.message);
      this._onFail("_handleSelectionPoint failed.");
      return false;
    }
    return true;
  },

  









  _shrinkSelectionFromPoint: function _shrinkSelectionFromPoint(aMarker, aFramePoint) {
    let result = false;
    try {
      let selection = this._getSelection();
      for (let range = 0; range < selection.rangeCount; range++) {
        
        let rects = selection.getRangeAt(range).getClientRects();
        for (let idx = 0; idx < rects.length; idx++) {
          
          
          if (Util.pointWithinDOMRect(aFramePoint.xPos, aFramePoint.yPos, rects[idx])) {
            result = true;
            if (aMarker == "start") {
              selection.collapseToEnd();
            } else {
              selection.collapseToStart();
            }
            
            
            
            
            
            let selCtrl = this._getSelectController();
            
            if (aMarker == "start") {
              
              selCtrl.characterMove(false, true);
              
              selection.collapseToStart();
              
              selCtrl.characterMove(true, true);
              
            } else {
              selCtrl.characterMove(true, true);
            }
            break;
          }
        }
      }
    } catch (ex) {
      Util.dumpLn("error shrinking selection:", ex.message);
    }
    return result;
  },

  



  



  scrollTimerCallback: function scrollTimerCallback() {
    let result = SelectionHandler.updateTextEditSelection();
    
    if (result.trigger) {
      if (result.start)
        SelectionHandler._updateSelectionUI(true, false);
      if (result.end)
        SelectionHandler._updateSelectionUI(false, true);
    }
  },

  receiveMessage: function sh_receiveMessage(aMessage) {
    if (this._debugEvents && aMessage.name != "Browser:SelectionMove") {
      Util.dumpLn("SelectionHandler:", aMessage.name);
    }
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Browser:SelectionStart":
        this._onSelectionStart(json.xPos, json.yPos);
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

      case "Browser:SelectionClose":
        this._onSelectionClose();
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

      case "Browser:SelectionClear":
        this._onSelectionClear(json.clearFocus);
        break;

      case "Browser:SelectionDebug":
        this._onSelectionDebug(json);
        break;

      case "Browser:SelectionUpdate":
        this._onSelectionUpdate();
        break;
    }
  },

  



  




  _extractClientRectFromRange: function _extractClientRectFromRange(aRange) {
    let cache = {
      start: {}, end: {}, caret: {},
      selection: { left: 0, top: 0, right: 0, bottom: 0 },
      element: { left: 0, top: 0, right: 0, bottom: 0 }
    };

    
    let rects = aRange.getClientRects();

    let startSet = false;
    for (let idx = 0; idx < rects.length; idx++) {
      if (this. _debugOptions.dumpRanges) Util.dumpDOMRect(idx, rects[idx]);
      if (!startSet && !Util.isEmptyDOMRect(rects[idx])) {
        cache.start.xPos = rects[idx].left + this._contentOffset.x;
        cache.start.yPos = rects[idx].bottom + this._contentOffset.y;
        cache.caret = cache.start;
        startSet = true;
        if (this. _debugOptions.dumpRanges) Util.dumpLn("start set");
      }
      if (!Util.isEmptyDOMRect(rects[idx])) {
        cache.end.xPos = rects[idx].right + this._contentOffset.x;
        cache.end.yPos = rects[idx].bottom + this._contentOffset.y;
        if (this. _debugOptions.dumpRanges) Util.dumpLn("end set");
      }
    }

    
    let r = aRange.getBoundingClientRect();
    cache.selection.left = r.left + this._contentOffset.x;
    cache.selection.top = r.top + this._contentOffset.y;
    cache.selection.right = r.right + this._contentOffset.x;
    cache.selection.bottom = r.bottom + this._contentOffset.y;

    
    r = this._getTargetClientRect();
    cache.element.left = r.left + this._contentOffset.x;
    cache.element.top = r.top + this._contentOffset.y;
    cache.element.right = r.right + this._contentOffset.x;
    cache.element.bottom = r.bottom + this._contentOffset.y;

    if (!rects.length) {
      Util.dumpLn("no rects in selection range. unexpected.");
    }

    return cache;
  },

  _getTargetClientRect: function _getTargetClientRect() {
    return this._targetElement.getBoundingClientRect();
  },

   


  _clientPointToFramePoint: function _clientPointToFramePoint(aClientPoint) {
    let point = {
      xPos: aClientPoint.xPos - this._contentOffset.x,
      yPos: aClientPoint.yPos - this._contentOffset.y
    };
    return point;
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

  _isTextInput: function _isTextInput(aElement) {
    return ((aElement instanceof Ci.nsIDOMHTMLInputElement &&
             aElement.mozIsTextField(false)) ||
            aElement instanceof Ci.nsIDOMHTMLTextAreaElement);
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

  



  _debugDumpSelection: function _debugDumpSelection(aNote, aSel) {
    Util.dumpLn("--" + aNote + "--");
    Util.dumpLn("anchor:", aSel.anchorNode, aSel.anchorOffset);
    Util.dumpLn("focus:", aSel.focusNode, aSel.focusOffset);
  },

  _debugDumpChildNodes: function _dumpChildNodes(aNode, aSpacing) {
    for (let idx = 0; idx < aNode.childNodes.length; idx++) {
      let node = aNode.childNodes.item(idx);
      for (let spaceIdx = 0; spaceIdx < aSpacing; spaceIdx++) dump(" ");
      Util.dumpLn("[" + idx + "]", node);
      this._debugDumpChildNodes(node, aSpacing + 1);
    }
  },

  _setDebugElementRect: function _setDebugElementRect(e, aScrollOffset, aColor) {
    try {
      if (e == null) {
        Util.dumpLn("SelectionHandler _setDebugElementRect(): passed in null element");
        return;
      }
      if (e.offsetWidth == 0 || e.offsetHeight== 0) {
        Util.dumpLn("SelectionHandler _setDebugElementRect(): passed in flat rect");
        return;
      }
      
      sendAsyncMessage("Content:SelectionDebugRect",
        { left:e.offsetLeft - aScrollOffset.x,
          top:e.offsetTop - aScrollOffset.y,
          right:e.offsetLeft + e.offsetWidth - aScrollOffset.x,
          bottom:e.offsetTop + e.offsetHeight - aScrollOffset.y,
          color:aColor, id: e.id });
    } catch(ex) {
      Util.dumpLn("SelectionHandler _setDebugElementRect():", ex.message);
    }
  },

  











  _setDebugRect: function _setDebugRect(aRect, aColor, aFill, aId) {
    sendAsyncMessage("Content:SelectionDebugRect",
      { left:aRect.left, top:aRect.top,
        right:aRect.right, bottom:aRect.bottom,
        color:aColor, fill: aFill, id: aId });
  },

  






  _setDebugPoint: function _setDebugPoint(aX, aY, aColor) {
    let rect = { left: aX - 2, top: aY - 2,
                 right: aX + 2, bottom: aY + 2 };
    this._setDebugRect(rect, aColor, true);
  },
};

SelectionHandler.init();