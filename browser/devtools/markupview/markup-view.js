





const {Cc, Cu, Ci} = require("chrome");


const PAGE_SIZE = 10;
const PREVIEW_AREA = 700;
const DEFAULT_MAX_CHILDREN = 100;
const COLLAPSE_ATTRIBUTE_LENGTH = 120;
const COLLAPSE_DATA_URL_REGEX = /^data.+base64/;
const COLLAPSE_DATA_URL_LENGTH = 60;
const CONTAINER_FLASHING_DURATION = 500;
const IMAGE_PREVIEW_MAX_DIM = 400;

const {UndoStack} = require("devtools/shared/undo");
const {editableField, InplaceEditor} = require("devtools/shared/inplace-editor");
const {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
const {HTMLEditor} = require("devtools/markupview/html-editor");
const {OutputParser} = require("devtools/output-parser");
const promise = require("sdk/core/promise");
const {Tooltip} = require("devtools/shared/widgets/Tooltip");

Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/devtools/Templater.jsm");
Cu.import("resource://gre/modules/Services.jsm");

loader.lazyGetter(this, "DOMParser", function() {
 return Cc["@mozilla.org/xmlextras/domparser;1"].createInstance(Ci.nsIDOMParser);
});
loader.lazyGetter(this, "AutocompletePopup", () => {
  return require("devtools/shared/autocomplete-popup").AutocompletePopup
});



















function MarkupView(aInspector, aFrame, aControllerWindow) {
  this._inspector = aInspector;
  this.walker = this._inspector.walker;
  this._frame = aFrame;
  this.doc = this._frame.contentDocument;
  this._elt = this.doc.querySelector("#root");
  this._outputParser = new OutputParser();
  this.htmlEditor = new HTMLEditor(this.doc);

  this.layoutHelpers = new LayoutHelpers(this.doc.defaultView);

  try {
    this.maxChildren = Services.prefs.getIntPref("devtools.markup.pagesize");
  } catch(ex) {
    this.maxChildren = DEFAULT_MAX_CHILDREN;
  }

  
  let options = {
    fixedWidth: true,
    autoSelect: true,
    theme: "auto"
  };
  this.popup = new AutocompletePopup(this.doc.defaultView.parent.document, options);

  this.undo = new UndoStack();
  this.undo.installController(aControllerWindow);

  this._containers = new Map();

  this._boundMutationObserver = this._mutationObserver.bind(this);
  this.walker.on("mutations", this._boundMutationObserver);

  this._boundOnNewSelection = this._onNewSelection.bind(this);
  this._inspector.selection.on("new-node-front", this._boundOnNewSelection);
  this._onNewSelection();

  this._boundKeyDown = this._onKeyDown.bind(this);
  this._frame.contentWindow.addEventListener("keydown", this._boundKeyDown, false);

  this._boundFocus = this._onFocus.bind(this);
  this._frame.addEventListener("focus", this._boundFocus, false);

  this._handlePrefChange = this._handlePrefChange.bind(this);
  gDevTools.on("pref-changed", this._handlePrefChange);

  this._initPreview();

  this.tooltip = new Tooltip(this._inspector.panelDoc);
  this.tooltip.startTogglingOnHover(this._elt,
    this._buildTooltipContent.bind(this));
}

exports.MarkupView = MarkupView;

MarkupView.prototype = {
  _selectedContainer: null,

  template: function(aName, aDest, aOptions={stack: "markup-view.xhtml"}) {
    let node = this.doc.getElementById("template-" + aName).cloneNode(true);
    node.removeAttribute("id");
    template(node, aDest, aOptions);
    return node;
  },

  



  getContainer: function(aNode) {
    return this._containers.get(aNode);
  },

  _handlePrefChange: function(event, data) {
    if (data.pref == "devtools.defaultColorUnit") {
      this.update();
    }
  },

  update: function() {
    let updateChildren = function(node) {
      this.getContainer(node).update();
      for (let child of node.treeChildren()) {
        updateChildren(child);
      }
    }.bind(this);

    
    let documentElement;
    for (let node of this._rootNode.treeChildren()) {
      if (node.isDocumentElement === true) {
        documentElement = node;
        break;
      }
    }

    
    updateChildren(documentElement);
  },

  _buildTooltipContent: function(target) {
    
    
    let parent = target, container;
    while (parent !== this.doc.body) {
      if (parent.container) {
        container = parent.container;
        break;
      }
      parent = parent.parentNode;
    }

    if (container) {
      
      
      return container._buildTooltipContent(target, this.tooltip);
    }
  },

  


  _onNewSelection: function() {
    this.htmlEditor.hide();
    let done = this._inspector.updating("markup-view");
    if (this._inspector.selection.isNode()) {
      this.showNode(this._inspector.selection.nodeFront, true).then(() => {
        this.markNodeAsSelected(this._inspector.selection.nodeFront);
        done();
      });
    } else {
      this.unmarkSelectedNode();
      done();
    }
  },

  



  _selectionWalker: function(aStart) {
    let walker = this.doc.createTreeWalker(
      aStart || this._elt,
      Ci.nsIDOMNodeFilter.SHOW_ELEMENT,
      function(aElement) {
        if (aElement.container &&
            aElement.container.elt === aElement &&
            aElement.container.visible) {
          return Ci.nsIDOMNodeFilter.FILTER_ACCEPT;
        }
        return Ci.nsIDOMNodeFilter.FILTER_SKIP;
      }
    );
    walker.currentNode = this._selectedContainer.elt;
    return walker;
  },

  


  _onKeyDown: function(aEvent) {
    let handled = true;

    
    if (aEvent.target.tagName.toLowerCase() === "input" ||
        aEvent.target.tagName.toLowerCase() === "textarea") {
      return;
    }

    switch(aEvent.keyCode) {
      case Ci.nsIDOMKeyEvent.DOM_VK_H:
        let node = this._selectedContainer.node;
        if (node.hidden) {
          this.walker.unhideNode(node).then(() => this.nodeChanged(node));
        } else {
          this.walker.hideNode(node).then(() => this.nodeChanged(node));
        }
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_DELETE:
      case Ci.nsIDOMKeyEvent.DOM_VK_BACK_SPACE:
        this.deleteNode(this._selectedContainer.node);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_HOME:
        let rootContainer = this._containers.get(this._rootNode);
        this.navigate(rootContainer.children.firstChild.container);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_LEFT:
        if (this._selectedContainer.expanded) {
          this.collapseNode(this._selectedContainer.node);
        } else {
          let parent = this._selectionWalker().parentNode();
          if (parent) {
            this.navigate(parent.container);
          }
        }
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_RIGHT:
        if (!this._selectedContainer.expanded &&
            this._selectedContainer.hasChildren) {
          this._expandContainer(this._selectedContainer);
        } else {
          let next = this._selectionWalker().nextNode();
          if (next) {
            this.navigate(next.container);
          }
        }
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_UP:
        let prev = this._selectionWalker().previousNode();
        if (prev) {
          this.navigate(prev.container);
        }
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_DOWN:
        let next = this._selectionWalker().nextNode();
        if (next) {
          this.navigate(next.container);
        }
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP: {
        let walker = this._selectionWalker();
        let selection = this._selectedContainer;
        for (let i = 0; i < PAGE_SIZE; i++) {
          let prev = walker.previousNode();
          if (!prev) {
            break;
          }
          selection = prev.container;
        }
        this.navigate(selection);
        break;
      }
      case Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN: {
        let walker = this._selectionWalker();
        let selection = this._selectedContainer;
        for (let i = 0; i < PAGE_SIZE; i++) {
          let next = walker.nextNode();
          if (!next) {
            break;
          }
          selection = next.container;
        }
        this.navigate(selection);
        break;
      }
      default:
        handled = false;
    }
    if (handled) {
      aEvent.stopPropagation();
      aEvent.preventDefault();
    }
  },

  



  deleteNode: function(aNode) {
    if (aNode.isDocumentElement ||
        aNode.nodeType == Ci.nsIDOMNode.DOCUMENT_TYPE_NODE) {
      return;
    }

    let container = this._containers.get(aNode);

    
    this.walker.retainNode(aNode).then(() => {
      let parent = aNode.parentNode();
      let sibling = null;
      this.undo.do(() => {
        if (container.selected) {
          this.navigate(this._containers.get(parent));
        }
        this.walker.removeNode(aNode).then(nextSibling => {
          sibling = nextSibling;
        });
      }, () => {
        this.walker.insertBefore(aNode, parent, sibling);
      });
    }).then(null, console.error);
  },

  


  _onFocus: function(aEvent) {
    let parent = aEvent.target;
    while (!parent.container) {
      parent = parent.parentNode;
    }
    if (parent) {
      this.navigate(parent.container, true);
    }
  },

  








  navigate: function(aContainer, aIgnoreFocus) {
    if (!aContainer) {
      return;
    }

    let node = aContainer.node;
    this.markNodeAsSelected(node, "treepanel");

    
    
    this._inspector.selection.emit("new-node");
    this._inspector.selection.emit("new-node-front");

    if (!aIgnoreFocus) {
      aContainer.focus();
    }
  },

  








  importNode: function(aNode, aFlashNode) {
    if (!aNode) {
      return null;
    }

    if (this._containers.has(aNode)) {
      return this._containers.get(aNode);
    }

    if (aNode === this.walker.rootNode) {
      var container = new RootContainer(this, aNode);
      this._elt.appendChild(container.elt);
      this._rootNode = aNode;
    } else {
      var container = new MarkupContainer(this, aNode, this._inspector);
      if (aFlashNode) {
        container.flashMutation();
      }
    }

    this._containers.set(aNode, container);
    container.childrenDirty = true;

    this._updateChildren(container);

    return container;
  },

  


  _mutationObserver: function(aMutations) {
    let requiresLayoutChange = false;
    let reselectParent;
    let reselectChildIndex;

    for (let mutation of aMutations) {
      let type = mutation.type;
      let target = mutation.target;

      if (mutation.type === "documentUnload") {
        
        
        type = "childList";
        target = mutation.targetParent;
        if (!target) {
          continue;
        }
      }

      let container = this._containers.get(target);
      if (!container) {
        
        
        continue;
      }
      if (type === "attributes" || type === "characterData") {
        container.update();

        
        if (type === "attributes" && container.selected) {
          requiresLayoutChange = true;
        }
      } else if (type === "childList") {
        let isFromOuterHTML = mutation.removed.some((n) => {
          return n === this._outerHTMLNode;
        });

        
        if (isFromOuterHTML) {
          reselectParent = target;
          reselectChildIndex = this._outerHTMLChildIndex;

          delete this._outerHTMLNode;
          delete this._outerHTMLChildIndex;
        }

        container.childrenDirty = true;
        
        this._updateChildren(container, {flash: !isFromOuterHTML});
      }
    }

    if (requiresLayoutChange) {
      this._inspector.immediateLayoutChange();
    }
    this._waitForChildren().then((nodes) => {
      this._flashMutatedNodes(aMutations);
      this._inspector.emit("markupmutation", aMutations);

      
      
      this.htmlEditor.refresh();

      
      
      if (this._inspector.selection.nodeFront === reselectParent) {
        this.walker.children(reselectParent).then((o) => {
          let node = o.nodes[reselectChildIndex];
          let container = this._containers.get(node);
          if (node && container) {
            this.markNodeAsSelected(node, "outerhtml");
            if (container.hasChildren) {
              this.expandNode(node);
            }
          }
        });

      }
    });
  },

  



  _flashMutatedNodes: function(aMutations) {
    let addedOrEditedContainers = new Set();
    let removedContainers = new Set();

    for (let {type, target, added, removed} of aMutations) {
      let container = this._containers.get(target);

      if (container) {
        if (type === "attributes" || type === "characterData") {
          addedOrEditedContainers.add(container);
        } else if (type === "childList") {
          
          if (removed.length) {
            removedContainers.add(container);
          }

          
          added.forEach(added => {
            let addedContainer = this._containers.get(added);
            addedOrEditedContainers.add(addedContainer);

            
            
            
            
            removedContainers.delete(container);
          });
        }
      }
    }

    for (let container of removedContainers) {
      container.flashMutation();
    }
    for (let container of addedOrEditedContainers) {
      container.flashMutation();
    }
  },

  



  showNode: function(aNode, centered) {
    let container = this.importNode(aNode);
    let parent = aNode;
    while ((parent = parent.parentNode())) {
      this.importNode(parent);
      this.expandNode(parent);
    }

    return this._waitForChildren().then(() => {
      return this._ensureVisible(aNode);
    }).then(() => {
      
      this.layoutHelpers.scrollIntoViewIfNeeded(this._containers.get(aNode).editor.elt, centered);
    });
  },

  


  _expandContainer: function(aContainer) {
    return this._updateChildren(aContainer, {expand: true}).then(() => {
      aContainer.expanded = true;
    });
  },

  


  expandNode: function(aNode) {
    let container = this._containers.get(aNode);
    this._expandContainer(container);
  },

  




  _expandAll: function(aContainer) {
    return this._expandContainer(aContainer).then(() => {
      let child = aContainer.children.firstChild;
      let promises = [];
      while (child) {
        promises.push(this._expandAll(child.container));
        child = child.nextSibling;
      }
      return promise.all(promises);
    }).then(null, console.error);
  },

  





  expandAll: function(aNode) {
    aNode = aNode || this._rootNode;
    return this._expandAll(this._containers.get(aNode));
  },

  


  collapseNode: function(aNode) {
    let container = this._containers.get(aNode);
    container.expanded = false;
  },

  




  getNodeOuterHTML: function(aNode) {
    let def = promise.defer();
    this.walker.outerHTML(aNode).then(longstr => {
      longstr.string().then(outerHTML => {
        longstr.release().then(null, console.error);
        def.resolve(outerHTML);
      });
    });
    return def.promise;
  },

  





  getNodeChildIndex: function(aNode) {
    let def = promise.defer();
    let parentNode = aNode.parentNode();

    
    
    if (!parentNode) {
      def.resolve(-1);
    } else {
      this.walker.children(parentNode).then(children => {
        def.resolve(children.nodes.indexOf(aNode));
      });
    }

    return def.promise;
  },

  







  updateNodeOuterHTML: function(aNode, newValue, oldValue) {
    let container = this._containers.get(aNode);
    if (!container) {
      return;
    }

    this.getNodeChildIndex(aNode).then((i) => {
      this._outerHTMLChildIndex = i;
      this._outerHTMLNode = aNode;

      container.undo.do(() => {
        this.walker.setOuterHTML(aNode, newValue);
      }, () => {
        this.walker.setOuterHTML(aNode, oldValue);
      });
    });
  },

  



  beginEditingOuterHTML: function(aNode) {
    this.getNodeOuterHTML(aNode).then((oldValue)=> {
      let container = this._containers.get(aNode);
      if (!container) {
        return;
      }
      this.htmlEditor.show(container.tagLine, oldValue);
      this.htmlEditor.once("popup-hidden", (e, aCommit, aValue) => {
        if (aCommit) {
          this.updateNodeOuterHTML(aNode, aValue, oldValue);
        }
      });
    });
  },

  



  setNodeExpanded: function(aNode, aExpanded) {
    if (aExpanded) {
      this.expandNode(aNode);
    } else {
      this.collapseNode(aNode);
    }
  },

  




  markNodeAsSelected: function(aNode, reason) {
    let container = this._containers.get(aNode);
    if (this._selectedContainer === container) {
      return false;
    }
    if (this._selectedContainer) {
      this._selectedContainer.selected = false;
    }
    this._selectedContainer = container;
    if (aNode) {
      this._selectedContainer.selected = true;
    }

    this._inspector.selection.setNodeFront(aNode, reason || "nodeselected");
    return true;
  },

  



  _ensureVisible: function(node) {
    while (node) {
      let container = this._containers.get(node);
      let parent = node.parentNode();
      if (!container.elt.parentNode) {
        let parentContainer = this._containers.get(parent);
        parentContainer.childrenDirty = true;
        this._updateChildren(parentContainer, {expand: node});
      }

      node = parent;
    }
    return this._waitForChildren();
  },

  


  unmarkSelectedNode: function() {
    if (this._selectedContainer) {
      this._selectedContainer.selected = false;
      this._selectedContainer = null;
    }
  },

  


  nodeChanged: function(aNode) {
    if (aNode === this._inspector.selection.nodeFront) {
      this._inspector.change("markupview");
    }
  },

  





  _checkSelectionVisible: function(aContainer) {
    let centered = null;
    let node = this._inspector.selection.nodeFront;
    while (node) {
      if (node.parentNode() === aContainer.node) {
        centered = node;
        break;
      }
      node = node.parentNode();
    }

    return centered;
  },

  




















  _updateChildren: function(aContainer, options) {
    let expand = options && options.expand;
    let flash = options && options.flash;

    aContainer.hasChildren = aContainer.node.hasChildren;

    if (!this._queuedChildUpdates) {
      this._queuedChildUpdates = new Map();
    }

    if (this._queuedChildUpdates.has(aContainer)) {
      return this._queuedChildUpdates.get(aContainer);
    }

    if (!aContainer.childrenDirty) {
      return promise.resolve(aContainer);
    }

    if (!aContainer.hasChildren) {
      while (aContainer.children.firstChild) {
        aContainer.children.removeChild(aContainer.children.firstChild);
      }
      aContainer.childrenDirty = false;
      return promise.resolve(aContainer);
    }

    
    
    
    if (!(aContainer.expanded || expand)) {
      return promise.resolve(aContainer);
    }

    
    
    let centered = this._checkSelectionVisible(aContainer);

    
    
    
    aContainer.childrenDirty = false;
    let updatePromise = this._getVisibleChildren(aContainer, centered).then(children => {
      if (!this._containers) {
        return promise.reject("markup view destroyed");
      }
      this._queuedChildUpdates.delete(aContainer);

      
      
      if (aContainer.childrenDirty) {
        return this._updateChildren(aContainer, {expand: centered});
      }

      let fragment = this.doc.createDocumentFragment();

      for (let child of children.nodes) {
        let container = this.importNode(child, flash);
        fragment.appendChild(container.elt);
      }

      while (aContainer.children.firstChild) {
        aContainer.children.removeChild(aContainer.children.firstChild);
      }

      if (!(children.hasFirst && children.hasLast)) {
        let data = {
          showing: this.strings.GetStringFromName("markupView.more.showing"),
          showAll: this.strings.formatStringFromName(
                    "markupView.more.showAll",
                    [aContainer.node.numChildren.toString()], 1),
          allButtonClick: () => {
            aContainer.maxChildren = -1;
            aContainer.childrenDirty = true;
            this._updateChildren(aContainer);
          }
        };

        if (!children.hasFirst) {
          let span = this.template("more-nodes", data);
          fragment.insertBefore(span, fragment.firstChild);
        }
        if (!children.hasLast) {
          let span = this.template("more-nodes", data);
          fragment.appendChild(span);
        }
      }

      aContainer.children.appendChild(fragment);
      return aContainer;
    }).then(null, console.error);
    this._queuedChildUpdates.set(aContainer, updatePromise);
    return updatePromise;
  },

  _waitForChildren: function() {
    if (!this._queuedChildUpdates) {
      return promise.resolve(undefined);
    }
    return promise.all([updatePromise for (updatePromise of this._queuedChildUpdates.values())]);
  },

  


  _getVisibleChildren: function(aContainer, aCentered) {
    let maxChildren = aContainer.maxChildren || this.maxChildren;
    if (maxChildren == -1) {
      maxChildren = undefined;
    }

    return this.walker.children(aContainer.node, {
      maxNodes: maxChildren,
      center: aCentered
    });
  },

  


  destroy: function() {
    gDevTools.off("pref-changed", this._handlePrefChange);

    this.htmlEditor.destroy();
    delete this.htmlEditor;

    this.undo.destroy();
    delete this.undo;

    this.popup.destroy();
    delete this.popup;

    this._frame.removeEventListener("focus", this._boundFocus, false);
    delete this._boundFocus;

    delete this._outputParser;

    if (this._boundUpdatePreview) {
      this._frame.contentWindow.removeEventListener("scroll",
        this._boundUpdatePreview, true);
      delete this._boundUpdatePreview;
    }

    if (this._boundResizePreview) {
      this._frame.contentWindow.removeEventListener("resize",
        this._boundResizePreview, true);
      this._frame.contentWindow.removeEventListener("overflow",
        this._boundResizePreview, true);
      this._frame.contentWindow.removeEventListener("underflow",
        this._boundResizePreview, true);
      delete this._boundResizePreview;
    }

    this._frame.contentWindow.removeEventListener("keydown",
      this._boundKeyDown, false);
    delete this._boundKeyDown;

    this._inspector.selection.off("new-node-front", this._boundOnNewSelection);
    delete this._boundOnNewSelection;

    this.walker.off("mutations", this._boundMutationObserver)
    delete this._boundMutationObserver;

    delete this._elt;

    for (let [key, container] of this._containers) {
      container.destroy();
    }
    delete this._containers;

    this.tooltip.destroy();
    delete this.tooltip;
  },

  


  _initPreview: function() {
    if (!Services.prefs.getBoolPref("devtools.inspector.markupPreview")) {
      return;
    }

    this._previewBar = this.doc.querySelector("#previewbar");
    this._preview = this.doc.querySelector("#preview");
    this._viewbox = this.doc.querySelector("#viewbox");

    this._previewBar.classList.remove("disabled");

    this._previewWidth = this._preview.getBoundingClientRect().width;

    this._boundResizePreview = this._resizePreview.bind(this);
    this._frame.contentWindow.addEventListener("resize",
      this._boundResizePreview, true);
    this._frame.contentWindow.addEventListener("overflow",
      this._boundResizePreview, true);
    this._frame.contentWindow.addEventListener("underflow",
      this._boundResizePreview, true);

    this._boundUpdatePreview = this._updatePreview.bind(this);
    this._frame.contentWindow.addEventListener("scroll",
      this._boundUpdatePreview, true);
    this._updatePreview();
  },

  


  _updatePreview: function() {
    let win = this._frame.contentWindow;

    if (win.scrollMaxY == 0) {
      this._previewBar.classList.add("disabled");
      return;
    }

    this._previewBar.classList.remove("disabled");

    let ratio = this._previewWidth / PREVIEW_AREA;
    let width = ratio * win.innerWidth;

    let height = ratio * (win.scrollMaxY + win.innerHeight);
    let scrollTo
    if (height >= win.innerHeight) {
      scrollTo = -(height - win.innerHeight) * (win.scrollY / win.scrollMaxY);
      this._previewBar.setAttribute("style", "height:" + height +
        "px;transform:translateY(" + scrollTo + "px)");
    } else {
      this._previewBar.setAttribute("style", "height:100%");
    }

    let bgSize = ~~width + "px " + ~~height + "px";
    this._preview.setAttribute("style", "background-size:" + bgSize);

    let height = ~~(win.innerHeight * ratio) + "px";
    let top = ~~(win.scrollY * ratio) + "px";
    this._viewbox.setAttribute("style", "height:" + height +
      ";transform: translateY(" + top + ")");
  },

  


  _resizePreview: function() {
    let win = this._frame.contentWindow;
    this._previewBar.classList.add("hide");
    win.clearTimeout(this._resizePreviewTimeout);

    win.setTimeout(function() {
      this._updatePreview();
      this._previewBar.classList.remove("hide");
    }.bind(this), 1000);
  }
};















function MarkupContainer(aMarkupView, aNode, aInspector) {
  this.markup = aMarkupView;
  this.doc = this.markup.doc;
  this.undo = this.markup.undo;
  this.node = aNode;
  this._inspector = aInspector;

  if (aNode.nodeType == Ci.nsIDOMNode.TEXT_NODE) {
    this.editor = new TextEditor(this, aNode, "text");
  } else if (aNode.nodeType == Ci.nsIDOMNode.COMMENT_NODE) {
    this.editor = new TextEditor(this, aNode, "comment");
  } else if (aNode.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
    this.editor = new ElementEditor(this, aNode);
  } else if (aNode.nodeType == Ci.nsIDOMNode.DOCUMENT_TYPE_NODE) {
    this.editor = new DoctypeEditor(this, aNode);
  } else {
    this.editor = new GenericEditor(this, aNode);
  }

  
  this.elt = null;
  this.expander = null;
  this.highlighter = null;
  this.tagLine = null;
  this.children = null;
  this.markup.template("container", this);
  this.elt.container = this;
  this.children.container = this;

  
  this._onToggle = this._onToggle.bind(this);
  this.elt.addEventListener("dblclick", this._onToggle, false);
  this.expander.addEventListener("click", this._onToggle, false);

  
  
  
  this._onMouseOver = this._onMouseOver.bind(this);
  this.elt.addEventListener("mouseover", this._onMouseOver, false);

  this._onMouseOut = this._onMouseOut.bind(this);
  this.elt.addEventListener("mouseout", this._onMouseOut, false);

  
  this.tagLine.appendChild(this.editor.elt);

  this._onMouseDown = this._onMouseDown.bind(this);
  this.elt.addEventListener("mousedown", this._onMouseDown, false);

  
  this._prepareImagePreview();
}

MarkupContainer.prototype = {
  toString: function() {
    return "[MarkupContainer for " + this.node + "]";
  },

  _prepareImagePreview: function() {
    if (this.node.tagName) {
      let tagName = this.node.tagName.toLowerCase();
      let srcAttr = this.editor.getAttributeElement("src");
      let isImage = tagName === "img" && srcAttr;
      let isCanvas = tagName === "canvas";

      
      
      if (isImage || isCanvas) {
        let def = promise.defer();

        this.tooltipData = {
          target: isImage ? srcAttr : this.editor.tag,
          data: def.promise
        };

        this.node.getImageData(IMAGE_PREVIEW_MAX_DIM).then(data => {
          if (data) {
            data.data.string().then(str => {
              
              
              def.resolve(str, data.size);
              this.tooltipData.data = promise.resolve(str, data.size);
            });
          }
        });
      }
    }
  },

  _buildTooltipContent: function(target, tooltip) {
    if (this.tooltipData && target === this.tooltipData.target) {
      this.tooltipData.data.then((data, size) => {
        tooltip.setImageContent(data, size);
      });
      return true;
    }
  },

  



  _hasChildren: false,

  get hasChildren() {
    return this._hasChildren;
  },

  set hasChildren(aValue) {
    this._hasChildren = aValue;
    if (aValue) {
      this.expander.style.visibility = "visible";
    } else {
      this.expander.style.visibility = "hidden";
    }
  },

  parentContainer: function() {
    return this.elt.parentNode ? this.elt.parentNode.container : null;
  },

  


  get expanded() {
    return !this.elt.classList.contains("collapsed");
  },

  set expanded(aValue) {
    if (aValue && this.elt.classList.contains("collapsed")) {
      
      
      if (this.editor instanceof ElementEditor) {
        let closingTag = this.elt.querySelector(".close");
        if (closingTag) {
          if (!this.closeTagLine) {
            let line = this.markup.doc.createElement("div");
            line.classList.add("tag-line");

            let highlighter = this.markup.doc.createElement("div");
            highlighter.classList.add("highlighter");
            line.appendChild(highlighter);

            line.appendChild(closingTag.cloneNode(true));
            line.addEventListener("mouseover", this._onMouseOver, false);
            line.addEventListener("mouseout", this._onMouseOut, false);

            this.closeTagLine = line;
          }
          this.elt.appendChild(this.closeTagLine);
        }
      }
      this.elt.classList.remove("collapsed");
      this.expander.setAttribute("open", "");
      this.highlighted = false;
    } else if (!aValue) {
      if (this.editor instanceof ElementEditor && this.closeTagLine) {
        this.elt.removeChild(this.closeTagLine);
      }
      this.elt.classList.add("collapsed");
      this.expander.removeAttribute("open");
    }
  },

  _onToggle: function(event) {
    this.markup.navigate(this);
    if(this.hasChildren) {
      this.markup.setNodeExpanded(this.node, !this.expanded);
    }
    event.stopPropagation();
  },

  _onMouseOver: function(event) {
    this.highlighted = true;
    event.stopPropagation();
  },

  _onMouseOut: function(event) {
    this.highlighted = false;
    event.stopPropagation();
  },

  _onMouseDown: function(event) {
    this.highlighted = false;
    this.markup.navigate(this);
    event.stopPropagation();
  },

  



  flashMutation: function() {
    if (!this.selected) {
      let contentWin = this.markup._frame.contentWindow;
      this.flashed = true;
      if (this._flashMutationTimer) {
        contentWin.clearTimeout(this._flashMutationTimer);
        this._flashMutationTimer = null;
      }
      this._flashMutationTimer = contentWin.setTimeout(() => {
        this.flashed = false;
      }, CONTAINER_FLASHING_DURATION);
    }
  },

  set flashed(aValue) {
    if (aValue) {
      
      this.highlighter.classList.remove("flash-out");

      
      this.highlighter.classList.add("theme-bg-contrast");

      
      this.editor.elt.classList.add("theme-fg-contrast");
      [].forEach.call(
        this.editor.elt.querySelectorAll("[class*=theme-fg-color]"),
        span => span.classList.add("theme-fg-contrast")
      );
    } else {
      
      this.highlighter.classList.add("flash-out");

      
      this.highlighter.classList.remove("theme-bg-contrast");

      
      this.editor.elt.classList.remove("theme-fg-contrast");
      [].forEach.call(
        this.editor.elt.querySelectorAll("[class*=theme-fg-color]"),
        span => span.classList.remove("theme-fg-contrast")
      );
    }
  },

  _highlighted: false,

  



  set highlighted(aValue) {
    this.highlighter.classList.remove("flash-out");
    this._highlighted = aValue;
    if (aValue) {
      if (!this.selected) {
        this.highlighter.classList.add("theme-bg-darker");
      }
      if (this.closeTagLine) {
        this.closeTagLine.querySelector(".highlighter").classList.add(
          "theme-bg-darker");
      }
    } else {
      this.highlighter.classList.remove("theme-bg-darker");
      if (this.closeTagLine) {
        this.closeTagLine.querySelector(".highlighter").classList.remove(
          "theme-bg-darker");
      }
    }
  },

  


  get visible() {
    return this.elt.getBoundingClientRect().height > 0;
  },

  


  _selected: false,

  get selected() {
    return this._selected;
  },

  set selected(aValue) {
    this.highlighter.classList.remove("flash-out");
    this._selected = aValue;
    this.editor.selected = aValue;
    if (this._selected) {
      this.tagLine.setAttribute("selected", "");
      this.highlighter.classList.add("theme-selected");
    } else {
      this.tagLine.removeAttribute("selected");
      this.highlighter.classList.remove("theme-selected");
    }
  },

  



  update: function() {
    if (this.editor.update) {
      this.editor.update();
    }
  },

  


  focus: function() {
    let focusable = this.editor.elt.querySelector("[tabindex]");
    if (focusable) {
      focusable.focus();
    }
  },

  



  destroy: function() {
    
    let firstChild;
    while (firstChild = this.children.firstChild) {
      firstChild.container.destroy();
      this.children.removeChild(firstChild);
    }

    
    this.elt.removeEventListener("dblclick", this._onToggle, false);
    this.elt.removeEventListener("mouseover", this._onMouseOver, false);
    this.elt.removeEventListener("mouseout", this._onMouseOut, false);
    this.elt.removeEventListener("mousedown", this._onMouseDown, false);
    this.expander.removeEventListener("click", this._onToggle, false);

    
    this.editor.destroy();
  }
};





function RootContainer(aMarkupView, aNode) {
  this.doc = aMarkupView.doc;
  this.elt = this.doc.createElement("ul");
  this.elt.container = this;
  this.children = this.elt;
  this.node = aNode;
  this.toString = () => "[root container]";
}

RootContainer.prototype = {
  hasChildren: true,
  expanded: true,
  update: function() {},
  destroy: function() {}
};




function GenericEditor(aContainer, aNode) {
  this.elt = aContainer.doc.createElement("span");
  this.elt.className = "editor";
  this.elt.textContent = aNode.nodeName;
}

GenericEditor.prototype = {
  destroy: function() {}
};







function DoctypeEditor(aContainer, aNode) {
  this.elt = aContainer.doc.createElement("span");
  this.elt.className = "editor comment";
  this.elt.textContent = '<!DOCTYPE ' + aNode.name +
     (aNode.publicId ? ' PUBLIC "' +  aNode.publicId + '"': '') +
     (aNode.systemId ? ' "' + aNode.systemId + '"' : '') +
     '>';
}

DoctypeEditor.prototype = {
  destroy: function() {}
};









function TextEditor(aContainer, aNode, aTemplate) {
  this.node = aNode;
  this._selected = false;

  aContainer.markup.template(aTemplate, this);

  editableField({
    element: this.value,
    stopOnReturn: true,
    trigger: "dblclick",
    multiline: true,
    done: (aVal, aCommit) => {
      if (!aCommit) {
        return;
      }
      this.node.getNodeValue().then(longstr => {
        longstr.string().then(oldValue => {
          longstr.release().then(null, console.error);

          aContainer.undo.do(() => {
            this.node.setNodeValue(aVal).then(() => {
              aContainer.markup.nodeChanged(this.node);
            });
          }, () => {
            this.node.setNodeValue(oldValue).then(() => {
              aContainer.markup.nodeChanged(this.node);
            })
          });
        });
      });
    }
  });

  this.update();
}

TextEditor.prototype = {
  get selected() this._selected,
  set selected(aValue) {
    if (aValue === this._selected) {
      return;
    }
    this._selected = aValue;
    this.update();
  },

  update: function() {
    if (!this.selected || !this.node.incompleteValue) {
      let text = this.node.shortValue;
      
      if (this.node.incompleteValue) {
        text += "…";
      }
      this.value.textContent = text;
    } else {
      let longstr = null;
      this.node.getNodeValue().then(ret => {
        longstr = ret;
        return longstr.string();
      }).then(str => {
        longstr.release().then(null, console.error);
        if (this.selected) {
          this.value.textContent = str;
        }
      }).then(null, console.error);
    }
  },

  destroy: function() {}
};







function ElementEditor(aContainer, aNode) {
  this.doc = aContainer.doc;
  this.undo = aContainer.undo;
  this.template = aContainer.markup.template.bind(aContainer.markup);
  this.container = aContainer;
  this.markup = this.container.markup;
  this.node = aNode;

  this.attrs = {};

  
  this.elt = null;
  this.tag = null;
  this.closeTag = null;
  this.attrList = null;
  this.newAttr = null;
  this.closeElt = null;

  
  this.template("element", this);

  if (aNode.isLocal_toBeDeprecated()) {
    this.rawNode = aNode.rawNode();
  }

  
  
  if (this.rawNode && !aNode.isDocumentElement) {
    this.tag.setAttribute("tabindex", "0");
    editableField({
      element: this.tag,
      trigger: "dblclick",
      stopOnReturn: true,
      done: this.onTagEdit.bind(this),
    });
  }

  
  editableField({
    element: this.newAttr,
    trigger: "dblclick",
    stopOnReturn: true,
    contentType: InplaceEditor.CONTENT_TYPES.CSS_MIXED,
    popup: this.markup.popup,
    done: (aVal, aCommit) => {
      if (!aCommit) {
        return;
      }

      try {
        let doMods = this._startModifyingAttributes();
        let undoMods = this._startModifyingAttributes();
        this._applyAttributes(aVal, null, doMods, undoMods);
        this.undo.do(() => {
          doMods.apply();
        }, function() {
          undoMods.apply();
        });
      } catch(x) {
        console.error(x);
      }
    }
  });

  let tagName = this.node.nodeName.toLowerCase();
  this.tag.textContent = tagName;
  this.closeTag.textContent = tagName;

  this.update();
}

ElementEditor.prototype = {
  


  update: function() {
    let attrs = this.node.attributes;
    if (!attrs) {
      return;
    }

    
    
    
    let attrEditors = this.attrList.querySelectorAll(".attreditor");
    for (let i = 0; i < attrEditors.length; i++) {
      if (!attrEditors[i].inplaceEditor) {
        attrEditors[i].style.display = "none";
      }
    }

    
    
    for (let attr of attrs) {
      let attribute = this._createAttribute(attr);
      if (!attribute.inplaceEditor) {
        attribute.style.removeProperty("display");
      }
    }
  },

  _startModifyingAttributes: function() {
    return this.node.startModifyingAttributes();
  },

  




  getAttributeElement: function(attrName) {
    return this.attrList.querySelector(
      ".attreditor[data-attr=" + attrName + "] .attr-value");
  },

  _createAttribute: function(aAttr, aBefore = null) {
    
    let data = {
      attrName: aAttr.name,
    };
    this.template("attribute", data);
    var {attr, inner, name, val} = data;

    
    
    
    let editValueDisplayed = aAttr.value || "";
    let hasDoubleQuote = editValueDisplayed.contains('"');
    let hasSingleQuote = editValueDisplayed.contains("'");
    let initial = aAttr.name + '="' + editValueDisplayed + '"';

    
    if (hasDoubleQuote && hasSingleQuote) {
        editValueDisplayed = editValueDisplayed.replace(/\"/g, "&quot;");
        initial = aAttr.name + '="' + editValueDisplayed + '"';
    }

    
    if (hasDoubleQuote && !hasSingleQuote) {
        initial = aAttr.name + "='" + editValueDisplayed + "'";
    }

    
    editableField({
      element: inner,
      trigger: "dblclick",
      stopOnReturn: true,
      selectAll: false,
      initial: initial,
      contentType: InplaceEditor.CONTENT_TYPES.CSS_MIXED,
      popup: this.markup.popup,
      start: (aEditor, aEvent) => {
        
        
        if (aEvent && aEvent.target === name) {
          aEditor.input.setSelectionRange(0, name.textContent.length);
        } else if (aEvent && aEvent.target === val) {
          let length = editValueDisplayed.length;
          let editorLength = aEditor.input.value.length;
          let start = editorLength - (length + 1);
          aEditor.input.setSelectionRange(start, start + length);
        } else {
          aEditor.input.select();
        }
      },
      done: (aVal, aCommit) => {
        if (!aCommit || aVal === initial) {
          return;
        }

        let doMods = this._startModifyingAttributes();
        let undoMods = this._startModifyingAttributes();

        
        
        
        try {
          this._saveAttribute(aAttr.name, undoMods);
          doMods.removeAttribute(aAttr.name);
          this._applyAttributes(aVal, attr, doMods, undoMods);
          this.undo.do(() => {
            doMods.apply();
          }, () => {
            undoMods.apply();
          })
        } catch(ex) {
          console.error(ex);
        }
      }
    });

    
    let before = aBefore;
    if (aAttr.name == "id") {
      before = this.attrList.firstChild;
    } else if (aAttr.name == "class") {
      let idNode = this.attrs["id"];
      before = idNode ? idNode.nextSibling : this.attrList.firstChild;
    }
    this.attrList.insertBefore(attr, before);

    
    let oldAttr = this.attrs[aAttr.name];
    if (oldAttr && oldAttr.parentNode) {
      oldAttr.parentNode.removeChild(oldAttr);
    }

    this.attrs[aAttr.name] = attr;

    name.textContent = aAttr.name;

    if (typeof aAttr.value !== "undefined") {
      let outputParser = this.markup._outputParser;
      let frag = outputParser.parseHTMLAttribute(aAttr.value);
      frag = this._truncateFrag(frag);
      val.appendChild(frag);
    }

    return attr;
  },

  










  _truncateFrag: function(frag) {
    let chars = 0;
    let text = frag.textContent;
    let maxWidth = text.match(COLLAPSE_DATA_URL_REGEX) ?
                            COLLAPSE_DATA_URL_LENGTH : COLLAPSE_ATTRIBUTE_LENGTH;
    let overBy = text.length - maxWidth;
    let children = frag.childNodes;
    let croppedNode = null;

    if (overBy <= 0) {
      return frag;
    }

    
    
    if (children.length === 1) {
      let length = text.length;
      let start = text.substr(0, maxWidth / 2);
      let end = text.substr(length - maxWidth / 2, length - 1);

      frag.textContent = start + "…" + end;
      return frag;
    }

    
    for (let i = 0; i < children.length; i++) {
      let node = children[i];
      let text = node.textContent;

      let numChars = text.length;
      if (chars + numChars > maxWidth / 2) {
        node.textContent = text.substr(0, chars + numChars - maxWidth / 2) + "…";
        croppedNode = node;
        break;
      } else {
        chars += numChars;
      }
    }

    
    chars = 0;
    for (let i = children.length - 1; i >= 0; i--) {
      let node = children[i];
      let text = node.textContent;

      let numChars = text.length;
      if (chars + numChars > maxWidth / 2) {
        if (node !== croppedNode) {
          node.parentNode.removeChild(node);
          chars += numChars;
        } else {
          break;
        }
      } else {
        chars += numChars;
      }
    }

    return frag;
  },

  








  _applyAttributes: function(aValue, aAttrNode, aDoMods, aUndoMods) {
    let attrs = parseAttributeValues(aValue, this.doc);
    for (let attr of attrs) {
      
      this._createAttribute(attr, aAttrNode ? aAttrNode.nextSibling : null);
      this._saveAttribute(attr.name, aUndoMods);
      aDoMods.setAttribute(attr.name, attr.value);
    }
  },

  



  _saveAttribute: function(aName, aUndoMods) {
    let node = this.node;
    if (node.hasAttribute(aName)) {
      let oldValue = node.getAttribute(aName);
      aUndoMods.setAttribute(aName, oldValue);
    } else {
      aUndoMods.removeAttribute(aName);
    }
  },

  


  onTagEdit: function(aVal, aCommit) {
    if (!aCommit || aVal == this.rawNode.tagName) {
      return;
    }

    
    
    
    try {
      var newElt = nodeDocument(this.rawNode).createElement(aVal);
    } catch(x) {
      
      
      return;
    }

    let attrs = this.rawNode.attributes;

    for (let i = 0 ; i < attrs.length; i++) {
      newElt.setAttribute(attrs[i].name, attrs[i].value);
    }
    let newFront = this.markup.walker.frontForRawNode(newElt);
    let newContainer = this.markup.importNode(newFront);

    
    let walker = this.markup.walker;
    promise.all([
      walker.retainNode(newFront), walker.retainNode(this.node)
    ]).then(() => {
      function swapNodes(aOld, aNew) {
        aOld.parentNode.insertBefore(aNew, aOld);
        while (aOld.firstChild) {
          aNew.appendChild(aOld.firstChild);
        }
        aOld.parentNode.removeChild(aOld);
      }

      this.undo.do(() => {
        swapNodes(this.rawNode, newElt);
        this.markup.setNodeExpanded(newFront, this.container.expanded);
        if (this.container.selected) {
          this.markup.navigate(newContainer);
        }
      }, () => {
        swapNodes(newElt, this.rawNode);
        this.markup.setNodeExpanded(this.node, newContainer.expanded);
        if (newContainer.selected) {
          this.markup.navigate(this.container);
        }
      });
    }).then(null, console.error);
  },

  destroy: function() {}
};

function nodeDocument(node) {
  return node.ownerDocument ||
    (node.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE ? node : null);
}

function truncateString(str, maxLength) {
  if (str.length <= maxLength) {
    return str;
  }

  return str.substring(0, Math.ceil(maxLength / 2)) +
         "…" +
         str.substring(str.length - Math.floor(maxLength / 2));
}










function parseAttributeValues(attr, doc) {
  attr = attr.trim();

  
  let el = DOMParser.parseFromString("<div " + attr + "></div>", "text/html").body.childNodes[0] ||
           DOMParser.parseFromString("<div " + attr + "\"></div>", "text/html").body.childNodes[0] ||
           DOMParser.parseFromString("<div " + attr + "'></div>", "text/html").body.childNodes[0];
  let div = doc.createElement("div");

  let attributes = [];
  for (let attribute of el.attributes) {
    
    
    try {
      div.setAttribute(attribute.name, attribute.value);
      attributes.push({
        name: attribute.name,
        value: attribute.value
      });
    }
    catch(e) { }
  }

  
  return attributes.reverse();
}

loader.lazyGetter(MarkupView.prototype, "strings", () => Services.strings.createBundle(
  "chrome://browser/locale/devtools/inspector.properties"
));
