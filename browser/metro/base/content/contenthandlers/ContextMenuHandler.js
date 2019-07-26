



const kXLinkNamespace = "http://www.w3.org/1999/xlink";

dump("### ContextMenuHandler.js loaded\n");

var ContextMenuHandler = {
  _types: [],
  _previousState: null,

  init: function ch_init() {
    
    addEventListener("contextmenu", this, false);
    addEventListener("pagehide", this, false);

    
    
    addMessageListener("Browser:ContextCommand", this, false);
    
    
    
    addMessageListener("Browser:InvokeContextAtPoint", this, false);

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
      case "Browser:InvokeContextAtPoint":
        this._onContextAtPoint(aMessage);
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

      case "play":
      case "pause":
        if (node instanceof Ci.nsIDOMHTMLMediaElement)
          node[command]();
        break;

      case "videotab":
        if (node instanceof Ci.nsIDOMHTMLVideoElement) {
          node.pause();
          Cu.import("resource:///modules/video.jsm");
          Video.fullScreenSourceElement = node;
          sendAsyncMessage("Browser:FullScreenVideo:Start");
        }
        break;

      case "select-all":
        this._onSelectAll();
        break;

      case "copy-image-contents":
        this._onCopyImage();
        break;
    }
  },

  


  _onContextAtPoint: function _onContextAtPoint(aMessage) {
    
    
    let { element, frameX, frameY } =
      elementFromPoint(aMessage.json.xPos, aMessage.json.yPos);
    this._processPopupNode(element, frameX, frameY,
                           Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
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
    if (this._isTextInput(this._target)) {
      
      this._target.select();
    } else {
      
      content.getSelection().selectAllChildren(content.document);
    }
    this.reset();
  },

  _onPaste: function _onPaste() {
    
    if (this._isTextInput(this._target)) {
      let edit = this._target.QueryInterface(Ci.nsIDOMNSEditableElement);
      if (edit) {
        edit.editor.paste(Ci.nsIClipboard.kGlobalClipboard);
      } else {
        Util.dumpLn("error: target element does not support nsIDOMNSEditableElement");
      }
    }
    this.reset();
  },

  _onCopyImage: function _onCopyImage() {
    Util.copyImageToClipboard(this._target);
  },

  _onCut: function _onCut() {
    if (this._isTextInput(this._target)) {
      let edit = this._target.QueryInterface(Ci.nsIDOMNSEditableElement);
      if (edit) {
        edit.editor.cut();
      } else {
        Util.dumpLn("error: target element does not support nsIDOMNSEditableElement");
      }
    }
    this.reset();
  },

  _onCopy: function _onCopy() {
    if (this._isTextInput(this._target)) {
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

  



   




  _translateToTopLevelWindow: function _translateToTopLevelWindow(aPopupNode) {
    let offsetX = 0;
    let offsetY = 0;
    let element = aPopupNode;
    while (element &&
           element.ownerDocument &&
           element.ownerDocument.defaultView != content) {
      element = element.ownerDocument.defaultView.frameElement;
      let rect = element.getBoundingClientRect();
      offsetX += rect.left;
      offsetY += rect.top;
    }
    let win = null;
    if (element == aPopupNode)
      win = content;
    else
      win = element.contentDocument.defaultView;
    return { targetWindow: win, offsetX: offsetX, offsetY: offsetY };
  },

  





  _processPopupNode: function _processPopupNode(aPopupNode, aX, aY, aInputSrc) {
    if (!aPopupNode)
      return;

    let { targetWindow: targetWindow,
          offsetX: offsetX,
          offsetY: offsetY } =
      this._translateToTopLevelWindow(aPopupNode);

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

    
    if (popupNode.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
      
      if (popupNode instanceof Ci.nsIImageLoadingContent && popupNode.currentURI) {
        state.types.push("image");
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

    while (elem) {
      if (elem.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
        
        if (this._isLink(elem)) {
          
          
          if (imageUrl == this._getLinkURL(elem)) {
            elem = elem.parentNode;
            continue;
          }

          state.types.push("link");
          state.label = state.linkURL = this._getLinkURL(elem);
          linkUrl = state.linkURL;
          state.linkTitle = popupNode.textContent || popupNode.title;
          state.linkProtocol = this._getProtocol(this._getURI(state.linkURL));
          
          isText = true;
          break;
        } else if (this._isTextInput(elem)) {
          let selectionStart = elem.selectionStart;
          let selectionEnd = elem.selectionEnd;

          state.types.push("input-text");
          this._target = elem;

          
          if (!(elem instanceof Ci.nsIDOMHTMLInputElement) || elem.mozIsTextField(true)) {
            if (selectionStart != selectionEnd) {
              state.types.push("cut");
              state.types.push("copy");
              state.string = elem.value.slice(selectionStart, selectionEnd);
            }
            if (elem.value && (selectionStart > 0 || selectionEnd < elem.textLength)) {
              state.types.push("selectable");
              state.string = elem.value;
            }
          }

          if (!elem.textLength) {
            state.types.push("input-empty");
          }

          let flavors = ["text/unicode"];
          let cb = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
          let hasData = cb.hasDataMatchingFlavors(flavors,
                                                  flavors.length,
                                                  Ci.nsIClipboard.kGlobalClipboard);
          if (hasData && !elem.readOnly) {
            state.types.push("paste");
          }
          break;
        } else if (this._isText(elem)) {
          isText = true;
        } else if (elem instanceof Ci.nsIDOMHTMLMediaElement ||
                   elem instanceof Ci.nsIDOMHTMLVideoElement) {
          state.label = state.mediaURL = (elem.currentSrc || elem.src);
          state.types.push((elem.paused || elem.ended) ?
            "media-paused" : "media-playing");
          if (elem instanceof Ci.nsIDOMHTMLVideoElement) {
            state.types.push("video");
          }
        }
      }

      elem = elem.parentNode;
    }

    
    if (isText) {
      
      
      let selection = targetWindow.getSelection();
      if (selection && selection.toString().length > 0) {
        state.string = targetWindow.getSelection().toString();
        state.types.push("copy");
        state.types.push("selected-text");
      } else {
        
        if (state.types.indexOf("image") == -1 &&
            state.types.indexOf("media") == -1 &&
            state.types.indexOf("video") == -1 &&
            state.types.indexOf("link") == -1 &&
            state.types.indexOf("input-text") == -1) {
          state.types.push("content-text");
        }
      }
    }

    
    state.xPos = offsetX + aX;
    state.yPos = offsetY + aY;
    state.source = aInputSrc;

    for (let i = 0; i < this._types.length; i++)
      if (this._types[i].handler(state, popupNode))
        state.types.push(this._types[i].name);

    this._previousState = state;

    sendAsyncMessage("Content:ContextMenu", state);
  },

  _isTextInput: function _isTextInput(element) {
    return ((element instanceof Ci.nsIDOMHTMLInputElement &&
             element.mozIsTextField(false)) ||
            element instanceof Ci.nsIDOMHTMLTextAreaElement);
  },

  _isLink: function _isLink(element) {
    return ((element instanceof Ci.nsIDOMHTMLAnchorElement && element.href) ||
            (element instanceof Ci.nsIDOMHTMLAreaElement && element.href) ||
            element instanceof Ci.nsIDOMHTMLLinkElement ||
            element.getAttributeNS(kXLinkNamespace, "type") == "simple");
  },

  _isText: function _isText(element) {
    return (element instanceof Ci.nsIDOMHTMLParagraphElement ||
            element instanceof Ci.nsIDOMHTMLDivElement ||
            element instanceof Ci.nsIDOMHTMLLIElement ||
            element instanceof Ci.nsIDOMHTMLPreElement ||
            element instanceof Ci.nsIDOMHTMLHeadingElement ||
            element instanceof Ci.nsIDOMHTMLTableCellElement ||
            element instanceof Ci.nsIDOMHTMLBodyElement);
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

ContextMenuHandler.init();
