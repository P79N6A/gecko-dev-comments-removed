



let Ci = Components.interfaces;
let Cc = Components.classes;

this.kXLinkNamespace = "http://www.w3.org/1999/xlink";

dump("### ContextMenuHandler.js loaded\n");

var ContextMenuHandler = {
  _types: [],
  _previousState: null,

  init: function ch_init() {
    
    addEventListener("contextmenu", this, false);
    addEventListener("pagehide", this, false);

    
    
    addMessageListener("Browser:ContextCommand", this, false);

    this.popupNode = null;
  },

  handleEvent: function ch_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "contextmenu":
        this._onContentContextMenu(aEvent);
        break;
      case "pagehide":
        this.reset();
        break;
    }
  },

  receiveMessage: function ch_receiveMessage(aMessage) {
    switch (aMessage.name) {
      case "Browser:ContextCommand":
        this._onContextCommand(aMessage);
      break;
    }
  },

  



  _onContextCommand: function _onContextCommand(aMessage) {
    let node = this.popupNode;
    let command = aMessage.json.command;

    switch (command) {
      case "cut":
        this._onCut();
        break;

      case "copy":
        this._onCopy();
        break;

      case "paste":
        this._onPaste();
        break;

      case "select-all":
        this._onSelectAll();
        break;

      case "copy-image-contents":
        this._onCopyImage();
        break;
    }
  },

  



  reset: function ch_reset() {
    this.popupNode = null;
    this._target = null;
  },

  
  _onContentContextMenu: function _onContentContextMenu(aEvent) {
    if (aEvent.defaultPrevented)
      return;

    
    aEvent.stopPropagation();
    aEvent.preventDefault();

    this._processPopupNode(aEvent.originalTarget, aEvent.clientX,
                           aEvent.clientY, aEvent.mozInputSource);
  },

  



  _onSelectAll: function _onSelectAll() {
    if (Util.isTextInput(this._target)) {
      
      this._target.select();
    } else {
      
      content.getSelection().selectAllChildren(content.document);
    }
    this.reset();
  },

  _onPaste: function _onPaste() {
    
    if (Util.isTextInput(this._target)) {
      let edit = this._target.QueryInterface(Ci.nsIDOMNSEditableElement);
      if (edit) {
        edit.editor.paste(Ci.nsIClipboard.kGlobalClipboard);
      } else {
        Util.dumpLn("error: target element does not support nsIDOMNSEditableElement");
      }
    } else if (this._target.isContentEditable) {
      try {
        this._target.ownerDocument.execCommand("paste",
                                               false,
                                               Ci.nsIClipboard.kGlobalClipboard);
      } catch (ex) {
        dump("ContextMenuHandler: exception pasting into contentEditable: " + ex.message + "\n");
      }
    }
    this.reset();
  },

  _onCopyImage: function _onCopyImage() {
    Util.copyImageToClipboard(this._target);
  },

  _onCut: function _onCut() {
    if (Util.isTextInput(this._target)) {
      let edit = this._target.QueryInterface(Ci.nsIDOMNSEditableElement);
      if (edit) {
        edit.editor.cut();
      } else {
        Util.dumpLn("error: target element does not support nsIDOMNSEditableElement");
      }
    } else if (this._target.isContentEditable) {
      try {
        this._target.ownerDocument.execCommand("cut", false);
      } catch (ex) {
        dump("ContextMenuHandler: exception cutting from contentEditable: " + ex.message + "\n");
      }
    }
    this.reset();
  },

  _onCopy: function _onCopy() {
    if (Util.isTextInput(this._target)) {
      let edit = this._target.QueryInterface(Ci.nsIDOMNSEditableElement);
      if (edit) {
        edit.editor.copy();
      } else {
        Util.dumpLn("error: target element does not support nsIDOMNSEditableElement");
      }
    } else {
      let selectionText = this._previousState.string;

      Cc["@mozilla.org/widget/clipboardhelper;1"]
        .getService(Ci.nsIClipboardHelper).copyString(selectionText);
    }
    this.reset();
  },

  



  





  _processPopupNode: function _processPopupNode(aPopupNode, aX, aY, aInputSrc) {
    if (!aPopupNode)
      return;

    let { targetWindow: targetWindow,
          offsetX: offsetX,
          offsetY: offsetY } =
      Util.translateToTopLevelWindow(aPopupNode);

    let popupNode = this.popupNode = aPopupNode;
    let imageUrl = "";

    let state = {
      types: [],
      label: "",
      linkURL: "",
      linkTitle: "",
      linkProtocol: null,
      mediaURL: "",
      contentType: "",
      contentDisposition: "",
      string: "",
    };
    let uniqueStateTypes = new Set();

    
    if (popupNode.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
      
      if (popupNode instanceof Ci.nsIImageLoadingContent && popupNode.currentURI) {
        uniqueStateTypes.add("image");
        state.label = state.mediaURL = popupNode.currentURI.spec;
        imageUrl = state.mediaURL;
        this._target = popupNode;

        
        
        try {
          let imageCache = Cc["@mozilla.org/image/cache;1"].getService(Ci.imgICache);
          let props = imageCache.findEntryProperties(popupNode.currentURI,
                                                     content.document.characterSet);
          if (props) {
            state.contentType = String(props.get("type", Ci.nsISupportsCString));
            state.contentDisposition = String(props.get("content-disposition",
                                                        Ci.nsISupportsCString));
          }
        } catch (ex) {
          Util.dumpLn(ex.message);
          
        }
      }
    }

    let elem = popupNode;
    let isText = false;
    let isEditableText = false;

    while (elem) {
      if (elem.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
        
        if (Util.isLink(elem)) {
          
          
          if (imageUrl == this._getLinkURL(elem)) {
            elem = elem.parentNode;
            continue;
          }

          uniqueStateTypes.add("link");
          state.label = state.linkURL = this._getLinkURL(elem);
          linkUrl = state.linkURL;
          state.linkTitle = popupNode.textContent || popupNode.title;
          state.linkProtocol = this._getProtocol(this._getURI(state.linkURL));
          
          isText = true;
          break;
        }
        
        else if (elem.contentEditable == "true") {
          this._target = elem;
          isEditableText = true;
          isText = true;
          uniqueStateTypes.add("input-text");

          if (elem.textContent.length) {
            uniqueStateTypes.add("selectable");
          } else {
            uniqueStateTypes.add("input-empty");
          }
          break;
        }
        
        else if (Util.isTextInput(elem)) {
          this._target = elem;
          isEditableText = true;
          uniqueStateTypes.add("input-text");

          let selectionStart = elem.selectionStart;
          let selectionEnd = elem.selectionEnd;

          
          if (!(elem instanceof Ci.nsIDOMHTMLInputElement) || elem.mozIsTextField(true)) {
            
            if (selectionStart != selectionEnd) {
              uniqueStateTypes.add("cut");
              uniqueStateTypes.add("copy");
              state.string = elem.value.slice(selectionStart, selectionEnd);
            } else if (elem.value && elem.textLength) {
              
              uniqueStateTypes.add("selectable");
              state.string = elem.value;
            }
          }

          if (!elem.textLength) {
            uniqueStateTypes.add("input-empty");
          }
          break;
        }
        
        else if (Util.isText(elem)) {
          isText = true;
        }
        
        else if (elem instanceof Ci.nsIDOMHTMLMediaElement ||
                   elem instanceof Ci.nsIDOMHTMLVideoElement) {
          state.label = state.mediaURL = (elem.currentSrc || elem.src);
          uniqueStateTypes.add((elem.paused || elem.ended) ?
            "media-paused" : "media-playing");
          if (elem instanceof Ci.nsIDOMHTMLVideoElement) {
            uniqueStateTypes.add("video");
          }
        }
      }

      elem = elem.parentNode;
    }

    
    if (isText) {
      
      
      let selection = targetWindow.getSelection();
      if (selection && this._tapInSelection(selection, aX, aY)) {
        state.string = targetWindow.getSelection().toString();
        uniqueStateTypes.add("copy");
        uniqueStateTypes.add("selected-text");
        if (isEditableText) {
          uniqueStateTypes.add("cut");
        }
      } else {
        
        if (!(
            uniqueStateTypes.has("image") ||
            uniqueStateTypes.has("media") ||
            uniqueStateTypes.has("video") ||
            uniqueStateTypes.has("link") ||
            uniqueStateTypes.has("input-text")
        )) {
          uniqueStateTypes.add("content-text");
        }
      }
    }

    
    if (isEditableText) {
      let flavors = ["text/unicode"];
      let cb = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
      let hasData = cb.hasDataMatchingFlavors(flavors,
                                              flavors.length,
                                              Ci.nsIClipboard.kGlobalClipboard);
      
      if (hasData && !elem.readOnly) {
        uniqueStateTypes.add("paste");
      }
    }
    
    state.xPos = offsetX + aX;
    state.yPos = offsetY + aY;
    state.source = aInputSrc;

    for (let i = 0; i < this._types.length; i++)
      if (this._types[i].handler(state, popupNode))
        uniqueStateTypes.add(this._types[i].name);

    state.types = [type for (type of uniqueStateTypes)];
    this._previousState = state;

    sendAsyncMessage("Content:ContextMenu", state);
  },

  _tapInSelection: function (aSelection, aX, aY) {
    if (!aSelection || !aSelection.rangeCount) {
      return false;
    }
    for (let idx = 0; idx < aSelection.rangeCount; idx++) {
      let range = aSelection.getRangeAt(idx);
      let rect = range.getBoundingClientRect();
      if (Util.pointWithinDOMRect(aX, aY, rect)) {
        return true;
      }
    }
    return false;
  },

  _getLinkURL: function ch_getLinkURL(aLink) {
    let href = aLink.href;
    if (href)
      return href;

    href = aLink.getAttributeNS(kXLinkNamespace, "href");
    if (!href || !href.match(/\S/)) {
      
      
      throw "Empty href";
    }

    return Util.makeURLAbsolute(aLink.baseURI, href);
  },

  _getURI: function ch_getURI(aURL) {
    try {
      return Util.makeURI(aURL);
    } catch (ex) { }

    return null;
  },

  _getProtocol: function ch_getProtocol(aURI) {
    if (aURI)
      return aURI.scheme;
    return null;
  },

  







  registerType: function registerType(aName, aHandler) {
    this._types.push({name: aName, handler: aHandler});
  },

  
  unregisterType: function unregisterType(aName) {
    this._types = this._types.filter(function(type) type.name != aName);
  }
};
this.ContextMenuHandler = ContextMenuHandler;

ContextMenuHandler.init();
