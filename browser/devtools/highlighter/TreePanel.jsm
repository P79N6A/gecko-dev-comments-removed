











































const Cu = Components.utils;

Cu.import("resource:///modules/domplate.jsm");
Cu.import("resource:///modules/InsideOutBox.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/inspector.jsm");

var EXPORTED_SYMBOLS = ["TreePanel", "DOMHelpers"];

const INSPECTOR_URI = "chrome://browser/content/inspector.html";







function TreePanel(aContext, aIUI) {
  this._init(aContext, aIUI);
};

TreePanel.prototype = {
  showTextNodesWithWhitespace: false,
  id: "treepanel", 
  _open: false,

  






  get container()
  {
    return this.document.getElementById("inspector-tree-box");
  },

  





  _init: function TP__init(aContext, aIUI)
  {
    this.IUI = aIUI;
    this.window = aContext;
    this.document = this.window.document;
    this.button =
     this.IUI.chromeDoc.getElementById("inspector-treepanel-toolbutton");

    domplateUtils.setDOM(this.window);

    this.DOMHelpers = new DOMHelpers(this.window);

    let isOpen = this.isOpen.bind(this);

    this.editingEvents = {};
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
    this.treeIFrame.focus();
    delete this.initializingTreePanel;
    Services.obs.notifyObservers(null,
      this.IUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY, null);
    if (this.pendingSelection) {
      this.select(this.pendingSelection.node, this.pendingSelection.scroll);
      delete this.pendingSelection;
    }
  },

  


  open: function TP_open()
  {
    if (this._open) {
      return;
    }

    this._open = true;

    this.button.setAttribute("checked", true);
    this.initializingTreePanel = true;

    this.treeIFrame = this.document.getElementById("inspector-tree-iframe");
    if (!this.treeIFrame) {
      this.treeIFrame = this.document.createElement("iframe");
      this.treeIFrame.setAttribute("id", "inspector-tree-iframe");
      this.treeIFrame.flex = 1;
      this.treeIFrame.setAttribute("type", "content");
      this.treeIFrame.setAttribute("context", "inspector-node-popup");
    }

    let treeBox = null;
    treeBox = this.document.createElement("vbox");
    treeBox.id = "inspector-tree-box";
    treeBox.state = "open";
    try {
      treeBox.height =
        Services.prefs.getIntPref("devtools.inspector.htmlHeight");
    } catch(e) {
      treeBox.height = 112;
    }

    treeBox.minHeight = 64;

    this.splitter = this.document.createElement("splitter");
    this.splitter.id = "inspector-tree-splitter";

    let container = this.document.getElementById("appcontent");
    container.appendChild(this.splitter);
    container.appendChild(treeBox);

    treeBox.appendChild(this.treeIFrame);

    this._boundLoadedInitializeTreePanel = function loadedInitializeTreePanel()
    {
      this.treeIFrame.removeEventListener("load",
        this._boundLoadedInitializeTreePanel, true);
      delete this._boundLoadedInitializeTreePanel;
      this.initializeIFrame();
    }.bind(this);

    this.treeIFrame.addEventListener("load",
      this._boundLoadedInitializeTreePanel, true);

    let src = this.treeIFrame.getAttribute("src");
    if (src != INSPECTOR_URI) {
      this.treeIFrame.setAttribute("src", INSPECTOR_URI);
    } else {
      this.treeIFrame.contentWindow.location.reload();
    }
  },

  


  close: function TP_close()
  {
    this._open = false;

    
    if (this._boundLoadedInitializeTreePanel) {
      this.treeIFrame.removeEventListener("load",
        this._boundLoadedInitializeTreePanel, true);
      delete this._boundLoadedInitializeTreePanel;
    }

    this.button.removeAttribute("checked");
    let treeBox = this.container;
    Services.prefs.setIntPref("devtools.inspector.htmlHeight", treeBox.height);
    let treeBoxParent = treeBox.parentNode;
    treeBoxParent.removeChild(this.splitter);
    treeBoxParent.removeChild(treeBox);

    if (this.treePanelDiv) {
      this.treePanelDiv.ownerPanel = null;
      let parent = this.treePanelDiv.parentNode;
      parent.removeChild(this.treePanelDiv);
      delete this.treePanelDiv;
      delete this.treeBrowserDocument;
    }

    if (this.ioBox) {
      this.ioBox.destroy();
      delete this.ioBox;
    }

    this.treeLoaded = false;
  },

  



  isOpen: function TP_isOpen()
  {
    return this._open;
  },

  


  toggle: function TP_toggle()
  {
    this.isOpen() ? this.close() : this.open();
  },

  





  createObjectBox: function TP_createObjectBox(object, isRoot)
  {
    let tag = domplateUtils.getNodeTag(object);
    if (tag)
      return tag.replace({object: object}, this.treeBrowserDocument);
  },

  getParentObject: function TP_getParentObject(node)
  {
    return this.DOMHelpers.getParentObject(node);
  },

  getChildObject: function TP_getChildObject(node, index, previousSibling)
  {
    return this.DOMHelpers.getChildObject(node, index, previousSibling,
                                        this.showTextNodesWithWhitespace);
  },

  getFirstChild: function TP_getFirstChild(node)
  {
    return this.DOMHelpers.getFirstChild(node);
  },

  getNextSibling: function TP_getNextSibling(node)
  {
    return this.DOMHelpers.getNextSibling(node);
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
          this.IUI.highlighter.highlight(node);
        }
      }
    }
  },

  





  onTreeDblClick: function TP_onTreeDblClick(aEvent)
  {
    
    
    if (this.editingContext)
      this.closeEditor();

    let target = aEvent.target;

    if (!this.hasClass(target, "editable")) {
      return;
    }

    let repObj = this.getRepObject(target);

    if (this.hasClass(target, "nodeValue")) {
      let attrName = target.getAttribute("data-attributeName");
      let attrVal = target.innerHTML;

      this.editAttribute(target, repObj, attrName, attrVal);
    }

    if (this.hasClass(target, "nodeName")) {
      let attrName = target.innerHTML;
      let attrValNode = target.nextSibling.nextSibling; 

      if (attrValNode)
        this.editAttribute(target, repObj, attrName, attrValNode.innerHTML);
    }
  },

  











  editAttribute:
  function TP_editAttribute(aAttrObj, aRepObj, aAttrName, aAttrVal)
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
      attrName: aAttrName,
      attrValue: aAttrVal
    };

    
    this.addClass(aAttrObj, "editingAttributeValue");

    
    this.addClass(editor, "editing");

    
    let editorVerticalOffset = 2;

    
    let editorViewportBoundary = 5;

    
    editorInput.style.width = Math.min(attrDims.width, viewportWidth -
                                editorViewportBoundary) + "px";
    editorInput.style.height = Math.min(attrDims.height, viewportHeight -
                                editorViewportBoundary) + "px";
    let editorDims = editor.getBoundingClientRect();

    
    let editorLeft = attrDims.left + this.treeIFrame.contentWindow.scrollX -
                    
                    ((editorDims.width - attrDims.width) / 2);
    let editorTop = attrDims.top + this.treeIFrame.contentWindow.scrollY +
                    attrDims.height + editorVerticalOffset;

    
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

    
    if (this.hasClass(aAttrObj, "nodeValue")) {
      editorInput.value = aAttrVal;
      editorInput.select();
    } else {
      editorInput.value = aAttrName;
      editorInput.select();
    }

    
    this.bindEditorEvent(editor, "click", function(aEvent) {
      aEvent.stopPropagation();
    });
    this.bindEditorEvent(editor, "dblclick", function(aEvent) {
      aEvent.stopPropagation();
    });
    this.bindEditorEvent(editor, "keypress",
                          this.handleEditorKeypress.bind(this));

    
    Services.obs.notifyObservers(null, this.IUI.INSPECTOR_NOTIFICATIONS.EDITOR_OPENED,
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
      aEvent.preventDefault();
      aEvent.stopPropagation();
    } else if (aEvent.keyCode == this.window.KeyEvent.DOM_VK_ESCAPE) {
      this.closeEditor();
      aEvent.preventDefault();
      aEvent.stopPropagation();
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

    
    Services.obs.notifyObservers(null, this.IUI.INSPECTOR_NOTIFICATIONS.EDITOR_CLOSED,
                                  null);
  },

  


  saveEditor: function TP_saveEditor()
  {
    let editorInput =
      this.treeBrowserDocument.getElementById("attribute-editor-input");
    let dirty = false;

    if (this.hasClass(this.editingContext.attrObj, "nodeValue")) {
      
      this.editingContext.repObj.setAttribute(this.editingContext.attrName,
                                                editorInput.value);

      
      this.editingContext.attrObj.innerHTML = editorInput.value;
      dirty = true;
    }

    if (this.hasClass(this.editingContext.attrObj, "nodeName")) {
      
      this.editingContext.repObj.removeAttribute(this.editingContext.attrName);

      
      this.editingContext.repObj.setAttribute(editorInput.value,
                                              this.editingContext.attrValue);

      
      this.editingContext.attrObj.innerHTML = editorInput.value;
      dirty = true;
    }

    this.IUI.isDirty = dirty;
    this.IUI.nodeChanged(this.registrationObject);

    
    Services.obs.notifyObservers(null, this.IUI.INSPECTOR_NOTIFICATIONS.EDITOR_SAVED,
                                  null);

    this.closeEditor();
  },

  




  select: function TP_select(aNode, aScroll)
  {
    if (this.ioBox) {
      this.ioBox.select(aNode, true, true, aScroll);
    } else {
      this.pendingSelection = { node: aNode, scroll: aScroll };
    }
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

  




  deleteChildBox: function TP_deleteChildBox(aElement)
  {
    let childBox = this.ioBox.findObjectBox(aElement);
    if (!childBox) {
      return;
    }
    childBox.parentNode.removeChild(childBox);
  },

  


  destroy: function TP_destroy()
  {
    if (this.isOpen()) {
      this.close();
    }

    domplateUtils.setDOM(null);

    if (this.DOMHelpers) {
      this.DOMHelpers.destroy();
      delete this.DOMHelpers;
    }

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
  }
};










