








const kSelectionNodeAnchor = 1;
const kSelectionNodeFocus = 2;


const kChromeSelector = 1;
const kContentSelector = 2;

dump("### SelectionPrototype.js loaded\n");































var SelectionPrototype = function() { }

SelectionPrototype.prototype = {
  _debugEvents: false,
  _cache: {},
  _targetElement: null,
  _targetIsEditable: true,
  _contentOffset: { x: 0, y: 0 },
  _contentWindow: null,
  _debugOptions: { dumpRanges: false, displayRanges: false },
  _domWinUtils: null,
  _selectionMoveActive: false,
  _snap: false,
  _type: 0,

  



  get isActive() {
    return !!this._targetElement;
  },

  get targetIsEditable() {
    return this._targetIsEditable || false;
  },

  




  get snap() {
    return this._snap;
  },

  set snap(aValue) {
    this._snap = aValue;
  },

  



  set type(aValue) {
    this._type = aValue;
  },

  get type() {
    return this._type;
  },

  



  






  sendAsync: function sendAsync(aMsg, aJson) {
    Util.dumpLn("Base sendAsync called on SelectionPrototype. This is a no-op.");
  },

  



   






  _onCaretPositionUpdate: function _onCaretPositionUpdate(aX, aY) {
    this._onCaretMove(aX, aY);

    
    this._updateSelectionUI("caret", false, false, true);
  },

   





  _onCaretMove: function _onCaretMove(aX, aY) {
    if (!this._targetIsEditable) {
      this._onFail("Unexpected, caret position isn't supported with non-inputs.");
      return;
    }

    
    
    
    
    let containedCoords = this._restrictCoordinateToEditBounds(aX, aY);
    let cp = this._contentWindow.document.caretPositionFromPoint(containedCoords.xPos,
                                                                 containedCoords.yPos);
    let input = cp.offsetNode;
    let offset = cp.offset;
    input.selectionStart = input.selectionEnd = offset;
  },

  


  _onSelectionDebug: function _onSelectionDebug(aMsg) {
    this._debugOptions = aMsg;
    this._debugEvents = aMsg.dumpEvents;
  },

  



  











  _updateSelectionUI: function _updateSelectionUI(aSrcMsg, aUpdateStart, aUpdateEnd,
                                                  aUpdateCaret) {
    let selection = this._getSelection();

    
    if (!selection) {
      this._onFail("no selection was present");
      return;
    }

    
    
    
    
    try {
      this._updateUIMarkerRects(selection);
    } catch (ex) {
      Util.dumpLn("_updateUIMarkerRects:", ex.message);
      return;
    }

    this._cache.src = aSrcMsg;
    this._cache.updateStart = aUpdateStart;
    this._cache.updateEnd = aUpdateEnd;
    this._cache.updateCaret = aUpdateCaret || false;
    this._cache.targetIsEditable = this._targetIsEditable;

    
    this.sendAsync("Content:SelectionRange", this._cache);
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

    
    if (this._targetIsEditable && !Util.isEditableContent(this._targetElement)) {
      this._adjustEditableSelection(aMarker, clientPoint, aEndOfSelection);
    } else {
      this._adjustSelectionAtPoint(aMarker, clientPoint, aEndOfSelection);
    }
  },

  



  











  _adjustEditableSelection: function _adjustEditableSelection(aMarker,
                                                              aAdjustedClientPoint,
                                                              aEndOfSelection) {
    
    
    
    let result = this.updateTextEditSelection(aAdjustedClientPoint);

    
    
    if (result.trigger) {
      
      
      
      
      this._setTextEditUpdateInterval(result.speed);

      
      this._setContinuousSelection();

      
      this._updateSelectionUI("update", result.start, result.end);
    } else {
      
      this._clearTimers();

      
      let constrainedPoint =
        this._constrainPointWithinControl(aAdjustedClientPoint);

      
      
      if (Util.isMultilineInput(this._targetElement)) {
        this._adjustSelectionAtPoint(aMarker, constrainedPoint, aEndOfSelection);
        return;
      }

      
      let cp =
        this._contentWindow.document.caretPositionFromPoint(constrainedPoint.xPos,
                                                            constrainedPoint.yPos);
      if (!cp || !this._offsetNodeIsValid(cp.offsetNode)) {
        return;
      }
      if (aMarker == "start") {
        this._targetElement.selectionStart = cp.offset;
      } else {
        this._targetElement.selectionEnd = cp.offset;
      }
    }
  },

  




  _offsetNodeIsValid: function (aNode) {
    if (aNode == this._targetElement ||
        this._contentWindow.document.getBindingParent(aNode) == this._targetElement) {
      return true;
    }
    return false;
  },

  














  _adjustSelectionAtPoint: function _adjustSelectionAtPoint(aMarker, aClientPoint,
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

    
    
    
    this._updateSelectionUI("update", aMarker == "end", aMarker == "start");
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

  







  _constrainPointWithinControl: function _cpwc(aPoint, aMargin, aOffset) {
    let margin = aMargin || 0;
    let offset = aOffset || 0;
    let bounds = this._getTargetBrowserRect();
    let point = { xPos: aPoint.xPos, yPos: aPoint.yPos };
    if (point.xPos <= (bounds.left + margin))
      point.xPos = bounds.left + offset;
    if (point.xPos >= (bounds.right - margin))
      point.xPos = bounds.right - offset;
    if (point.yPos <= (bounds.top + margin))
      point.yPos = bounds.top + offset;
    if (point.yPos >= (bounds.bottom - margin))
      point.yPos = bounds.bottom - offset;
    return point;
  },

  






  _pointOrientationToRect: function _pointOrientationToRect(aPoint) {
    let bounds = this._getTargetBrowserRect();
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
    let ml = Util.isMultilineInput(this._targetElement);

    
    
    if (orientation.left || (ml && orientation.top)) {
      this._addEditSelection(kSelectionNodeAnchor);
      result.speed = orientation.left + orientation.top;
      result.trigger = true;
      result.end = true;
    } else if (orientation.right || (ml && orientation.bottom)) {
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
    if (!this._scrollTimer)
      this._scrollTimer = new Util.Timeout();
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
        let start = Math.max(this._targetElement.selectionStart - 1, 0);
        this._targetElement.setSelectionRange(start, this._targetElement.selectionEnd,
                                              "backward");
      } else {
        let end = Math.min(this._targetElement.selectionEnd + 1,
                           this._targetElement.textLength);
        this._targetElement.setSelectionRange(this._targetElement.selectionStart,
                                              end,
                                              "forward");
      }
      selCtrl.scrollSelectionIntoView(Ci.nsISelectionController.SELECTION_NORMAL,
                                      Ci.nsISelectionController.SELECTION_FOCUS_REGION,
                                      Ci.nsISelectionController.SCROLL_SYNCHRONOUS);
    } catch (ex) { Util.dumpLn(ex);}
  },

  _updateInputFocus: function _updateInputFocus(aMarker) {
    try {
      let selCtrl = this._getSelectController();
      this._targetElement.setSelectionRange(this._targetElement.selectionStart,
                                            this._targetElement.selectionEnd,
                                            aMarker == "start" ?
                                              "backward" : "forward");
      selCtrl.scrollSelectionIntoView(Ci.nsISelectionController.SELECTION_NORMAL,
                                      Ci.nsISelectionController.SELECTION_FOCUS_REGION,
                                      Ci.nsISelectionController.SCROLL_SYNCHRONOUS);
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

  






  _updateUIMarkerRects: function _updateUIMarkerRects(aSelection) {
    this._cache = this._extractUIRects(aSelection.getRangeAt(0));
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

  





  _extractUIRects: function _extractUIRects(aRange) {
    let seldata = {
      start: {}, end: {}, caret: {},
      selection: { left: 0, top: 0, right: 0, bottom: 0 },
      element: { left: 0, top: 0, right: 0, bottom: 0 }
    };

    
    let rects = aRange.getClientRects();

    if (rects && rects.length) {
      let startSet = false;
      for (let idx = 0; idx < rects.length; idx++) {
        if (this. _debugOptions.dumpRanges) Util.dumpDOMRect(idx, rects[idx]);
        if (!startSet && !Util.isEmptyDOMRect(rects[idx])) {
          seldata.start.xPos = rects[idx].left + this._contentOffset.x;
          seldata.start.yPos = rects[idx].bottom + this._contentOffset.y;
          seldata.caret = seldata.start;
          startSet = true;
          if (this. _debugOptions.dumpRanges) Util.dumpLn("start set");
        }
        if (!Util.isEmptyDOMRect(rects[idx])) {
          seldata.end.xPos = rects[idx].right + this._contentOffset.x;
          seldata.end.yPos = rects[idx].bottom + this._contentOffset.y;
          if (this. _debugOptions.dumpRanges) Util.dumpLn("end set");
        }
      }

      
      let r = aRange.getBoundingClientRect();
      seldata.selection.left = r.left + this._contentOffset.x;
      seldata.selection.top = r.top + this._contentOffset.y;
      seldata.selection.right = r.right + this._contentOffset.x;
      seldata.selection.bottom = r.bottom + this._contentOffset.y;
    }

    
    r = this._getTargetClientRect();
    seldata.element.left = r.left + this._contentOffset.x;
    seldata.element.top = r.top + this._contentOffset.y;
    seldata.element.right = r.right + this._contentOffset.x;
    seldata.element.bottom = r.bottom + this._contentOffset.y;

    
    seldata.selectionRangeFound = !!rects.length;

    return seldata;
  },

  




  _restrictSelectionRectToEditBounds: function _restrictSelectionRectToEditBounds() {
    if (!this._targetIsEditable)
      return;

    let bounds = this._getTargetBrowserRect();
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

  _restrictCoordinateToEditBounds: function _restrictCoordinateToEditBounds(aX, aY) {
    let result = {
      xPos: aX,
      yPos: aY
    };
    if (!this._targetIsEditable)
      return result;
    let bounds = this._getTargetBrowserRect();
    if (aX <= bounds.left)
      result.xPos = bounds.left + 1;
    if (aX >= bounds.right)
      result.xPos = bounds.right - 1;
    if (aY <= bounds.top)
      result.yPos = bounds.top + 1;
    if (aY >= bounds.bottom)
      result.yPos = bounds.bottom - 1;
    return result;
  },

  



  



  _getTargetClientRect: function _getTargetClientRect() {
    return this._targetElement.getBoundingClientRect();
  },

  


  _getTargetBrowserRect: function _getTargetBrowserRect() {
    let client = this._getTargetClientRect();
    return {
      left: client.left +  this._contentOffset.x,
      top: client.top +  this._contentOffset.y,
      right: client.right +  this._contentOffset.x,
      bottom: client.bottom +  this._contentOffset.y
    };
  },

  _clientPointToFramePoint: function _clientPointToFramePoint(aClientPoint) {
    let point = {
      xPos: aClientPoint.xPos - this._contentOffset.x,
      yPos: aClientPoint.yPos - this._contentOffset.y
    };
    return point;
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
        Util.dumpLn("_setDebugElementRect(): passed in null element");
        return;
      }
      if (e.offsetWidth == 0 || e.offsetHeight== 0) {
        Util.dumpLn("_setDebugElementRect(): passed in flat rect");
        return;
      }
      
      this.sendAsync("Content:SelectionDebugRect",
        { left:e.offsetLeft - aScrollOffset.x,
          top:e.offsetTop - aScrollOffset.y,
          right:e.offsetLeft + e.offsetWidth - aScrollOffset.x,
          bottom:e.offsetTop + e.offsetHeight - aScrollOffset.y,
          color:aColor, id: e.id });
    } catch(ex) {
      Util.dumpLn("_setDebugElementRect():", ex.message);
    }
  },

  











  _setDebugRect: function _setDebugRect(aRect, aColor, aFill, aId) {
    this.sendAsync("Content:SelectionDebugRect",
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
