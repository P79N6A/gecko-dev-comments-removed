



"use strict";


const PHONE_NUMBER_CONTAINERS = "td,div";

var SelectionHandler = {
  HANDLE_TYPE_START: "START",
  HANDLE_TYPE_MIDDLE: "MIDDLE",
  HANDLE_TYPE_END: "END",

  TYPE_NONE: 0,
  TYPE_CURSOR: 1,
  TYPE_SELECTION: 2,

  SELECT_ALL: 0,
  SELECT_AT_POINT: 1,

  
  
  _cache: null,
  _activeType: 0, 
  _draggingHandles: false, 
  _ignoreCompositionChanges: false, 
  _prevHandlePositions: [], 

  
  _prevTargetElementHasText: null,

  
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
    Services.obs.addObserver(this, "Tab:Selected", false);
    Services.obs.addObserver(this, "after-viewport-change", false);
    Services.obs.addObserver(this, "TextSelection:Move", false);
    Services.obs.addObserver(this, "TextSelection:Position", false);
    Services.obs.addObserver(this, "TextSelection:End", false);
    Services.obs.addObserver(this, "TextSelection:Action", false);
    Services.obs.addObserver(this, "TextSelection:LayerReflow", false);

    BrowserApp.deck.addEventListener("pagehide", this, false);
    BrowserApp.deck.addEventListener("blur", this, true);
    BrowserApp.deck.addEventListener("scroll", this, true);
  },

  _removeObservers: function sh_removeObservers() {
    Services.obs.removeObserver(this, "Gesture:SingleTap");
    Services.obs.removeObserver(this, "Tab:Selected");
    Services.obs.removeObserver(this, "after-viewport-change");
    Services.obs.removeObserver(this, "TextSelection:Move");
    Services.obs.removeObserver(this, "TextSelection:Position");
    Services.obs.removeObserver(this, "TextSelection:End");
    Services.obs.removeObserver(this, "TextSelection:Action");
    Services.obs.removeObserver(this, "TextSelection:LayerReflow");

    BrowserApp.deck.removeEventListener("pagehide", this, false);
    BrowserApp.deck.removeEventListener("blur", this, true);
    BrowserApp.deck.removeEventListener("scroll", this, true);
  },

  observe: function sh_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      
      
      case "TextSelection:LayerReflow": {
        if (this._activeType == this.TYPE_SELECTION) {
          this._updateCacheForSelection();
        }
        if (this._activeType != this.TYPE_NONE) {
          this._positionHandlesOnChange();
        }
        break;
      }

      
      case "TextSelection:UpdateCaretPos":
        
        this._positionHandles();
        break;

      case "Gesture:SingleTap": {
        if (this._activeType == this.TYPE_CURSOR) {
          
          
          this._deactivate();
        }
        break;
      }
      case "Tab:Selected":
      case "TextSelection:End":
        this._closeSelection();
        break;
      case "TextSelection:Action":
        for (let type in this.actions) {
          if (this.actions[type].id == aData) {
            this.actions[type].action(this._targetElement);
            break;
          }
        }
        break;
      case "after-viewport-change": {
        if (this._activeType == this.TYPE_SELECTION) {
          
          this._updateCacheForSelection();
        }
        break;
      }
      case "TextSelection:Move": {
        let data = JSON.parse(aData);
        if (this._activeType == this.TYPE_SELECTION) {
          this._startDraggingHandles();
          this._moveSelection(data.handleType == this.HANDLE_TYPE_START, data.x, data.y);

        } else if (this._activeType == this.TYPE_CURSOR) {
          this._startDraggingHandles();

          
          this._ignoreCompositionChanges = true;
          this._moveCaret(data.x, data.y);

          
          this._positionHandles();
        }
        break;
      }
      case "TextSelection:Position": {
        if (this._activeType == this.TYPE_SELECTION) {
          this._startDraggingHandles();

          
          let isStartHandle = JSON.parse(aData).handleType == this.HANDLE_TYPE_START;
          try {
            let selectionReversed = this._updateCacheForSelection(isStartHandle);
            if (selectionReversed) {
              
              let selection = this._getSelection();
              let anchorNode = selection.anchorNode;
              let anchorOffset = selection.anchorOffset;
              selection.collapse(selection.focusNode, selection.focusOffset);
              selection.extend(anchorNode, anchorOffset);
            }
          } catch (e) {
            
            this._closeSelection();
            break;
          }

          this._stopDraggingHandles();
          this._positionHandles();
          
          this._updateMenu();

        } else if (this._activeType == this.TYPE_CURSOR) {
          
          this._ignoreCompositionChanges = false;
          this._stopDraggingHandles();
          this._positionHandles();

        } else {
          Cu.reportError("Ignored \"TextSelection:Position\" message during invalid selection status");
        }

        break;
      }

      case "TextSelection:Get":
        Messaging.sendRequest({
          type: "TextSelection:Data",
          requestId: aData,
          text: this._getSelectedText()
        });
        break;
    }
  },

  
  
  _startDraggingHandles: function sh_startDraggingHandles() {
    if (!this._draggingHandles) {
      this._draggingHandles = true;
      Messaging.sendRequest({ type: "TextSelection:DraggingHandle", dragging: true });
    }
  },

  
  
  _stopDraggingHandles: function sh_stopDraggingHandles() {
    if (this._draggingHandles) {
      this._draggingHandles = false;
      Messaging.sendRequest({ type: "TextSelection:DraggingHandle", dragging: false });
    }
  },

  handleEvent: function sh_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "scroll":
        
        this._positionHandlesOnChange();
        break;

      case "pagehide":
      case "blur":
        this._closeSelection();
        break;

      
      case "keyup":
        
      case "compositionupdate":
      case "compositionend":
        
        if (!this._ignoreCompositionChanges) {
          this._positionHandles();
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
    
    if (this._draggingHandles) {
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

  











  startSelection: function sh_startSelection(aElement, aOptions = { mode: SelectionHandler.SELECT_ALL }) {
    
    this._closeSelection();

    this._initTargetInfo(aElement, this.TYPE_SELECTION);

    
    if (!this._performSelection(aOptions)) {
      this._deactivate();
      return false;
    }

    
    let selection = this._getSelection();
    if (!selection || selection.rangeCount == 0 || selection.getRangeAt(0).collapsed) {
      this._deactivate();
      return false;
    }

    
    selection.QueryInterface(Ci.nsISelectionPrivate).addSelectionListener(this);
    this._activeType = this.TYPE_SELECTION;

    
    this._cache = { start: {}, end: {}};
    this._updateCacheForSelection();

    let scroll = this._getScrollPos();
    
    let positions = this._getHandlePositions(scroll);

    if (aOptions.mode == this.SELECT_AT_POINT && !this._selectionNearClick(scroll.X + aOptions.x,
                                                                      scroll.Y + aOptions.y,
                                                                      positions)) {
        this._closeSelection();
        return false;
    }

    
    this._positionHandles(positions);
    Messaging.sendRequest({
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_START, this.HANDLE_TYPE_END]
    });
    this._updateMenu();
    return true;
  },

  


  _performSelection: function sh_performSelection(aOptions) {
    if (aOptions.mode == this.SELECT_AT_POINT) {
      
      this._contentWindow.getSelection().removeAllRanges();
      if (!this._domWinUtils.selectAtPoint(aOptions.x, aOptions.y, Ci.nsIDOMWindowUtils.SELECT_WORDNOSPACE)) {
        return false;
      }

      
      if (this._isPhoneNumber(this._getSelection().toString())) {
        this._selectSmartPhoneNumber();
      }

      return true;
    }

    if (aOptions.mode != this.SELECT_ALL) {
      Cu.reportError("SelectionHandler.js: _performSelection() Invalid selection mode " + aOptions.mode);
      return false;
    }

    
    if (this._targetElement instanceof HTMLPreElement)  {
      return this._domWinUtils.selectAtPoint(1, 1, Ci.nsIDOMWindowUtils.SELECT_PARAGRAPH);
    }

    
    let editor = this._getEditor();
    if (editor) {
      editor.selectAll();
    } else {
      this._getSelectionController().selectAll();
    }

    
    let selection = this._getSelection();
    let lastNode = selection.focusNode;
    while (lastNode && lastNode.lastChild) {
      lastNode = lastNode.lastChild;
    }

    if (lastNode instanceof Text) {
      try {
        selection.extend(lastNode, lastNode.length);
      } catch (e) {
        Cu.reportError("SelectionHandler.js: _performSelection() whitespace trim fails: lastNode[" + lastNode +
          "] lastNode.length[" + lastNode.length + "]");
      }
    }

    return true;
  },

  



  _selectSmartPhoneNumber: function() {
    this._extendPhoneNumberSelection("forward");
    this._reversePhoneNumberSelectionDir();

    this._extendPhoneNumberSelection("backward");
    this._reversePhoneNumberSelectionDir();
  },

  


  _extendPhoneNumberSelection: function(direction) {
    let selection = this._getSelection();

    
    while (true) {
      
      let focusNode = selection.focusNode;
      let focusOffset = selection.focusOffset;
      selection.modify("extend", direction, "character");

      
      if (selection.focusNode == focusNode && selection.focusOffset == focusOffset) {
        return;
      }

      
      if (!this._isPhoneNumber(selection.toString().trim())) {
        
        selection.collapse(selection.anchorNode, selection.anchorOffset);
        selection.extend(focusNode, focusOffset);
        return;
      }

      
      if (selection.focusNode != focusNode) {
        let nextContainer = (selection.focusNode instanceof Text) ?
          selection.focusNode.parentNode : selection.focusNode;
        if (nextContainer.matches &&
            nextContainer.matches(PHONE_NUMBER_CONTAINERS)) {
          
          selection.collapse(selection.anchorNode, selection.anchorOffset);
          selection.extend(focusNode, focusOffset);
          return
        }
      }
    }
  },

  


  _reversePhoneNumberSelectionDir: function(direction) {
    let selection = this._getSelection();

    let anchorNode = selection.anchorNode;
    let anchorOffset = selection.anchorOffset;
    selection.collapse(selection.focusNode, selection.focusOffset);
    selection.extend(anchorNode, anchorOffset);
  },

  
  _selectionNearClick: function(aX, aY, aPositions) {
      let distance = 0;

      
      if (aPositions[0].left < aX && aX < aPositions[1].left
          && aPositions[0].top < aY && aY < aPositions[1].top) {
        distance = 0;
      } else {
        
        let selectposX = (aPositions[0].left + aPositions[1].left) / 2;
        let selectposY = (aPositions[0].top + aPositions[1].top) / 2;

        let dx = Math.abs(selectposX - aX);
        let dy = Math.abs(selectposY - aY);
        distance = dx + dy;
      }

      let maxSelectionDistance = Services.prefs.getIntPref("browser.ui.selection.distance");
      return (distance < maxSelectionDistance);
  },

  

  _getValue: function(obj, name, defaultValue) {
    if (!(name in obj))
      return defaultValue;

    if (typeof obj[name] == "function")
      return obj[name](this._targetElement);

    return obj[name];
  },

  addAction: function(action) {
    if (!action.id)
      action.id = uuidgen.generateUUID().toString()

    if (this.actions[action.id])
      throw "Action with id " + action.id + " already added";

    
    this.actions[action.id] = action;
    this._updateMenu();
    return action.id;
  },

  removeAction: function(id) {
    
    delete this.actions[id];
    this._updateMenu();
  },

  _updateMenu: function() {
    if (this._activeType == this.TYPE_NONE) {
      return;
    }

    
    let actions = [];
    for (let type in this.actions) {
      let action = this.actions[type];
      if (action.selector.matches(this._targetElement)) {
        let a = {
          id: action.id,
          label: this._getValue(action, "label", ""),
          icon: this._getValue(action, "icon", "drawable://ic_status_logo"),
          showAsAction: this._getValue(action, "showAsAction", true),
          order: this._getValue(action, "order", 0)
        };
        actions.push(a);
      }
    }

    actions.sort((a, b) => b.order - a.order);

    Messaging.sendRequest({
      type: "TextSelection:Update",
      actions: actions
    });
  },

  


  actions: {
    SELECT_ALL: {
      label: Strings.browser.GetStringFromName("contextmenu.selectAll"),
      id: "selectall_action",
      icon: "drawable://ab_select_all",
      action: function(aElement) {
        SelectionHandler.startSelection(aElement);
        UITelemetry.addEvent("action.1", "actionbar", null, "select_all");
      },
      order: 5,
      selector: {
        matches: function(aElement) {
          return (aElement.textLength != 0);
        }
      }
    },

    CUT: {
      label: Strings.browser.GetStringFromName("contextmenu.cut"),
      id: "cut_action",
      icon: "drawable://ab_cut",
      action: function(aElement) {
        let start = aElement.selectionStart;
        let end   = aElement.selectionEnd;

        SelectionHandler.copySelection();
        aElement.value = aElement.value.substring(0, start) + aElement.value.substring(end)

        
        SelectionHandler.attachCaret(aElement);
        UITelemetry.addEvent("action.1", "actionbar", null, "cut");
      },
      order: 4,
      selector: {
        matches: function(aElement) {
          return SelectionHandler.isElementEditableText(aElement) ?
            SelectionHandler.isSelectionActive() : false;
        }
      }
    },

    COPY: {
      label: Strings.browser.GetStringFromName("contextmenu.copy"),
      id: "copy_action",
      icon: "drawable://ab_copy",
      action: function() {
        SelectionHandler.copySelection();
        UITelemetry.addEvent("action.1", "actionbar", null, "copy");
      },
      order: 3,
      selector: {
        matches: function(aElement) {
          
          
          if (aElement instanceof Ci.nsIDOMHTMLInputElement && !aElement.mozIsTextField(true)) {
            return false;
          }
          return SelectionHandler.isSelectionActive();
        }
      }
    },

    PASTE: {
      label: Strings.browser.GetStringFromName("contextmenu.paste"),
      id: "paste_action",
      icon: "drawable://ab_paste",
      action: function(aElement) {
        if (aElement && (aElement instanceof Ci.nsIDOMNSEditableElement)) {
          let target = aElement.QueryInterface(Ci.nsIDOMNSEditableElement);
          target.editor.paste(Ci.nsIClipboard.kGlobalClipboard);
          target.focus();
          SelectionHandler._closeSelection();
          UITelemetry.addEvent("action.1", "actionbar", null, "paste");
        }
      },
      order: 2,
      selector: {
        matches: function(aElement) {
          if (SelectionHandler.isElementEditableText(aElement)) {
            let flavors = ["text/unicode"];
            return Services.clipboard.hasDataMatchingFlavors(flavors, flavors.length, Ci.nsIClipboard.kGlobalClipboard);
          }
          return false;
        }
      }
    },

    SHARE: {
      label: Strings.browser.GetStringFromName("contextmenu.share"),
      id: "share_action",
      icon: "drawable://ic_menu_share",
      action: function() {
        SelectionHandler.shareSelection();
        UITelemetry.addEvent("action.1", "actionbar", null, "share");
      },
      selector: {
        matches: function() {
          if (!ParentalControls.isAllowed(ParentalControls.SHARE)) {
            return false;
          }

          return SelectionHandler.isSelectionActive();
        }
      }
    },

    SEARCH: {
      label: function() {
        return Strings.browser.formatStringFromName("contextmenu.search", [Services.search.defaultEngine.name], 1);
      },
      id: "search_action",
      icon: "drawable://ab_search",
      action: function() {
        SelectionHandler.searchSelection();
        SelectionHandler._closeSelection();
        UITelemetry.addEvent("action.1", "actionbar", null, "search");
      },
      order: 1,
      selector: {
        matches: function() {
          return SelectionHandler.isSelectionActive();
        }
      }
    },

    CALL: {
      label: Strings.browser.GetStringFromName("contextmenu.call"),
      id: "call_action",
      icon: "drawable://phone",
      action: function() {
        SelectionHandler.callSelection();
        UITelemetry.addEvent("action.1", "actionbar", null, "call");
      },
      order: 1,
      selector: {
        matches: function () {
          return SelectionHandler._getSelectedPhoneNumber() != null;
        }
      }
    }
  },

  





  attachCaret: function sh_attachCaret(aElement) {
    
    if (aElement.disabled || InputWidgetHelper.hasInputWidget(aElement) || !this.isElementEditableText(aElement)) {
      return false;
    }

    this._initTargetInfo(aElement, this.TYPE_CURSOR);

    
    Services.obs.addObserver(this, "TextSelection:UpdateCaretPos", false);
    BrowserApp.deck.addEventListener("keyup", this, false);
    BrowserApp.deck.addEventListener("compositionupdate", this, false);
    BrowserApp.deck.addEventListener("compositionend", this, false);

    this._activeType = this.TYPE_CURSOR;

    
    this._positionHandles();
    Messaging.sendRequest({
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_MIDDLE]
    });
    this._updateMenu();

    return true;
  },

  
  _initTargetInfo: function sh_initTargetInfo(aElement, aSelectionType) {
    this._targetElement = aElement;
    if (aElement instanceof Ci.nsIDOMNSEditableElement) {
      if (aSelectionType === this.TYPE_SELECTION) {
        
        
        aElement.blur();
      }
      
      aElement.focus();
    }

    this._stopDraggingHandles();
    this._contentWindow = aElement.ownerDocument.defaultView;
    this._isRTL = (this._contentWindow.getComputedStyle(aElement, "").direction == "rtl");

    this._addObservers();
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
    if (!selection)
      return "";

    if (this._targetElement instanceof Ci.nsIDOMHTMLTextAreaElement) {
      return selection.QueryInterface(Ci.nsISelectionPrivate).
        toStringWithFormat("text/plain", Ci.nsIDocumentEncoder.OutputPreformatted | Ci.nsIDocumentEncoder.OutputRaw, 0);
    }

    return selection.toString().trim();
  },

  _getEditor: function sh_getEditor() {
    if (this._targetElement instanceof Ci.nsIDOMNSEditableElement) {
      return this._targetElement.QueryInterface(Ci.nsIDOMNSEditableElement).editor;
    }
    return this._contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebNavigation)
                              .QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIEditingSession)
                              .getEditorForWindow(this._contentWindow);
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

  
  isSelectionActive: function sh_isSelectionActive() {
    return (this._activeType == this.TYPE_SELECTION);
  },

  isElementEditableText: function (aElement) {
    return (((aElement instanceof HTMLInputElement && aElement.mozIsTextField(false)) ||
            (aElement instanceof HTMLTextAreaElement)) && !aElement.readOnly);
  },

  






  _moveSelectionInEditable: function sh_moveSelectionInEditable(aAnchorX, aX, aCaretPos) {
    let anchorOffset = aX < aAnchorX ? this._targetElement.selectionEnd
                                     : this._targetElement.selectionStart;
    let newOffset = aCaretPos.offset;
    let [start, end] = anchorOffset <= newOffset ?
                       [anchorOffset, newOffset] :
                       [newOffset, anchorOffset];
    this._targetElement.setSelectionRange(start, end);
  },

  





  _moveSelection: function sh_moveSelection(aIsStartHandle, aX, aY) {
    
    
    let viewOffset = this._getViewOffset();
    let caretPos = this._contentWindow.document.caretPositionFromPoint(aX - viewOffset.x, aY - viewOffset.y);
    if (!caretPos) {
      
      return;
    }

    
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
        let anchorX = this._isRTL ? this._cache.start.x : this._cache.end.x;
        this._moveSelectionInEditable(anchorX, aX, caretPos);
      } else {
        let focusNode = selection.focusNode;
        let focusOffset = selection.focusOffset;
        selection.collapse(caretPos.offsetNode, caretPos.offset);
        selection.extend(focusNode, focusOffset);
      }
    } else {
      if (targetIsEditable) {
        let anchorX = this._isRTL ? this._cache.end.x : this._cache.start.x;
        this._moveSelectionInEditable(anchorX, aX, caretPos);
      } else {
        selection.extend(caretPos.offsetNode, caretPos.offset);
      }
    }
  },

  _moveCaret: function sh_moveCaret(aX, aY) {
    
    let range = document.createRange();
    range.selectNodeContents(this._targetElement.QueryInterface(Ci.nsIDOMNSEditableElement).editor.rootElement);
    let textBounds = range.getBoundingClientRect();

    
    let editorBounds = this._domWinUtils.sendQueryContentEvent(this._domWinUtils.QUERY_EDITOR_RECT, 0, 0, 0, 0,
                                                               this._domWinUtils.QUERY_CONTENT_FLAG_USE_XP_LINE_BREAK);
    
    
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

    this._domWinUtils.sendMouseEventToWindow("mousedown", aX, aY, 0, 0, 0, true);
    this._domWinUtils.sendMouseEventToWindow("mouseup", aX, aY, 0, 0, 0, true);
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
      Messaging.sendRequest({
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
      let isPrivate = PrivateBrowsingUtils.isBrowserPrivate(parent.browser);
      
      BrowserApp.addTab(req.uri.spec, {parentId: parent.id,
                                       selected: true,
                                       isPrivate: isPrivate});
    }
    this._closeSelection();
  },

  _phoneRegex: /^\+?[0-9\s,-.\(\)*#pw]{1,30}$/,

  _getSelectedPhoneNumber: function sh_getSelectedPhoneNumber() {
    return this._isPhoneNumber(this._getSelectedText().trim());
  },

  _isPhoneNumber: function sh_isPhoneNumber(selectedText) {
    return (this._phoneRegex.test(selectedText) ? selectedText : null);
  },

  callSelection: function sh_callSelection() {
    let selectedText = this._getSelectedPhoneNumber();
    if (selectedText) {
      BrowserApp.loadURI("tel:" + selectedText);
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
    this._stopDraggingHandles();
    
    Messaging.sendRequest({ type: "TextSelection:HideHandles" });

    this._removeObservers();

    
    if (this._activeType == this.TYPE_CURSOR) {
      Services.obs.removeObserver(this, "TextSelection:UpdateCaretPos");
      BrowserApp.deck.removeEventListener("keyup", this);
      BrowserApp.deck.removeEventListener("compositionupdate", this);
      BrowserApp.deck.removeEventListener("compositionend", this);
    }

    this._contentWindow = null;
    this._targetElement = null;
    this._isRTL = false;
    this._cache = null;
    this._ignoreCompositionChanges = false;
    this._prevHandlePositions = [];
    this._prevTargetElementHasText = null;

    this._activeType = this.TYPE_NONE;
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

  
  
  _updateCacheForSelection: function sh_updateCacheForSelection(aIsStartHandle) {
    let rects = this._getSelection().getRangeAt(0).getClientRects();
    if (!rects[0]) {
      
      throw "Failed to update cache for invalid selection";
    }

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
      
      
      let cursor = this._domWinUtils.sendQueryContentEvent(this._domWinUtils.QUERY_CARET_RECT, this._targetElement.selectionEnd, 0, 0, 0,
                                                           this._domWinUtils.QUERY_CONTENT_FLAG_USE_XP_LINE_BREAK);
      
      
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

  
  
  _positionHandlesOnChange: function() {
    
    let samePositions = function(aPrev, aCurr) {
      if (aPrev.length != aCurr.length) {
        return false;
      }
      for (let i = 0; i < aPrev.length; i++) {
        if (aPrev[i].left != aCurr[i].left ||
            aPrev[i].top != aCurr[i].top ||
            aPrev[i].hidden != aCurr[i].hidden) {
          return false;
        }
      }
      return true;
    }

    let positions = this._getHandlePositions(this._getScrollPos());
    if (!samePositions(this._prevHandlePositions, positions)) {
      this._positionHandles(positions);
    }
  },

  
  
  
  
  _positionHandles: function sh_positionHandles(positions) {
    if (!positions) {
      positions = this._getHandlePositions(this._getScrollPos());
    }
    Messaging.sendRequest({
      type: "TextSelection:PositionHandles",
      positions: positions,
      rtl: this._isRTL
    });
    this._prevHandlePositions = positions;

    
    let currTargetElementHasText = (this._targetElement.textLength > 0);
    if (currTargetElementHasText != this._prevTargetElementHasText) {
      this._prevTargetElementHasText = currTargetElementHasText;
      this._updateMenu();
    }
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
