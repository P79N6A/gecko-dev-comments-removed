



this.EXPORTED_SYMBOLS = ["Finder"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
const Services = Cu.import("resource://gre/modules/Services.jsm").Services;

function Finder(docShell) {
  this._fastFind = Cc["@mozilla.org/typeaheadfind;1"].createInstance(Ci.nsITypeAheadFind);
  this._fastFind.init(docShell);

  this._docShell = docShell;
  this._listeners = [];
  this._previousLink = null;
  this._searchString = null;

  docShell.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIWebProgress)
          .addProgressListener(this, Ci.nsIWebProgress.NOTIFY_LOCATION);
}

Finder.prototype = {
  addResultListener: function (aListener) {
    if (this._listeners.indexOf(aListener) === -1)
      this._listeners.push(aListener);
  },

  removeResultListener: function (aListener) {
    this._listeners = this._listeners.filter(l => l != aListener);
  },

  _notify: function (aResult, aFindBackwards, aDrawOutline) {
    this._outlineLink(aDrawOutline);

    let foundLink = this._fastFind.foundLink;
    let linkURL = null;
    if (foundLink) {
      let docCharset = null;
      let ownerDoc = foundLink.ownerDocument;
      if (ownerDoc)
        docCharset = ownerDoc.characterSet;

      if (!this._textToSubURIService) {
        this._textToSubURIService = Cc["@mozilla.org/intl/texttosuburi;1"]
                                      .getService(Ci.nsITextToSubURI);
      }

      linkURL = this._textToSubURIService.unEscapeURIForUI(docCharset, foundLink.href);
    }

    for (let l of this._listeners) {
      l.onFindResult(aResult, aFindBackwards, linkURL);
    }
  },

  get searchString() {
    return this._fastFind.searchString;
  },

  set caseSensitive(aSensitive) {
    this._fastFind.caseSensitive = aSensitive;
  },

  






  fastFind: function (aSearchString, aLinksOnly, aDrawOutline) {
    let result = this._fastFind.find(aSearchString, aLinksOnly);
    this._notify(result, false, aDrawOutline);
  },

  








  findAgain: function (aFindBackwards, aLinksOnly, aDrawOutline) {
    let result = this._fastFind.findAgain(aFindBackwards, aLinksOnly);
    this._notify(result, aFindBackwards, aDrawOutline);
  },

  highlight: function (aHighlight, aWord) {
    this._searchString = aWord;
    let found = this._highlight(aHighlight, aWord, null);
    if (found)
      this._notify(Ci.nsITypeAheadFind.FIND_FOUND, false, false);
    else
      this._notify(Ci.nsITypeAheadFind.FIND_NOTFOUND, false, false);
  },

  removeSelection: function() {
    let fastFind = this._fastFind;

    fastFind.collapseSelection();
    fastFind.setSelectionModeAndRepaint(Ci.nsISelectionController.SELECTION_ON);

    this._restoreOriginalOutline();
  },

  focusContent: function() {
    let fastFind = this._fastFind;

    try {
      
      if (fastFind.foundLink) {
        fastFind.foundLink.focus();
      } else if (fastFind.foundEditable) {
        fastFind.foundEditable.focus();
        fastFind.collapseSelection();
      } else {
        this._getWindow().focus()
      }
    } catch (e) {}
  },

  keyPress: function (aEvent) {
    let controller = this._getSelectionController(this._getWindow());

    switch (aEvent.keyCode) {
      case Ci.nsIDOMKeyEvent.DOM_VK_RETURN:
        if (this._fastFind.foundLink) 
          this._fastFind.foundLink.click();
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_TAB:
        let direction = Services.focus.MOVEFOCUS_FORWARD;
        if (aEvent.shiftKey) {
          direction = Services.focus.MOVEFOCUS_BACKWARD;
        }
        Services.focus.moveFocus(this._getWindow(), null, direction, 0);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP:
        controller.scrollPage(false);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN:
        controller.scrollPage(true);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_UP:
        controller.scrollLine(false);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_DOWN:
        controller.scrollLine(true);
        break;
    }
  },

  _getWindow: function () {
    return this._docShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
  },

  _outlineLink: function (aDrawOutline) {
    let foundLink = this._fastFind.foundLink;

    
    
    if (foundLink == this._previousLink && aDrawOutline)
      return;

    this._restoreOriginalOutline();

    if (foundLink && aDrawOutline) {
      
      this._tmpOutline = foundLink.style.outline;
      this._tmpOutlineOffset = foundLink.style.outlineOffset;

      
      
      
      
      
      foundLink.style.outline = "1px dotted";
      foundLink.style.outlineOffset = "0";

      this._previousLink = foundLink;
    }
  },

  _restoreOriginalOutline: function () {
    
    if (this._previousLink) {
      this._previousLink.style.outline = this._tmpOutline;
      this._previousLink.style.outlineOffset = this._tmpOutlineOffset;
      this._previousLink = null;
    }
  },

  _highlight: function (aHighlight, aWord, aWindow) {
    let win = aWindow || this._getWindow();

    let found = false;
    for (let i = 0; win.frames && i < win.frames.length; i++) {
      if (this._highlight(aHighlight, aWord, win.frames[i]))
        found = true;
    }

    let controller = this._getSelectionController(win);
    let doc = win.document;
    if (!controller || !doc || !doc.documentElement) {
      
      
      return found;
    }

    let body = (doc instanceof Ci.nsIDOMHTMLDocument && doc.body) ?
               doc.body : doc.documentElement;

    if (aHighlight) {
      let searchRange = doc.createRange();
      searchRange.selectNodeContents(body);

      let startPt = searchRange.cloneRange();
      startPt.collapse(true);

      let endPt = searchRange.cloneRange();
      endPt.collapse(false);

      let retRange = null;
      let finder = Cc["@mozilla.org/embedcomp/rangefind;1"]
                     .createInstance()
                     .QueryInterface(Ci.nsIFind);

      finder.caseSensitive = this._fastFind.caseSensitive;

      while ((retRange = finder.Find(aWord, searchRange,
                                     startPt, endPt))) {
        this._highlightRange(retRange, controller);
        startPt = retRange.cloneRange();
        startPt.collapse(false);

        found = true;
      }
    } else {
      
      let sel = controller.getSelection(Ci.nsISelectionController.SELECTION_FIND);
      sel.removeAllRanges();

      
      
      if (this._editors) {
        for (let x = this._editors.length - 1; x >= 0; --x) {
          if (this._editors[x].document == doc) {
            sel = this._editors[x].selectionController
                                  .getSelection(Ci.nsISelectionController.SELECTION_FIND);
            sel.removeAllRanges();
            
            this._unhookListenersAtIndex(x);
          }
        }
      }

      
      found = true;
    }

    return found;
  },

  _highlightRange: function(aRange, aController) {
    let node = aRange.startContainer;
    let controller = aController;

    let editableNode = this._getEditableNode(node);
    if (editableNode)
      controller = editableNode.editor.selectionController;

    let findSelection = controller.getSelection(Ci.nsISelectionController.SELECTION_FIND);
    findSelection.addRange(aRange);

    if (editableNode) {
      
      
      if (!this._editors) {
        this._editors = [];
        this._stateListeners = [];
      }

      let existingIndex = this._editors.indexOf(editableNode.editor);
      if (existingIndex == -1) {
        let x = this._editors.length;
        this._editors[x] = editableNode.editor;
        this._stateListeners[x] = this._createStateListener();
        this._editors[x].addEditActionListener(this);
        this._editors[x].addDocumentStateListener(this._stateListeners[x]);
      }
    }
  },

  _getSelectionController: function(aWindow) {
    
    if (!aWindow.innerWidth || !aWindow.innerHeight)
      return null;

    
    let docShell = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIWebNavigation)
                          .QueryInterface(Ci.nsIDocShell);

    let controller = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                             .getInterface(Ci.nsISelectionDisplay)
                             .QueryInterface(Ci.nsISelectionController);
    return controller;
  },

  







  _getEditableNode: function (aNode) {
    while (aNode) {
      if (aNode instanceof Ci.nsIDOMNSEditableElement)
        return aNode.editor ? aNode : null;

      aNode = aNode.parentNode;
    }
    return null;
  },

  






  _unhookListenersAtIndex: function (aIndex) {
    this._editors[aIndex].removeEditActionListener(this);
    this._editors[aIndex]
        .removeDocumentStateListener(this._stateListeners[aIndex]);
    this._editors.splice(aIndex, 1);
    this._stateListeners.splice(aIndex, 1);
    if (!this._editors.length) {
      delete this._editors;
      delete this._stateListeners;
    }
  },

  





  _removeEditorListeners: function (aEditor) {
    
    
    let idx = this._editors.indexOf(aEditor);
    if (idx == -1)
      return;
    
    this._unhookListenersAtIndex(idx);
  },

  









  







  _checkOverlap: function (aSelectionRange, aFindRange) {
    
    
    
    
    
    if (aFindRange.isPointInRange(aSelectionRange.startContainer,
                                  aSelectionRange.startOffset))
      return true;
    if (aFindRange.isPointInRange(aSelectionRange.endContainer,
                                  aSelectionRange.endOffset))
      return true;
    if (aSelectionRange.isPointInRange(aFindRange.startContainer,
                                       aFindRange.startOffset))
      return true;
    if (aSelectionRange.isPointInRange(aFindRange.endContainer,
                                       aFindRange.endOffset))
      return true;

    return false;
  },

  








  _findRange: function (aSelection, aNode, aOffset) {
    let rangeCount = aSelection.rangeCount;
    let rangeidx = 0;
    let foundContainingRange = false;
    let range = null;

    
    while (!foundContainingRange && rangeidx < rangeCount) {
      range = aSelection.getRangeAt(rangeidx);
      if (range.isPointInRange(aNode, aOffset)) {
        foundContainingRange = true;
        break;
      }
      rangeidx++;
    }

    if (foundContainingRange)
      return range;

    return null;
  },

  

  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {
    if (!aWebProgress.isTopLevel)
      return;

    
    this._previousLink = null;
  },

  

  WillDeleteText: function (aTextNode, aOffset, aLength) {
    let editor = this._getEditableNode(aTextNode).editor;
    let controller = editor.selectionController;
    let fSelection = controller.getSelection(Ci.nsISelectionController.SELECTION_FIND);
    let range = this._findRange(fSelection, aTextNode, aOffset);

    if (range) {
      
      
      if (aTextNode != range.endContainer ||
          aOffset != range.endOffset) {
        
        
        fSelection.removeRange(range);
        if (fSelection.rangeCount == 0)
          this._removeEditorListeners(editor);
      }
    }
  },

  DidInsertText: function (aTextNode, aOffset, aString) {
    let editor = this._getEditableNode(aTextNode).editor;
    let controller = editor.selectionController;
    let fSelection = controller.getSelection(Ci.nsISelectionController.SELECTION_FIND);
    let range = this._findRange(fSelection, aTextNode, aOffset);

    if (range) {
      
      
      if (aTextNode == range.startContainer &&
          aOffset == range.startOffset) {
        range.setStart(range.startContainer,
                       range.startOffset+aString.length);
      } else if (aTextNode != range.endContainer ||
                 aOffset != range.endOffset) {
        
        
        
        fSelection.removeRange(range);
        if (fSelection.rangeCount == 0)
          this._removeEditorListeners(editor);
      }
    }
  },

  WillDeleteSelection: function (aSelection) {
    let editor = this._getEditableNode(aSelection.getRangeAt(0)
                                                 .startContainer).editor;
    let controller = editor.selectionController;
    let fSelection = controller.getSelection(Ci.nsISelectionController.SELECTION_FIND);

    let selectionIndex = 0;
    let findSelectionIndex = 0;
    let shouldDelete = {};
    let numberOfDeletedSelections = 0;
    let numberOfMatches = fSelection.rangeCount;

    
    
    
    

    for (let fIndex = 0; fIndex < numberOfMatches; fIndex++) {
      shouldDelete[fIndex] = false;
      let fRange = fSelection.getRangeAt(fIndex);

      for (let index = 0; index < aSelection.rangeCount; index++) {
        if (shouldDelete[fIndex])
          continue;

        let selRange = aSelection.getRangeAt(index);
        let doesOverlap = this._checkOverlap(selRange, fRange);
        if (doesOverlap) {
          shouldDelete[fIndex] = true;
          numberOfDeletedSelections++;
        }
      }
    }

    
    
    if (numberOfDeletedSelections == 0)
      return;

    for (let i = numberOfMatches - 1; i >= 0; i--) {
      if (shouldDelete[i])
        fSelection.removeRange(fSelection.getRangeAt(i));
    }

    
    if (fSelection.rangeCount == 0)
      this._removeEditorListeners(editor);
  },

  














  














  _onEditorDestruction: function (aListener) {
    
    
    
    
    let idx = 0;
    while (this._stateListeners[idx] != aListener)
      idx++;

    
    this._unhookListenersAtIndex(idx);
  },

  








  _createStateListener: function () {
    return {
      findbar: this,

      QueryInterface: function(aIID) {
        if (aIID.equals(Ci.nsIDocumentStateListener) ||
            aIID.equals(Ci.nsISupports))
          return this;

        throw Components.results.NS_ERROR_NO_INTERFACE;
      },

      NotifyDocumentWillBeDestroyed: function() {
        this.findbar._onEditorDestruction(this);
      },

      
      notifyDocumentCreated: function() {},
      notifyDocumentStateChanged: function(aDirty) {}
    };
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};
