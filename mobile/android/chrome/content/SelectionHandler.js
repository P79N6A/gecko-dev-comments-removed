


"use strict";

var SelectionHandler = {
  HANDLE_TYPE_START: "START",
  HANDLE_TYPE_MIDDLE: "MIDDLE",
  HANDLE_TYPE_END: "END",

  TYPE_NONE: 0,
  TYPE_CURSOR: 1,
  TYPE_SELECTION: 2,

  
  
  _cache: null,
  _activeType: 0, 
  _ignoreSelectionChanges: false, 

  
  get _contentWindow() {
    if (this._contentWindowRef)
      return this._contentWindowRef.get();
    return null;
  },

  set _contentWindow(aContentWindow) {
    this._contentWindowRef = Cu.getWeakReference(aContentWindow);
  },

  get _targetElement() {
    if (this._targetElementRef)
      return this._targetElementRef.get();
    return null;
  },

  set _targetElement(aTargetElement) {
    this._targetElementRef = Cu.getWeakReference(aTargetElement);
  },

  get _domWinUtils() {
    return BrowserApp.selectedBrowser.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                                                    getInterface(Ci.nsIDOMWindowUtils);
  },

  _isRTL: false,

  _addObservers: function sh_addObservers() {
    Services.obs.addObserver(this, "Gesture:SingleTap", false);
    Services.obs.addObserver(this, "Window:Resize", false);
    Services.obs.addObserver(this, "Tab:Selected", false);
    Services.obs.addObserver(this, "after-viewport-change", false);
    Services.obs.addObserver(this, "TextSelection:Move", false);
    Services.obs.addObserver(this, "TextSelection:Position", false);
    BrowserApp.deck.addEventListener("compositionend", this, false);
  },

  _removeObservers: function sh_removeObservers() {
    Services.obs.removeObserver(this, "Gesture:SingleTap");
    Services.obs.removeObserver(this, "Window:Resize");
    Services.obs.removeObserver(this, "Tab:Selected");
    Services.obs.removeObserver(this, "after-viewport-change");
    Services.obs.removeObserver(this, "TextSelection:Move");
    Services.obs.removeObserver(this, "TextSelection:Position");
    BrowserApp.deck.removeEventListener("compositionend", this);
  },

  observe: function sh_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "Gesture:SingleTap": {
        if (this._activeType == this.TYPE_SELECTION) {
          let data = JSON.parse(aData);
          if (this._pointInSelection(data.x, data.y))
            this.copySelection();
          else
            this._closeSelection();
        } else if (this._activeType == this.TYPE_CURSOR) {
          
          
          this._deactivate();
        }
        break;
      }
      case "Tab:Selected":
        this._closeSelection();
        break;

      case "Window:Resize": {
        if (this._activeType == this.TYPE_SELECTION) {
          
          
          this._closeSelection();
        }
        break;
      }
      case "after-viewport-change": {
        if (this._activeType == this.TYPE_SELECTION) {
          
          this._updateCacheForSelection();
        }
        break;
      }
      case "TextSelection:Move": {
        let data = JSON.parse(aData);
        if (this._activeType == this.TYPE_SELECTION) {
          
          this._ignoreSelectionChanges = true;
          this._moveSelection(data.handleType == this.HANDLE_TYPE_START, data.x, data.y);
        } else if (this._activeType == this.TYPE_CURSOR) {
          
          this._sendMouseEvents(data.x, data.y);

          
          this._positionHandles();
        }
        break;
      }
      case "TextSelection:Position": {
        if (this._activeType == this.TYPE_SELECTION) {
          
          this._ignoreSelectionChanges = true;
          
          let isStartHandle = JSON.parse(aData).handleType == this.HANDLE_TYPE_START;
          let selectionReversed = this._updateCacheForSelection(isStartHandle);
          if (selectionReversed) {
            
            let selection = this._getSelection();
            let anchorNode = selection.anchorNode;
            let anchorOffset = selection.anchorOffset;
            selection.collapse(selection.focusNode, selection.focusOffset);
            selection.extend(anchorNode, anchorOffset);
          }
          
          this._ignoreSelectionChanges = false;
        }
        this._positionHandles();
        break;
      }

      case "TextSelection:Get":
        sendMessageToJava({
          type: "TextSelection:Data",
          requestId: aData,
          text: this._getSelectedText()
        });
        break;
    }
  },

  handleEvent: function sh_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "pagehide":
      
      case "keydown":
      case "blur":
        this._closeSelection();
        break;

      case "compositionend":
        if (this._activeType == this.TYPE_CURSOR) {
          this._deactivate();
        }
        break;
    }
  },

  
  canSelect: function sh_canSelect(aElement) {
    return !(aElement instanceof Ci.nsIDOMHTMLButtonElement ||
             aElement instanceof Ci.nsIDOMHTMLEmbedElement ||
             aElement instanceof Ci.nsIDOMHTMLImageElement ||
             aElement instanceof Ci.nsIDOMHTMLMediaElement) &&
             aElement.style.MozUserSelect != 'none';
  },

  _getScrollPos: function sh_getScrollPos() {
    
    let scrollX = {}, scrollY = {};
    this._contentWindow.top.QueryInterface(Ci.nsIInterfaceRequestor).
                            getInterface(Ci.nsIDOMWindowUtils).getScrollXY(false, scrollX, scrollY);
    return {
      X: scrollX.value,
      Y: scrollY.value
    };
  },

  notifySelectionChanged: function sh_notifySelectionChanged(aDocument, aSelection, aReason) {
    
    if (this._ignoreSelectionChanges) {
      return;
    }

    
    if ((aReason & Ci.nsISelectionListener.COLLAPSETOSTART_REASON) ||
        (aReason & Ci.nsISelectionListener.COLLAPSETOEND_REASON)) {
      this._closeSelection();
      return;
    }

    
    if (!aSelection.toString()) {
      this._closeSelection();
    }
  },

  






  startSelection: function sh_startSelection(aElement, aX, aY) {
    
    this._closeSelection();

    this._initTargetInfo(aElement);

    
    this._contentWindow.getSelection().removeAllRanges();

    if (!this._domWinUtils.selectAtPoint(aX, aY, Ci.nsIDOMWindowUtils.SELECT_WORDNOSPACE)) {
      this._deactivate();
      return;
    }

    let selection = this._getSelection();
    
    if (!selection || selection.rangeCount == 0) {
      this._deactivate();
      return;
    }

    
    selection.QueryInterface(Ci.nsISelectionPrivate).addSelectionListener(this);
    this._activeType = this.TYPE_SELECTION;

    
    this._cache = { start: {}, end: {}};
    this._updateCacheForSelection();

    let scroll = this._getScrollPos();
    
    let positions = this._getHandlePositions(scroll);
    let clickX = scroll.X + aX;
    let clickY = scroll.Y + aY;
    let distance = 0;

    
    if (positions[0].left < clickX && clickX < positions[1].left
        && positions[0].top < clickY && clickY < positions[1].top) {
      distance = 0;
    } else {
      
      let selectposX = (positions[0].left + positions[1].left) / 2;
      let selectposY = (positions[0].top + positions[1].top) / 2;

      let dx = Math.abs(selectposX - clickX);
      let dy = Math.abs(selectposY - clickY);
      distance = dx + dy;
    }

    let maxSelectionDistance = Services.prefs.getIntPref("browser.ui.selection.distance");
    
    if (distance > maxSelectionDistance) {
      this._closeSelection();
      return;
    }

    this._positionHandles(positions);

    sendMessageToJava({
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_START, this.HANDLE_TYPE_END]
    });
  },

  





  attachCaret: function sh_attachCaret(aElement) {
    this._initTargetInfo(aElement);

    this._contentWindow.addEventListener("keydown", this, false);
    this._contentWindow.addEventListener("blur", this, true);

    this._activeType = this.TYPE_CURSOR;
    this._positionHandles();

    sendMessageToJava({
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_MIDDLE]
    });
  },

  _initTargetInfo: function sh_initTargetInfo(aElement) {
    this._targetElement = aElement;
    if (aElement instanceof Ci.nsIDOMNSEditableElement) {
      aElement.focus();
    }

    this._contentWindow = aElement.ownerDocument.defaultView;
    this._isRTL = (this._contentWindow.getComputedStyle(aElement, "").direction == "rtl");

    this._addObservers();
    this._contentWindow.addEventListener("pagehide", this, false);
  },

  _getSelection: function sh_getSelection() {
    if (this._targetElement instanceof Ci.nsIDOMNSEditableElement)
      return this._targetElement.QueryInterface(Ci.nsIDOMNSEditableElement).editor.selection;
    else
      return this._contentWindow.getSelection();
  },

  _getSelectedText: function sh_getSelectedText() {
    if (!this._contentWindow)
      return "";

    let selection = this._getSelection();
    if (selection)
      return selection.toString().trim();
    return "";
  },

  _getSelectionController: function sh_getSelectionController() {
    if (this._targetElement instanceof Ci.nsIDOMNSEditableElement)
      return this._targetElement.QueryInterface(Ci.nsIDOMNSEditableElement).editor.selectionController;
    else
      return this._contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                                 getInterface(Ci.nsIWebNavigation).
                                 QueryInterface(Ci.nsIInterfaceRequestor).
                                 getInterface(Ci.nsISelectionDisplay).
                                 QueryInterface(Ci.nsISelectionController);
  },

  
  shouldShowContextMenu: function sh_shouldShowContextMenu(aX, aY) {
    return (this._activeType == this.TYPE_SELECTION) && this._pointInSelection(aX, aY);
  },

  selectAll: function sh_selectAll(aElement, aX, aY) {
    if (this._activeType != this.TYPE_SELECTION)
      this.startSelection(aElement, aX, aY);

    let selectionController = this._getSelectionController();
    selectionController.selectAll();
    this._updateCacheForSelection();
    this._positionHandles();
  },

  





  _moveSelection: function sh_moveSelection(aIsStartHandle, aX, aY) {
    
    
    let viewOffset = this._getViewOffset();
    let caretPos = this._contentWindow.document.caretPositionFromPoint(aX - viewOffset.x, aY - viewOffset.y);

    
    let targetIsEditable = this._targetElement instanceof Ci.nsIDOMNSEditableElement;
    if (targetIsEditable && (caretPos.offsetNode != this._targetElement)) {
      return;
    }

    
    if (aIsStartHandle) {
      this._cache.start.x = aX;
      this._cache.start.y = aY;
    } else {
      this._cache.end.x = aX;
      this._cache.end.y = aY;
    }

    let selection = this._getSelection();

    
    
    if ((aIsStartHandle && !this._isRTL) || (!aIsStartHandle && this._isRTL)) {
      if (targetIsEditable) {
        
        this._targetElement.selectionStart = caretPos.offset;
      } else {
        let focusNode = selection.focusNode;
        let focusOffset = selection.focusOffset;
        selection.collapse(caretPos.offsetNode, caretPos.offset);
        selection.extend(focusNode, focusOffset);
      }
    } else {
      if (targetIsEditable) {
        
        this._targetElement.selectionEnd = caretPos.offset;
      } else {
        selection.extend(caretPos.offsetNode, caretPos.offset);
      }
    }
  },

  _sendMouseEvents: function sh_sendMouseEvents(aX, aY, useShift) {
    
    
    if (this._activeType == this.TYPE_CURSOR) {
      
      let range = document.createRange();
      range.selectNodeContents(this._targetElement.QueryInterface(Ci.nsIDOMNSEditableElement).editor.rootElement);
      let textBounds = range.getBoundingClientRect();

      
      let editorBounds = this._domWinUtils.sendQueryContentEvent(this._domWinUtils.QUERY_EDITOR_RECT, 0, 0, 0, 0);
      
      
      let editorRect = new Rect(editorBounds.left / window.devicePixelRatio,
                                editorBounds.top / window.devicePixelRatio,
                                editorBounds.width / window.devicePixelRatio,
                                editorBounds.height / window.devicePixelRatio);

      
      let rect = new Rect(textBounds.left, textBounds.top, textBounds.width, textBounds.height);
      rect.restrictTo(editorRect);

      
      
      
      
      if (aY < rect.y + 1) {
        aY = rect.y + 1;
        this._getSelectionController().scrollLine(false);
      } else if (aY > rect.y + rect.height - 1) {
        aY = rect.y + rect.height - 1;
        this._getSelectionController().scrollLine(true);
      }

      
      if (aX < rect.x) {
        aX = rect.x;
        this._getSelectionController().scrollCharacter(false);
      } else if (aX > rect.x + rect.width) {
        aX = rect.x + rect.width;
        this._getSelectionController().scrollCharacter(true);
      }
    } else if (this._activeType == this.TYPE_SELECTION) {
      
      aY -= 1;
    }

    this._domWinUtils.sendMouseEventToWindow("mousedown", aX, aY, 0, 0, useShift ? Ci.nsIDOMNSEvent.SHIFT_MASK : 0, true);
    this._domWinUtils.sendMouseEventToWindow("mouseup", aX, aY, 0, 0, useShift ? Ci.nsIDOMNSEvent.SHIFT_MASK : 0, true);
  },

  copySelection: function sh_copySelection() {
    let selectedText = this._getSelectedText();
    if (selectedText.length) {
      let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);
      clipboard.copyString(selectedText, this._contentWindow.document);
      NativeWindow.toast.show(Strings.browser.GetStringFromName("selectionHelper.textCopied"), "short");
    }
    this._closeSelection();
  },

  shareSelection: function sh_shareSelection() {
    let selectedText = this._getSelectedText();
    if (selectedText.length) {
      sendMessageToJava({
        type: "Share:Text",
        text: selectedText
      });
    }
    this._closeSelection();
  },

  searchSelection: function sh_searchSelection() {
    let selectedText = this._getSelectedText();
    if (selectedText.length) {
      let req = Services.search.defaultEngine.getSubmission(selectedText);
      let parent = BrowserApp.selectedTab;
      let isPrivate = PrivateBrowsingUtils.isWindowPrivate(parent.browser.contentWindow);
      
      BrowserApp.addTab(req.uri.spec, {parentId: parent.id,
                                       selected: true,
                                       isPrivate: isPrivate});
    }
    this._closeSelection();
  },

  


  _closeSelection: function sh_closeSelection() {
    
    if (this._activeType == this.TYPE_NONE)
      return;

    if (this._activeType == this.TYPE_SELECTION)
      this._clearSelection();

    this._deactivate();
  },

  _clearSelection: function sh_clearSelection() {
    let selection = this._getSelection();
    if (selection) {
      
      selection.QueryInterface(Ci.nsISelectionPrivate).removeSelectionListener(this);
      
      if (selection.rangeCount != 0) {
        selection.collapseToStart();
      }
    }
  },

  _deactivate: function sh_deactivate() {
    this._activeType = this.TYPE_NONE;

    sendMessageToJava({ type: "TextSelection:HideHandles" });

    this._removeObservers();
    this._contentWindow.removeEventListener("pagehide", this, false);
    this._contentWindow.removeEventListener("keydown", this, false);
    this._contentWindow.removeEventListener("blur", this, true);
    this._contentWindow = null;
    this._targetElement = null;
    this._isRTL = false;
    this._cache = null;
    this._ignoreSelectionChanges = false;
  },

  _getViewOffset: function sh_getViewOffset() {
    let offset = { x: 0, y: 0 };
    let win = this._contentWindow;

    
    while (win.frameElement) {
      let rect = win.frameElement.getBoundingClientRect();
      offset.x += rect.left;
      offset.y += rect.top;

      win = win.parent;
    }

    return offset;
  },

  _pointInSelection: function sh_pointInSelection(aX, aY) {
    let offset = this._getViewOffset();
    let rangeRect = this._getSelection().getRangeAt(0).getBoundingClientRect();
    let radius = ElementTouchHelper.getTouchRadius();
    return (aX - offset.x > rangeRect.left - radius.left &&
            aX - offset.x < rangeRect.right + radius.right &&
            aY - offset.y > rangeRect.top - radius.top &&
            aY - offset.y < rangeRect.bottom + radius.bottom);
  },

  
  
  _updateCacheForSelection: function sh_updateCacheForSelection(aIsStartHandle) {
    let selection = this._getSelection();
    let rects = selection.getRangeAt(0).getClientRects();
    let start = { x: this._isRTL ? rects[0].right : rects[0].left, y: rects[0].bottom };
    let end = { x: this._isRTL ? rects[rects.length - 1].left : rects[rects.length - 1].right, y: rects[rects.length - 1].bottom };

    let selectionReversed = false;
    if (this._cache.start) {
      
      selectionReversed = (aIsStartHandle && (end.y > this._cache.end.y || (end.y == this._cache.end.y && end.x > this._cache.end.x))) ||
                          (!aIsStartHandle && (start.y < this._cache.start.y || (start.y == this._cache.start.y && start.x < this._cache.start.x)));
    }

    this._cache.start = start;
    this._cache.end = end;

    return selectionReversed;
  },

  _getHandlePositions: function sh_getHandlePositions(scroll) {
    
    
    
    
    let checkHidden = function(x, y) {
      return false;
    };
    if (this._contentWindow.frameElement) {
      let bounds = this._contentWindow.frameElement.getBoundingClientRect();
      checkHidden = function(x, y) {
        return x < 0 || y < 0 || x > bounds.width || y > bounds.height;
      };
    }

    let positions = null;
    if (this._activeType == this.TYPE_CURSOR) {
      
      
      let cursor = this._domWinUtils.sendQueryContentEvent(this._domWinUtils.QUERY_CARET_RECT, this._targetElement.selectionEnd, 0, 0, 0);
      
      
      let x = cursor.left / window.devicePixelRatio;
      let y = (cursor.top + cursor.height) / window.devicePixelRatio;
      return [{ handle: this.HANDLE_TYPE_MIDDLE,
                left: x + scroll.X,
                top: y + scroll.Y,
                hidden: checkHidden(x, y) }];
    } else {
      let sx = this._cache.start.x;
      let sy = this._cache.start.y;
      let ex = this._cache.end.x;
      let ey = this._cache.end.y;

      
      
      let offset = this._getViewOffset();

      return  [{ handle: this.HANDLE_TYPE_START,
                 left: sx + offset.x + scroll.X,
                 top: sy + offset.y + scroll.Y,
                 hidden: checkHidden(sx, sy) },
               { handle: this.HANDLE_TYPE_END,
                 left: ex + offset.x + scroll.X,
                 top: ey + offset.y + scroll.Y,
                 hidden: checkHidden(ex, ey) }];
    }
  },

  
  
  _positionHandles: function sh_positionHandles(positions) {
    if (!positions) {
      positions = this._getHandlePositions(this._getScrollPos());
    }
    sendMessageToJava({
      type: "TextSelection:PositionHandles",
      positions: positions,
      rtl: this._isRTL
    });
  },

  subdocumentScrolled: function sh_subdocumentScrolled(aElement) {
    if (this._activeType == this.TYPE_NONE) {
      return;
    }
    let scrollView = aElement.ownerDocument.defaultView;
    let view = this._contentWindow;
    while (true) {
      if (view == scrollView) {
        
        
        if (this._activeType == this.TYPE_SELECTION) {
          this._updateCacheForSelection();
        }
        this._positionHandles();
        break;
      }
      if (view == view.parent) {
        break;
      }
      view = view.parent;
    }
  }
};
