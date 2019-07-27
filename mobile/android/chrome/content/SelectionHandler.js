



"use strict";


const PHONE_NUMBER_CONTAINERS = "td,div";
const DEFER_CLOSE_TRIGGER_MS = 125; 

var SelectionHandler = {

  
  ERROR_NONE: "",

  
  START_ERROR_INVALID_MODE: "Invalid selection mode requested.",
  START_ERROR_NONTEXT_INPUT: "Target element by definition contains no text.",
  START_ERROR_NO_WORD_SELECTED: "No word selected at point.",
  START_ERROR_SELECT_WORD_FAILED: "Word selection at point failed.",
  START_ERROR_SELECT_ALL_PARAGRAPH_FAILED: "Select-All Paragraph failed.",
  START_ERROR_NO_SELECTION: "Selection performed, but nothing resulted.",
  START_ERROR_PROXIMITY: "Selection target and result seem unrelated.",

  
  ATTACH_ERROR_INCOMPATIBLE: "Element disabled, handled natively, or not editable.",

  HANDLE_TYPE_ANCHOR: "ANCHOR",
  HANDLE_TYPE_CARET: "CARET",
  HANDLE_TYPE_FOCUS: "FOCUS",

  TYPE_NONE: 0,
  TYPE_CURSOR: 1,
  TYPE_SELECTION: 2,

  SELECT_ALL: 0,
  SELECT_AT_POINT: 1,

  
  
  _cache: { anchorPt: {}, focusPt: {} },
  _targetIsRTL: false,
  _anchorIsRTL: false,
  _focusIsRTL: false,

  _activeType: 0, 
  _selectionPrivate: null, 
  _selectionID: null, 

  _draggingHandles: false, 
  _dragStartAnchorOffset: null, 
  _dragStartFocusOffset: null, 

  _ignoreCompositionChanges: false, 
  _prevHandlePositions: [], 
  _deferCloseTimer: null, 

  
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

  
  get _idService() {
    delete this._idService;
    return this._idService = Cc["@mozilla.org/uuid-generator;1"].
      getService(Ci.nsIUUIDGenerator);
  },

  _addObservers: function sh_addObservers() {
    Services.obs.addObserver(this, "Gesture:SingleTap", false);
    Services.obs.addObserver(this, "Tab:Selected", false);
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
    
    if (this._deferCloseTimer) {
      return;
    }

    switch (aTopic) {
      
      
      case "TextSelection:LayerReflow": {
        if (this._activeType == this.TYPE_SELECTION) {
          this._updateSelectionListener();
        }
        if (this._activeType != this.TYPE_NONE) {
          this._positionHandlesOnChange();
        }
        break;
      }

      case "Gesture:SingleTap": {
        if (this._activeType == this.TYPE_CURSOR) {
          
          
          this._deactivate();
        }
        break;
      }

      case "Tab:Selected":
        this._closeSelection();
        break;

      case "TextSelection:End":
        let data = JSON.parse(aData);
        
        if (this._selectionID === data.selectionID) {
          this._closeSelection();
        }
        break;

      case "TextSelection:Action":
        for (let type in this.actions) {
          if (this.actions[type].id == aData) {
            this.actions[type].action(this._targetElement);
            break;
          }
        }
        break;

      case "TextSelection:Move": {
        let data = JSON.parse(aData);
        if (this._activeType == this.TYPE_SELECTION) {
          this._startDraggingHandles();
          this._moveSelection(data.handleType, new Point(data.x, data.y));

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
          this._ensureSelectionDirection();
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
      let selection = this._getSelection();
      this._dragStartAnchorOffset = selection.anchorOffset;
      this._dragStartFocusOffset = selection.focusOffset;
      Messaging.sendRequest({ type: "TextSelection:DraggingHandle", dragging: true });
    }
  },

  
  
  _stopDraggingHandles: function sh_stopDraggingHandles() {
    if (this._draggingHandles) {
      this._draggingHandles = false;
      this._dragStartAnchorOffset = null;
      this._dragStartFocusOffset = null;
      Messaging.sendRequest({ type: "TextSelection:DraggingHandle", dragging: false });
    }
  },

  handleEvent: function sh_handleEvent(aEvent) {
    
    if (this._deferCloseTimer) {
      return;
    }

    switch (aEvent.type) {
      case "scroll":
        
        this._positionHandlesOnChange();
        break;

      case "pagehide": {
        
        let tab = BrowserApp.getTabForWindow(aEvent.originalTarget.defaultView);
        if (tab == BrowserApp.selectedTab) {
          this._closeSelection();
        }
        break;
      }

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

  


  _addSelectionListener: function(selection) {
    this._selectionPrivate = selection.QueryInterface(Ci.nsISelectionPrivate);
    this._selectionPrivate.addSelectionListener(this);
  },

  






  _updateSelectionListener: function() {
    if (!(this._targetElement instanceof Ci.nsIDOMNSEditableElement)) {
      return;
    }

    let selection = this._getSelection();
    if (this._selectionPrivate != selection.QueryInterface(Ci.nsISelectionPrivate)) {
      this._removeSelectionListener();
      this._addSelectionListener(selection);
    }
  },

  


  _removeSelectionListener: function() {
    this._selectionPrivate.removeSelectionListener(this);
    this._selectionPrivate = null;
  },

  


  notifySelectionChanged: function sh_notifySelectionChanged(aDocument, aSelection, aReason) {
    
    this._cancelDeferredCloseSelection();

    
    if (this._draggingHandles) {
      return;
    }

    
    if ((aReason & Ci.nsISelectionListener.COLLAPSETOSTART_REASON) ||
        (aReason & Ci.nsISelectionListener.COLLAPSETOEND_REASON)) {
      this._closeSelection();
      return;
    }

    
    if (!aSelection.toString()) {
      this._deferCloseSelection();
      return;
    }

    
    this._positionHandles();
  },

  











  startSelection: function sh_startSelection(aElement, aOptions = { mode: SelectionHandler.SELECT_ALL }) {
    
    this._closeSelection();

    if (this._isNonTextInputElement(aElement)) {
      return this.START_ERROR_NONTEXT_INPUT;
    }

    const focus = Services.focus.focusedWindow;
    if (focus) {
      
      Services.focus.clearFocus(focus);
    }

    this._initTargetInfo(aElement, this.TYPE_SELECTION);

    
    let selectionResult = this._performSelection(aOptions);
    if (selectionResult !== this.ERROR_NONE) {
      this._deactivate();
      return selectionResult;
    }

    
    let selection = this._getSelection();
    if (!selection ||
        selection.rangeCount == 0 ||
        selection.getRangeAt(0).collapsed ||
        this._getSelectedText().length == 0) {
      this._deactivate();
      return this.START_ERROR_NO_SELECTION;
    }

    
    this._addSelectionListener(selection);
    this._activeType = this.TYPE_SELECTION;

    
    let scroll = this._getScrollPos();
    let positions = this._getHandlePositions(scroll);

    if (aOptions.mode == this.SELECT_AT_POINT &&
        !this._selectionNearClick(scroll.X + aOptions.x, scroll.Y + aOptions.y, positions)) {
        this._closeSelection();
        return this.START_ERROR_PROXIMITY;
    }

    
    this._positionHandles(positions);
    Messaging.sendRequest({
      selectionID: this._selectionID,
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_ANCHOR, this.HANDLE_TYPE_FOCUS]
    });
    this._updateMenu();
    return this.ERROR_NONE;
  },

  


  _performSelection: function sh_performSelection(aOptions) {
    if (aOptions.mode == this.SELECT_AT_POINT) {
      
      this._contentWindow.getSelection().removeAllRanges();
      try {
        if (!this._domWinUtils.selectAtPoint(aOptions.x, aOptions.y, Ci.nsIDOMWindowUtils.SELECT_WORDNOSPACE)) {
          return this.START_ERROR_NO_WORD_SELECTED;
        }
      } catch (e) {
        return this.START_ERROR_SELECT_WORD_FAILED;
      }

      
      if (this._isPhoneNumber(this._getSelection().toString())) {
        this._selectSmartPhoneNumber();
      }

      return this.ERROR_NONE;
    }

    
    if (aOptions.mode != this.SELECT_ALL) {
      return this.START_ERROR_INVALID_MODE;
    }

    
    if (this._targetElement instanceof HTMLPreElement)  {
      try {
        this._domWinUtils.selectAtPoint(1, 1, Ci.nsIDOMWindowUtils.SELECT_PARAGRAPH);
        return this.ERROR_NONE;
      } catch (e) {
        return this.START_ERROR_SELECT_ALL_PARAGRAPH_FAILED;
      }
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

    return this.ERROR_NONE;
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
          
          return !aElement.isContentEditable && SelectionHandler.isElementEditableText(aElement) ?
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
        if (aElement) {
          let target = SelectionHandler._getEditor();
          aElement.focus();
          target.paste(Ci.nsIClipboard.kGlobalClipboard);
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
    
    this._closeSelection();

    
    if (aElement.disabled || InputWidgetHelper.hasInputWidget(aElement) || !this.isElementEditableText(aElement)) {
      return this.ATTACH_ERROR_INCOMPATIBLE;
    }

    this._initTargetInfo(aElement, this.TYPE_CURSOR);

    
    BrowserApp.deck.addEventListener("keyup", this, false);
    BrowserApp.deck.addEventListener("compositionupdate", this, false);
    BrowserApp.deck.addEventListener("compositionend", this, false);

    this._activeType = this.TYPE_CURSOR;

    
    this._positionHandles();
    Messaging.sendRequest({
      selectionID: this._selectionID,
      type: "TextSelection:ShowHandles",
      handles: [this.HANDLE_TYPE_CARET]
    });
    this._updateMenu();

    return this.ERROR_NONE;
  },

  
  _initTargetInfo: function sh_initTargetInfo(aElement, aSelectionType) {
    this._targetElement = aElement;
    if (aElement instanceof Ci.nsIDOMNSEditableElement) {
      if (aSelectionType === this.TYPE_SELECTION) {
        
        
        aElement.blur();
      }
      
      aElement.focus();
    }

    this._selectionID = this._idService.generateUUID().toString();
    this._stopDraggingHandles();
    this._contentWindow = aElement.ownerDocument.defaultView;
    this._targetIsRTL = (this._contentWindow.getComputedStyle(aElement, "").direction == "rtl");

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
            (aElement instanceof HTMLTextAreaElement)) && !aElement.readOnly) ||
            aElement.isContentEditable;
  },

  _isNonTextInputElement: function(aElement) {
    return (aElement instanceof HTMLInputElement && !aElement.mozIsTextField(false));
  },

  




  _moveSelection: function sh_moveSelection(handleType, handlePt) {
    let isAnchorHandle = (handleType == this.HANDLE_TYPE_ANCHOR);

    
    
    let viewOffset = this._getViewOffset();
    let ptX = handlePt.x - viewOffset.x;
    let ptY = handlePt.y - viewOffset.y;
    let cwd = this._contentWindow.document;
    let caretPos = cwd.caretPositionFromPoint(ptX, ptY);
    if (!caretPos) {
      return;
    }

    
    let targetIsEditable = this._targetElement instanceof Ci.nsIDOMNSEditableElement;
    if (targetIsEditable && (caretPos.offsetNode != this._targetElement)) {
      return;
    }

    
    
    
    if (targetIsEditable) {
      let start = this._dragStartAnchorOffset;
      let end = this._dragStartFocusOffset;
      if (isAnchorHandle) {
        start = caretPos.offset;
      } else {
        end = caretPos.offset;
      }
      if (start > end) {
        [start, end] = [end, start];
      }
      this._targetElement.setSelectionRange(start, end);
      return;
    }

    
    
    
    
    let selection = this._getSelection();
    if (isAnchorHandle) {
      let focusNode = selection.focusNode;
      let focusOffset = selection.focusOffset;
      selection.collapse(caretPos.offsetNode, caretPos.offset);
      selection.extend(focusNode, focusOffset);
    } else {
      selection.extend(caretPos.offsetNode, caretPos.offset);
    }
  },

  _moveCaret: function sh_moveCaret(aX, aY) {
    
    let range = document.createRange();
    range.selectNodeContents(this._getEditor().rootElement);
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

  








  _deferCloseSelection: function() {
    
    this._deferCloseTimer = setTimeout((function() {
      
      this._deferCloseTimer = null;
      this._closeSelection();
    }).bind(this), DEFER_CLOSE_TRIGGER_MS);

    
    if (this._prevHandlePositions.length) {
      let positions = this._prevHandlePositions;
      for (let i in positions) {
        positions[i].hidden = true;
      }

      Messaging.sendRequest({
        type: "TextSelection:PositionHandles",
        positions: positions,
      });
    }
  },

  


  _cancelDeferredCloseSelection: function() {
    if (this._deferCloseTimer) {
      clearTimeout(this._deferCloseTimer);
      this._deferCloseTimer = null;
    }
  },

  


  _closeSelection: function sh_closeSelection() {
    
    if (this._activeType == this.TYPE_NONE)
      return;

    if (this._activeType == this.TYPE_SELECTION)
      this._clearSelection();

    this._deactivate();
  },

  _clearSelection: function sh_clearSelection() {
    
    this._cancelDeferredCloseSelection();

    let selection = this._getSelection();
    if (selection) {
      
      this._removeSelectionListener();

      
      
      if (selection.rangeCount != 0) {
        if (this.isElementEditableText(this._targetElement)) {
          selection.collapseToStart();
        } else {
          selection.removeAllRanges();
        }
      }
    }
  },

  _deactivate: function sh_deactivate() {
    this._stopDraggingHandles();
    
    Messaging.sendRequest({ type: "TextSelection:HideHandles" });

    this._removeObservers();

    
    if (this._activeType == this.TYPE_CURSOR) {
      BrowserApp.deck.removeEventListener("keyup", this);
      BrowserApp.deck.removeEventListener("compositionupdate", this);
      BrowserApp.deck.removeEventListener("compositionend", this);
    }

    this._contentWindow = null;
    this._targetElement = null;
    this._targetIsRTL = false;
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

  





  _ensureSelectionDirection: function() {
    
    if (this._targetElement instanceof Ci.nsIDOMNSEditableElement) {
      return;
    }

    
    let qcEventResult = this._domWinUtils.sendQueryContentEvent(
      this._domWinUtils.QUERY_SELECTED_TEXT, 0, 0, 0, 0);
    if (!qcEventResult.reversed) {
      return;
    }

    
    let selection = this._getSelection();
    let newFocusNode = selection.anchorNode;
    let newFocusOffset = selection.anchorOffset;

    selection.collapse(selection.focusNode, selection.focusOffset);
    selection.extend(newFocusNode, newFocusOffset);
  },

  






  _updateCacheForSelection: function() {
    let selection = this._getSelection();
    let rects = selection.getRangeAt(0).getClientRects();
    if (rects.length == 0) {
      
      throw "Failed to update cache for invalid selection";
    }

    
    
    this._anchorIsRTL = this._isNodeRTL(selection.anchorNode);
    let anchorIdx = 0;
    this._cache.anchorPt = (this._anchorIsRTL) ?
      new Point(rects[anchorIdx].right, rects[anchorIdx].bottom) :
      new Point(rects[anchorIdx].left, rects[anchorIdx].bottom);

    
    
    this._focusIsRTL = this._isNodeRTL(selection.focusNode);
    let focusIdx = rects.length - 1;
    this._cache.focusPt = (this._focusIsRTL) ?
      new Point(rects[focusIdx].left, rects[focusIdx].bottom) :
      new Point(rects[focusIdx].right, rects[focusIdx].bottom);
  },

  


  _isNodeRTL: function(node) {
    
    
    while (node && !(node instanceof Element)) {
      node = node.parentNode;
    }

    
    if (!node) {
      return this._targetIsRTL;
    }

    let nodeWin = node.ownerDocument.defaultView;
    let nodeStyle = nodeWin.getComputedStyle(node, "");
    return (nodeStyle.direction == "rtl");
  },

  _getHandlePositions: function(scroll = this._getScrollPos()) {
    
    
    
    
    let checkHidden = function(x, y) {
      return false;
    };
    if (this._contentWindow.frameElement) {
      let bounds = this._contentWindow.frameElement.getBoundingClientRect();
      checkHidden = function(x, y) {
        return x < 0 || y < 0 || x > bounds.width || y > bounds.height;
      };
    }

    if (this._activeType == this.TYPE_CURSOR) {
      
      
      let cursor = this._domWinUtils.sendQueryContentEvent(this._domWinUtils.QUERY_CARET_RECT, this._targetElement.selectionEnd, 0, 0, 0,
                                                           this._domWinUtils.QUERY_CONTENT_FLAG_USE_XP_LINE_BREAK);
      
      
      let x = cursor.left / window.devicePixelRatio;
      let y = (cursor.top + cursor.height) / window.devicePixelRatio;
      return [{ handle: this.HANDLE_TYPE_CARET,
                left: x + scroll.X,
                top: y + scroll.Y,
                rtl: this._targetIsRTL,
                hidden: checkHidden(x, y) }];
    }

    
    this._updateCacheForSelection();
    let offset = this._getViewOffset();
    return  [{ handle: this.HANDLE_TYPE_ANCHOR,
               left: this._cache.anchorPt.x + offset.x + scroll.X,
               top: this._cache.anchorPt.y + offset.y + scroll.Y,
               rtl: this._anchorIsRTL,
               hidden: checkHidden(this._cache.anchorPt.x, this._cache.anchorPt.y) },
             { handle: this.HANDLE_TYPE_FOCUS,
               left: this._cache.focusPt.x + offset.x + scroll.X,
               top: this._cache.focusPt.y + offset.y + scroll.Y,
               rtl: this._focusIsRTL,
               hidden: checkHidden(this._cache.focusPt.x, this._cache.focusPt.y) }];
  },

  
  
  _positionHandlesOnChange: function() {
    
    let samePositions = function(aPrev, aCurr) {
      if (aPrev.length != aCurr.length) {
        return false;
      }
      for (let i = 0; i < aPrev.length; i++) {
        if (aPrev[i].left != aCurr[i].left ||
            aPrev[i].top != aCurr[i].top ||
            aPrev[i].rtl != aCurr[i].rtl ||
            aPrev[i].hidden != aCurr[i].hidden) {
          return false;
        }
      }
      return true;
    }

    let positions = this._getHandlePositions();
    if (!samePositions(this._prevHandlePositions, positions)) {
      this._positionHandles(positions);
    }
  },

  
  
  
  
  _positionHandles: function(positions = this._getHandlePositions()) {
    Messaging.sendRequest({
      type: "TextSelection:PositionHandles",
      positions: positions,
    });
    this._prevHandlePositions = positions;

    
    let currTargetElementHasText = (this._targetElement.textLength > 0);
    if (currTargetElementHasText != this._prevTargetElementHasText) {
      this._prevTargetElementHasText = currTargetElementHasText;
      this._updateMenu();
    }
  },

  subdocumentScrolled: function sh_subdocumentScrolled(aElement) {
    
    if (this._deferCloseTimer) {
      return;
    }

    if (this._activeType == this.TYPE_NONE) {
      return;
    }
    let scrollView = aElement.ownerDocument.defaultView;
    let view = this._contentWindow;
    while (true) {
      if (view == scrollView) {
        
        
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