function DOMHelpers(aWindow) {
  this.window = aWindow;
};

DOMHelpers.prototype = {
  getParentObject: function Helpers_getParentObject(node)
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

  getChildObject: function Helpers_getChildObject(node, index, previousSibling,
                                                showTextNodesWithWhitespace)
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

    if (showTextNodesWithWhitespace)
      return child;

    for (; child; child = this.getNextSibling(child)) {
      if (!this.isWhitespaceText(child))
        return child;
    }

    return null;  
  },

  getFirstChild: function Helpers_getFirstChild(node)
  {
    let SHOW_ALL = Components.interfaces.nsIDOMNodeFilter.SHOW_ALL;
    this.treeWalker = node.ownerDocument.createTreeWalker(node,
      SHOW_ALL, null, false);
    return this.treeWalker.firstChild();
  },

  getNextSibling: function Helpers_getNextSibling(node)
  {
    let next = this.treeWalker.nextSibling();

    if (!next)
      delete this.treeWalker;

    return next;
  },

  isWhitespaceText: function Helpers_isWhitespaceText(node)
  {
    return node.nodeType == this.window.Node.TEXT_NODE &&
                            !/[^\s]/.exec(node.nodeValue);
  },

  destroy: function Helpers_destroy()
  {
    delete this.window;
    delete this.treeWalker;
  }
};
