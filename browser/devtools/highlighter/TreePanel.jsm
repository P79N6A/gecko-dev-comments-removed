










































const Cu = Components.utils;

Cu.import("resource:///modules/domplate.jsm");
Cu.import("resource:///modules/InsideOutBox.jsm");
Cu.import("resource:///modules/Services.jsm");

var EXPORTED_SYMBOLS = ["TreePanel"];







function TreePanel(aContext, aIUI) {
  this._init(aContext, aIUI);
};

TreePanel.prototype = {
  showTextNodesWithWhitespace: false,
  id: "treepanel", 

  



  get container()
  {
    return this.document.getElementById("inspector-tree-panel");
  },

  





  _init: function TP__init(aContext, aIUI)
  {
    this.IUI = aIUI;
    this.window = aContext;
    this.document = this.window.document;

    domplateUtils.setDOM(this.window);

    let isOpen = this.isOpen.bind(this);

    this.registrationObject = {
      id: this.id,
      label: this.IUI.strings.GetStringFromName("htmlPanel.label"),
      tooltiptext: this.IUI.strings.GetStringFromName("htmlPanel.tooltiptext"),
      accesskey: this.IUI.strings.GetStringFromName("htmlPanel.accesskey"),
      context: this,
      get isOpen() isOpen(),
      show: this.open,
      hide: this.close,
      onSelect: this.select,
      panel: this.container,
      unregister: this.destroy,
    };
    this.editingEvents = {};

    this._boundClose = this.close.bind(this);
    this.container.addEventListener("popuphiding", this._boundClose, false);

    
    this.IUI.registerTool(this.registrationObject);
  },

  


  initializeIFrame: function TP_initializeIFrame()
  {
    if (!this.initializingTreePanel || this.treeLoaded) {
      return;
    }
    this.treeBrowserDocument = this.treeIFrame.contentDocument;
    this.treePanelDiv = this.treeBrowserDocument.createElement("div");
    this.treeBrowserDocument.body.appendChild(this.treePanelDiv);
    this.treePanelDiv.ownerPanel = this;
    this.ioBox = new InsideOutBox(this, this.treePanelDiv);
    this.ioBox.createObjectBox(this.IUI.win.document.documentElement);
    this.treeLoaded = true;
    this.treeIFrame.addEventListener("click", this.onTreeClick.bind(this), false);
    this.treeIFrame.addEventListener("dblclick", this.onTreeDblClick.bind(this), false);
    delete this.initializingTreePanel;
    Services.obs.notifyObservers(null,
      this.window.INSPECTOR_NOTIFICATIONS.TREEPANELREADY, null);
    if (this.IUI.selection)
      this.select(this.IUI.selection, true);
  },

  


  open: function TP_open()
  {
    if (this.initializingTreePanel && !this.treeLoaded) {
      return;
    }

    this.initializingTreePanel = true;
    this.container.hidden = false;

    this.treeIFrame = this.document.getElementById("inspector-tree-iframe");
    if (!this.treeIFrame) {
      let resizerBox = this.document.getElementById("tree-panel-resizer-box");
      this.treeIFrame = this.document.createElement("iframe");
      this.treeIFrame.setAttribute("id", "inspector-tree-iframe");
      this.treeIFrame.setAttribute("flex", "1");
      this.treeIFrame.setAttribute("type", "content");
      this.treeIFrame = this.container.insertBefore(this.treeIFrame, resizerBox);
    }

    let self = this;
    this.container.addEventListener("popupshown", function treePanelShown() {
      self.container.removeEventListener("popupshown",
        treePanelShown, false);

        self.treeIFrame.addEventListener("load",
          function loadedInitializeTreePanel() {
            self.treeIFrame.removeEventListener("load",
              loadedInitializeTreePanel, true);
            self.initializeIFrame();
          }, true);

      let src = self.treeIFrame.getAttribute("src");
      if (src != "chrome://browser/content/inspector.html") {
        self.treeIFrame.setAttribute("src",
          "chrome://browser/content/inspector.html");
      } else {
        self.treeIFrame.contentWindow.location.reload();
      }
    }, false);

    const panelWidthRatio = 7 / 8;
    const panelHeightRatio = 1 / 5;

    let width = parseInt(this.IUI.win.outerWidth * panelWidthRatio);
    let height = parseInt(this.IUI.win.outerHeight * panelHeightRatio);
    let y = Math.min(this.document.defaultView.screen.availHeight - height,
      this.IUI.win.innerHeight);

    this.container.openPopup(this.browser, "overlap", 0, 0,
      false, false);

    this.container.moveTo(80, y);
    this.container.sizeTo(width, height);
  },

  


  close: function TP_close()
  {
    if (this.treePanelDiv) {
      this.treePanelDiv.ownerPanel = null;
      let parent = this.treePanelDiv.parentNode;
      parent.removeChild(this.treePanelDiv);
      delete this.treePanelDiv;
      delete this.treeBrowserDocument;
    }

    this.treeLoaded = false;
    this.container.hidePopup();
  },

  



  isOpen: function TP_isOpen()
  {
    return this.treeLoaded && this.container.state == "open";
  },

  





  createObjectBox: function TP_createObjectBox(object, isRoot)
  {
    let tag = domplateUtils.getNodeTag(object);
    if (tag)
      return tag.replace({object: object}, this.treeBrowserDocument);
  },

  getParentObject: function TP_getParentObject(node)
  {
    let parentNode = node ? node.parentNode : null;

    if (!parentNode) {
      
      
      if (node && node == this.window.Node.DOCUMENT_NODE) {
        
        if (node.defaultView) {
          let embeddingFrame = node.defaultView.frameElement;
          if (embeddingFrame)
            return embeddingFrame.parentNode;
        }
      }
      
      return null;  
    }

    if (parentNode.nodeType == this.window.Node.DOCUMENT_NODE) {
      if (parentNode.defaultView) {
        return parentNode.defaultView.frameElement;
      }
      
      return null;
    }

    if (!parentNode.localName)
      return null;

    return parentNode;
  },

  getChildObject: function TP_getChildObject(node, index, previousSibling)
  {
    if (!node)
      return null;

    if (node.contentDocument) {
      
      if (index == 0) {
        return node.contentDocument.documentElement;  
      }
      return null;
    }

    if (node instanceof this.window.GetSVGDocument) {
      let svgDocument = node.getSVGDocument();
      if (svgDocument) {
        
        if (index == 0) {
          return svgDocument.documentElement;  
        }
        return null;
      }
    }

    let child = null;
    if (previousSibling)  
      child = this.getNextSibling(previousSibling);
    else
      child = this.getFirstChild(node);

    if (this.showTextNodesWithWhitespace)
      return child;

    for (; child; child = this.getNextSibling(child)) {
      if (!domplateUtils.isWhitespaceText(child))
        return child;
    }

    return null;  
  },

  getFirstChild: function TP_getFirstChild(node)
  {
    this.treeWalker = node.ownerDocument.createTreeWalker(node,
      this.window.NodeFilter.SHOW_ALL, null, false);
    return this.treeWalker.firstChild();
  },

  getNextSibling: function TP_getNextSibling(node)
  {
    let next = this.treeWalker.nextSibling();

    if (!next)
      delete this.treeWalker;

    return next;
  },

  
  

  




  onTreeClick: function TP_onTreeClick(aEvent)
  {
    let node;
    let target = aEvent.target;
    let hitTwisty = false;
    if (this.hasClass(target, "twisty")) {
      node = this.getRepObject(aEvent.target.nextSibling);
      hitTwisty = true;
    } else {
      node = this.getRepObject(aEvent.target);
    }

    if (node) {
      if (hitTwisty) {
        this.ioBox.toggleObject(node);
      } else {
        if (this.IUI.inspecting) {
          this.IUI.stopInspecting(true);
        } else {
          this.IUI.select(node, true, false);
          this.IUI.highlighter.highlightNode(node);
        }
      }
    }
  },

  





  onTreeDblClick: function TP_onTreeDblClick(aEvent)
  {
    
    
    if (this.editingContext)
      this.closeEditor();

    let target = aEvent.target;

    if (this.hasClass(target, "nodeValue")) {
      let repObj = this.getRepObject(target);
      let attrName = target.getAttribute("data-attributeName");
      let attrVal = target.innerHTML;

      this.editAttributeValue(target, repObj, attrName, attrVal);
    }
  },

  










  editAttributeValue:
  function TP_editAttributeValue(aAttrObj, aRepObj, aAttrName, aAttrVal)
  {
    let editor = this.treeBrowserDocument.getElementById("attribute-editor");
    let editorInput =
      this.treeBrowserDocument.getElementById("attribute-editor-input");
    let attrDims = aAttrObj.getBoundingClientRect();
    
    let viewportWidth = this.treeBrowserDocument.documentElement.clientWidth;
    let viewportHeight = this.treeBrowserDocument.documentElement.clientHeight;

    
    this.editingContext = {
      attrObj: aAttrObj,
      repObj: aRepObj,
      attrName: aAttrName
    };

    
    this.addClass(aAttrObj, "editingAttributeValue");

    
    this.addClass(editor, "editing");

    
    let editorVeritcalOffset = 2;

    
    let editorViewportBoundary = 5;

    
    editorInput.style.width = Math.min(attrDims.width, viewportWidth -
                                editorViewportBoundary) + "px";
    editorInput.style.height = Math.min(attrDims.height, viewportHeight -
                                editorViewportBoundary) + "px";
    let editorDims = editor.getBoundingClientRect();

    
    let editorLeft = attrDims.left + this.treeIFrame.contentWindow.scrollX -
                    
                    ((editorDims.width - attrDims.width) / 2);
    let editorTop = attrDims.top + this.treeIFrame.contentWindow.scrollY +
                    attrDims.height + editorVeritcalOffset;

    
    editorLeft = Math.max(0, Math.min(
                                      (this.treeIFrame.contentWindow.scrollX +
                                          viewportWidth - editorDims.width),
                                      editorLeft)
                          );
    editorTop = Math.max(0, Math.min(
                                      (this.treeIFrame.contentWindow.scrollY +
                                          viewportHeight - editorDims.height),
                                      editorTop)
                          );

    
    editor.style.left = editorLeft + "px";
    editor.style.top = editorTop + "px";

    
    editorInput.value = aAttrVal;
    editorInput.select();

    
    this.bindEditorEvent(editor, "click", function(aEvent) {
      aEvent.stopPropagation();
    });
    this.bindEditorEvent(editor, "dblclick", function(aEvent) {
      aEvent.stopPropagation();
    });
    this.bindEditorEvent(editor, "keypress",
                          this.handleEditorKeypress.bind(this));

    
    Services.obs.notifyObservers(null, this.window.INSPECTOR_NOTIFICATIONS.EDITOR_OPENED,
                                  null);
  },

  










  bindEditorEvent:
  function TP_bindEditorEvent(aEditor, aEventName, aEventCallback)
  {
    this.editingEvents[aEventName] = aEventCallback;
    aEditor.addEventListener(aEventName, aEventCallback, false);
  },

  







  unbindEditorEvent: function TP_unbindEditorEvent(aEditor, aEventName)
  {
    aEditor.removeEventListener(aEventName, this.editingEvents[aEventName],
                                  false);
    this.editingEvents[aEventName] = null;
  },

  




  handleEditorKeypress: function TP_handleEditorKeypress(aEvent)
  {
    if (aEvent.which == this.window.KeyEvent.DOM_VK_RETURN) {
      this.saveEditor();
    } else if (aEvent.keyCode == this.window.KeyEvent.DOM_VK_ESCAPE) {
      this.closeEditor();
    }
  },

  


  closeEditor: function TP_closeEditor()
  {
    let editor = this.treeBrowserDocument.getElementById("attribute-editor");
    let editorInput =
      this.treeBrowserDocument.getElementById("attribute-editor-input");

    
    this.removeClass(this.editingContext.attrObj, "editingAttributeValue");

    
    this.removeClass(editor, "editing");

    
    this.unbindEditorEvent(editor, "click");
    this.unbindEditorEvent(editor, "dblclick");
    this.unbindEditorEvent(editor, "keypress");

    
    editorInput.value = "";
    editorInput.blur();
    this.editingContext = null;
    this.editingEvents = {};

    
    Services.obs.notifyObservers(null, this.window.INSPECTOR_NOTIFICATIONS.EDITOR_CLOSED,
                                  null);
  },

  


  saveEditor: function TP_saveEditor()
  {
    let editorInput =
      this.treeBrowserDocument.getElementById("attribute-editor-input");

    
    this.editingContext.repObj.setAttribute(this.editingContext.attrName,
                                              editorInput.value);

    
    this.editingContext.attrObj.innerHTML = editorInput.value;

    this.IUI.isDirty = true;

    
    Services.obs.notifyObservers(null, this.window.INSPECTOR_NOTIFICATIONS.EDITOR_SAVED,
                                  null);

    this.closeEditor();
  },

  




  select: function TP_select(aNode, aScroll)
  {
    if (this.ioBox)
      this.ioBox.select(aNode, true, true, aScroll);
  },

  
  

  







  hasClass: function TP_hasClass(aNode, aClass)
  {
    if (!(aNode instanceof this.window.Element))
      return false;
    return aNode.classList.contains(aClass);
  },

  






  addClass: function TP_addClass(aNode, aClass)
  {
    if (aNode instanceof this.window.Element)
      aNode.classList.add(aClass);
  },

  






  removeClass: function TP_removeClass(aNode, aClass)
  {
    if (aNode instanceof this.window.Element)
      aNode.classList.remove(aClass);
  },

  








  getRepObject: function TP_getRepObject(element)
  {
    let target = null;
    for (let child = element; child; child = child.parentNode) {
      if (this.hasClass(child, "repTarget"))
        target = child;

      if (child.repObject) {
        if (!target && this.hasClass(child.repObject, "repIgnore"))
          break;
        else
          return child.repObject;
      }
    }
    return null;
  },

  


  destroy: function TP_destroy()
  {
    if (this.isOpen()) {
      this.close();
    }

    domplateUtils.setDOM(null);

    delete this.treeWalker;

    if (this.treePanelDiv) {
      this.treePanelDiv.ownerPanel = null;
      let parent = this.treePanelDiv.parentNode;
      parent.removeChild(this.treePanelDiv);
      delete this.treePanelDiv;
      delete this.treeBrowserDocument;
    }

    if (this.treeIFrame) {
      this.treeIFrame.removeEventListener("dblclick", this.onTreeDblClick, false);
      this.treeIFrame.removeEventListener("click", this.onTreeClick, false);
      let parent = this.treeIFrame.parentNode;
      parent.removeChild(this.treeIFrame);
      delete this.treeIFrame;
    }

    if (this.ioBox) {
      this.ioBox.destroy();
      delete this.ioBox;
    }

    this.container.removeEventListener("popuphiding", this._boundClose, false);
    delete this._boundClose;
  }
};

