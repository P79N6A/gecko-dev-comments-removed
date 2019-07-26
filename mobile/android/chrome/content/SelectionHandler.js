


"use strict";

var SelectionHandler = {
  HANDLE_TYPE_START: "START",
  HANDLE_TYPE_MIDDLE: "MIDDLE",
  HANDLE_TYPE_END: "END",

  TYPE_NONE: 0,
  TYPE_CURSOR: 1,
  TYPE_SELECTION: 2,

  
  
  cache: null,
  _activeType: 0, 

  
  get _view() {
    if (this._viewRef)
      return this._viewRef.get();
    return null;
  },

  set _view(aView) {
    this._viewRef = Cu.getWeakReference(aView);
  },

  
  get _target() {
    if (this._targetRef)
      return this._targetRef.get();
    return null;
  },

  set _target(aTarget) {
    this._targetRef = Cu.getWeakReference(aTarget);
  },

  get _cwu() {
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
          
          this.updateCacheForSelection();
        }
        break;
      }
      case "TextSelection:Move": {
        let data = JSON.parse(aData);
        if (this._activeType == this.TYPE_SELECTION)
          this.moveSelection(data.handleType == this.HANDLE_TYPE_START, data.x, data.y);
        else if (this._activeType == this.TYPE_CURSOR) {
          
          this._sendMouseEvents(data.x, data.y);

          
          this.positionHandles();
        }
        break;
      }
      case "TextSelection:Position": {
        if (this._activeType == this.TYPE_SELECTION) {
          let data = JSON.parse(aData);

          
          let selectionReversed = this.updateCacheForSelection(data.handleType == this.HANDLE_TYPE_START);
          if (selectionReversed) {
            
            if (this._isRTL) {
              this._sendMouseEvents(this.cache.end.x, this.cache.end.y, false);
              this._sendMouseEvents(this.cache.start.x, this.cache.start.y, true);
            } else {
              this._sendMouseEvents(this.cache.start.x, this.cache.start.y, false);
              this._sendMouseEvents(this.cache.end.x, this.cache.end.y, true);
            }
          }

          
          this.positionHandles();
        } else if (this._activeType == this.TYPE_CURSOR) {
          this.positionHandles();
        }
        break;
      }
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
          this._closeSelection();
        }
        break;
    }
  },

  _ignoreCollapsedSelection: false,

  notifySelectionChanged: function sh_notifySelectionChanged(aDoc, aSel, aReason) {
    if (aSel.isCollapsed) {
      
      if (this._ignoreCollapsedSelection)
        return;

      
      
      if (aReason & Ci.nsISelectionListener.MOUSEDOWN_REASON) {
        this._ignoreCollapsedSelection = true;
        return;
      }

      
      this._closeSelection();
    }

    this._ignoreCollapsedSelection = false;
  },

  
  canSelect: function sh_canSelect(aElement) {
    return !(aElement instanceof Ci.nsIDOMHTMLButtonElement ||
             aElement instanceof Ci.nsIDOMHTMLEmbedElement ||
             aElement instanceof Ci.nsIDOMHTMLImageElement ||
             aElement instanceof Ci.nsIDOMHTMLMediaElement) &&
             aElement.style.MozUserSelect != 'none';
  },

  
  startSelection: function sh_startSelection(aElement, aX, aY) {
    
    this._closeSelection();

    
    this._view = aElement.ownerDocument.defaultView;

    if (aElement instanceof Ci.nsIDOMNSEditableElement)
      this._target = aElement;
    else
      this._target = this._view;

    this._addObservers();
    this._view.addEventListener("pagehide", this, false);
    this._isRTL = (this._view.getComputedStyle(aElement, "").direction == "rtl");

    
    
    let selection = this.getSelection();
    selection.removeAllRanges();

    
    this._sendMouseEvents(aX, aY, false);

    try {
      let selectionController = this.getSelectionController();

      
      selectionController.wordMove(false, false);

      
      selectionController.wordMove(!this._isRTL, true);
    } catch(e) {
      
      this._closeSelection();
      return;
    }

    
    if (!selection.rangeCount || !selection.getRangeAt(0) || !selection.toString().trim().length) {
      selection.collapseToStart();
      this._closeSelection();
      return;
    }

    
    selection.QueryInterface(Ci.nsISelectionPrivate).addSelectionListener(this);

    
    this.cache = { start: {}, end: {}};
    this.updateCacheForSelection();

    this._activeType = this.TYPE_SELECTION;
    this.positionHandles();

    sendMessageToJava({
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_START, this.HANDLE_TYPE_END]
    });

    if (aElement instanceof Ci.nsIDOMNSEditableElement)
      aElement.focus();
  },

  getSelection: function sh_getSelection() {
    if (this._target instanceof Ci.nsIDOMNSEditableElement)
      return this._target.QueryInterface(Ci.nsIDOMNSEditableElement).editor.selection;
    else
      return this._target.getSelection();
  },

  _getSelectedText: function sh_getSelectedText() {
    let selection = this.getSelection();
    if (selection)
      return selection.toString().trim();
    return "";
  },

  getSelectionController: function sh_getSelectionController() {
    if (this._target instanceof Ci.nsIDOMNSEditableElement)
      return this._target.QueryInterface(Ci.nsIDOMNSEditableElement).editor.selectionController;
    else
      return this._target.QueryInterface(Ci.nsIInterfaceRequestor).
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

    let selectionController = this.getSelectionController();
    selectionController.selectAll();
    this.updateCacheForSelection();
    this.positionHandles();
  },

  
  
  moveSelection: function sh_moveSelection(aIsStartHandle, aX, aY) {
    
    if (aIsStartHandle) {
      this.cache.start.x = aX;
      this.cache.start.y = aY;
    } else {
      this.cache.end.x = aX;
      this.cache.end.y = aY;
    }

    
    
    if (this._isRTL) {
      
      if (!aIsStartHandle)
        this._sendMouseEvents(this.cache.end.x, this.cache.end.y, false);

      
      this._sendMouseEvents(this.cache.start.x, this.cache.start.y, true);
    } else {
      
      if (aIsStartHandle)
        this._sendMouseEvents(this.cache.start.x, this.cache.start.y, false);

      
      this._sendMouseEvents( this.cache.end.x, this.cache.end.y, true);
    }
  },

  _sendMouseEvents: function sh_sendMouseEvents(aX, aY, useShift) {
    
    
    if (this._activeType == this.TYPE_CURSOR) {
      
      let range = document.createRange();
      range.selectNodeContents(this._target.QueryInterface(Ci.nsIDOMNSEditableElement).editor.rootElement);
      let textBounds = range.getBoundingClientRect();

      
      let editorBounds = this._cwu.sendQueryContentEvent(this._cwu.QUERY_EDITOR_RECT, 0, 0, 0, 0);
      let editorRect = new Rect(editorBounds.left, editorBounds.top, editorBounds.width, editorBounds.height);

      
      let rect = new Rect(textBounds.left, textBounds.top, textBounds.width, textBounds.height);
      rect.restrictTo(editorRect);

      
      
      
      
      if (aY < rect.y + 1) {
        aY = rect.y + 1;
        this.getSelectionController().scrollLine(false);
      } else if (aY > rect.y + rect.height - 1) {
        aY = rect.y + rect.height - 1;
        this.getSelectionController().scrollLine(true);
      }

      
      if (aX < rect.x) {
        aX = rect.x;
        this.getSelectionController().scrollCharacter(false);
      } else if (aX > rect.x + rect.width) {
        aX = rect.x + rect.width;
        this.getSelectionController().scrollCharacter(true);
      }
    } else if (this._activeType == this.TYPE_SELECTION) {
      
      aY -= 1;
    }

    this._cwu.sendMouseEventToWindow("mousedown", aX, aY, 0, 0, useShift ? Ci.nsIDOMNSEvent.SHIFT_MASK : 0, true);
    this._cwu.sendMouseEventToWindow("mouseup", aX, aY, 0, 0, useShift ? Ci.nsIDOMNSEvent.SHIFT_MASK : 0, true);
  },

  copySelection: function sh_copySelection() {
    let selectedText = this._getSelectedText();
    if (selectedText.length) {
      let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);
      clipboard.copyString(selectedText, this._view.document);
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

  


  _closeSelection: function sh_closeSelection() {
    
    if (this._activeType == this.TYPE_NONE)
      return;

    if (this._activeType == this.TYPE_SELECTION) {
      let selection = this.getSelection();
      if (selection) {
        
        
        selection.QueryInterface(Ci.nsISelectionPrivate).removeSelectionListener(this);
        selection.removeAllRanges();
      }
    }

    this._activeType = this.TYPE_NONE;

    sendMessageToJava({ type: "TextSelection:HideHandles" });

    this._removeObservers();
    this._view.removeEventListener("pagehide", this, false);
    this._view.removeEventListener("keydown", this, false);
    this._view.removeEventListener("blur", this, true);
    this._view = null;
    this._target = null;
    this._isRTL = false;
    this.cache = null;
  },

  _getViewOffset: function sh_getViewOffset() {
    let offset = { x: 0, y: 0 };
    let win = this._view;

    
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
    let rangeRect = this.getSelection().getRangeAt(0).getBoundingClientRect();
    let radius = ElementTouchHelper.getTouchRadius();
    return (aX - offset.x > rangeRect.left - radius.left &&
            aX - offset.x < rangeRect.right + radius.right &&
            aY - offset.y > rangeRect.top - radius.top &&
            aY - offset.y < rangeRect.bottom + radius.bottom);
  },

  
  
  updateCacheForSelection: function sh_updateCacheForSelection(aIsStartHandle) {
    let selection = this.getSelection();
    let rects = selection.getRangeAt(0).getClientRects();
    let start = { x: this._isRTL ? rects[0].right : rects[0].left, y: rects[0].bottom };
    let end = { x: this._isRTL ? rects[rects.length - 1].left : rects[rects.length - 1].right, y: rects[rects.length - 1].bottom };

    let selectionReversed = false;
    if (this.cache.start) {
      
      selectionReversed = (aIsStartHandle && (end.y > this.cache.end.y || (end.y == this.cache.end.y && end.x > this.cache.end.x))) ||
                          (!aIsStartHandle && (start.y < this.cache.start.y || (start.y == this.cache.start.y && start.x < this.cache.start.x)));
    }

    this.cache.start = start;
    this.cache.end = end;

    return selectionReversed;
  },

  showThumb: function sh_showThumb(aElement) {
    if (!aElement)
      return;

    
    this._view = aElement.ownerDocument.defaultView;
    this._target = aElement;

    this._addObservers();
    this._view.addEventListener("pagehide", this, false);
    this._view.addEventListener("keydown", this, false);
    this._view.addEventListener("blur", this, true);

    this._activeType = this.TYPE_CURSOR;
    this.positionHandles();

    sendMessageToJava({
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_MIDDLE]
    });
  },

  positionHandles: function sh_positionHandles() {
    let scrollX = {}, scrollY = {};
    this._view.top.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).getScrollXY(false, scrollX, scrollY);

    
    
    
    
    let checkHidden = function(x, y) {
      return false;
    };
    if (this._view.frameElement) {
      let bounds = this._view.frameElement.getBoundingClientRect();
      checkHidden = function(x, y) {
        return x < 0 || y < 0 || x > bounds.width || y > bounds.height;
      }
    }

    let positions = null;
    if (this._activeType == this.TYPE_CURSOR) {
      
      
      let cursor = this._cwu.sendQueryContentEvent(this._cwu.QUERY_CARET_RECT, this._target.selectionEnd, 0, 0, 0);
      let x = cursor.left;
      let y = cursor.top + cursor.height;
      positions = [{ handle: this.HANDLE_TYPE_MIDDLE,
                     left: x + scrollX.value,
                     top: y + scrollY.value,
                     hidden: checkHidden(x, y) }];
    } else {
      let sx = this.cache.start.x;
      let sy = this.cache.start.y;
      let ex = this.cache.end.x;
      let ey = this.cache.end.y;

      
      
      let offset = this._getViewOffset();

      positions = [{ handle: this.HANDLE_TYPE_START,
                     left: sx + offset.x + scrollX.value,
                     top: sy + offset.y + scrollY.value,
                     hidden: checkHidden(sx, sy) },
                   { handle: this.HANDLE_TYPE_END,
                     left: ex + offset.x + scrollX.value,
                     top: ey + offset.y + scrollY.value,
                     hidden: checkHidden(ex, ey) }];
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
    let view = this._view;
    while (true) {
      if (view == scrollView) {
        
        
        if (this._activeType == this.TYPE_SELECTION) {
          this.updateCacheForSelection();
        }
        this.positionHandles();
        break;
      }
      if (view == view.parent) {
        break;
      }
      view = view.parent;
    }
  }
};
