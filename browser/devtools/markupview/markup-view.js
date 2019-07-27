





const {Cc, Cu, Ci} = require("chrome");


const PAGE_SIZE = 10;
const PREVIEW_AREA = 700;
const DEFAULT_MAX_CHILDREN = 100;
const COLLAPSE_ATTRIBUTE_LENGTH = 120;
const COLLAPSE_DATA_URL_REGEX = /^data.+base64/;
const COLLAPSE_DATA_URL_LENGTH = 60;
const NEW_SELECTION_HIGHLIGHTER_TIMER = 1000;
const GRAB_DELAY = 400;
const DRAG_DROP_AUTOSCROLL_EDGE_DISTANCE = 50;
const DRAG_DROP_MIN_AUTOSCROLL_SPEED = 5;
const DRAG_DROP_MAX_AUTOSCROLL_SPEED = 15;

const {UndoStack} = require("devtools/shared/undo");
const {editableField, InplaceEditor} = require("devtools/shared/inplace-editor");
const {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
const {HTMLEditor} = require("devtools/markupview/html-editor");
const promise = require("resource://gre/modules/Promise.jsm").Promise;
const {Tooltip} = require("devtools/shared/widgets/Tooltip");
const EventEmitter = require("devtools/toolkit/event-emitter");
const Heritage = require("sdk/core/heritage");
const {setTimeout, clearTimeout, setInterval, clearInterval} = require("sdk/timers");
const ELLIPSIS = Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data;

Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm");
Cu.import("resource://gre/modules/devtools/Templater.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

loader.lazyGetter(this, "DOMParser", function() {
 return Cc["@mozilla.org/xmlextras/domparser;1"].createInstance(Ci.nsIDOMParser);
});
loader.lazyGetter(this, "AutocompletePopup", () => {
  return require("devtools/shared/autocomplete-popup").AutocompletePopup;
});






















function MarkupView(aInspector, aFrame, aControllerWindow) {
  this._inspector = aInspector;
  this.walker = this._inspector.walker;
  this._frame = aFrame;
  this.win = this._frame.contentWindow;
  this.doc = this._frame.contentDocument;
  this._elt = this.doc.querySelector("#root");
  this.htmlEditor = new HTMLEditor(this.doc);

  this.layoutHelpers = new LayoutHelpers(this.doc.defaultView);

  try {
    this.maxChildren = Services.prefs.getIntPref("devtools.markup.pagesize");
  } catch(ex) {
    this.maxChildren = DEFAULT_MAX_CHILDREN;
  }

  
  let options = {
    autoSelect: true,
    theme: "auto"
  };
  this.popup = new AutocompletePopup(this.doc.defaultView.parent.document, options);

  this.undo = new UndoStack();
  this.undo.installController(aControllerWindow);

  this._containers = new Map();

  this._boundMutationObserver = this._mutationObserver.bind(this);
  this.walker.on("mutations", this._boundMutationObserver);

  this._boundOnDisplayChange = this._onDisplayChange.bind(this);
  this.walker.on("display-change", this._boundOnDisplayChange);

  this._onMouseClick = this._onMouseClick.bind(this);

  this._onMouseUp = this._onMouseUp.bind(this);
  this.doc.body.addEventListener("mouseup", this._onMouseUp);

  this._boundOnNewSelection = this._onNewSelection.bind(this);
  this._inspector.selection.on("new-node-front", this._boundOnNewSelection);
  this._onNewSelection();

  this._boundKeyDown = this._onKeyDown.bind(this);
  this._frame.contentWindow.addEventListener("keydown", this._boundKeyDown, false);

  this._boundFocus = this._onFocus.bind(this);
  this._frame.addEventListener("focus", this._boundFocus, false);

  this._makeTooltipPersistent = this._makeTooltipPersistent.bind(this);

  this._initPreview();
  this._initTooltips();
  this._initHighlighter();

  EventEmitter.decorate(this);
}

exports.MarkupView = MarkupView;

MarkupView.prototype = {
  


  CONTAINER_FLASHING_DURATION: 500,

  _selectedContainer: null,

  _initTooltips: function() {
    this.tooltip = new Tooltip(this._inspector.panelDoc);
    this._makeTooltipPersistent(false);

    this._elt.addEventListener("click", this._onMouseClick, false);
  },

  _initHighlighter: function() {
    
    this._onMouseMove = this._onMouseMove.bind(this);
    this._elt.addEventListener("mousemove", this._onMouseMove, false);
    this._onMouseLeave = this._onMouseLeave.bind(this);
    this._elt.addEventListener("mouseleave", this._onMouseLeave, false);

    
    
    this._onToolboxPickerHover = (event, nodeFront) => {
      this.showNode(nodeFront, true).then(() => {
        this._showContainerAsHovered(nodeFront);
      });
    };
    this._inspector.toolbox.on("picker-node-hovered", this._onToolboxPickerHover);
  },

  _makeTooltipPersistent: function(state) {
    if (state) {
      this.tooltip.stopTogglingOnHover();
    } else {
      this.tooltip.startTogglingOnHover(this._elt,
        this._isImagePreviewTarget.bind(this));
    }
  },

  isDragging: false,

  _onMouseMove: function(event) {
    if (this.isDragging) {
      event.preventDefault();
      this._dragStartEl = event.target;

      let docEl = this.doc.documentElement;

      if (this._scrollInterval) {
        clearInterval(this._scrollInterval);
      }

      
      let distanceFromBottom = docEl.clientHeight - event.pageY + this.win.scrollY,
          distanceFromTop = event.pageY - this.win.scrollY;

      if (distanceFromBottom <= DRAG_DROP_AUTOSCROLL_EDGE_DISTANCE) {
        
        
        let speed = map(distanceFromBottom, 0, DRAG_DROP_AUTOSCROLL_EDGE_DISTANCE,
                        DRAG_DROP_MIN_AUTOSCROLL_SPEED, DRAG_DROP_MAX_AUTOSCROLL_SPEED);
        
        
        
        this._scrollInterval = setInterval(() => {
          docEl.scrollTop -= speed - DRAG_DROP_MAX_AUTOSCROLL_SPEED;
        }, 0);
      }

      if (distanceFromTop <= DRAG_DROP_AUTOSCROLL_EDGE_DISTANCE) {
        
        let speed = map(distanceFromTop, 0, DRAG_DROP_AUTOSCROLL_EDGE_DISTANCE,
                        DRAG_DROP_MIN_AUTOSCROLL_SPEED, DRAG_DROP_MAX_AUTOSCROLL_SPEED);

        this._scrollInterval = setInterval(() => {
          docEl.scrollTop += speed - DRAG_DROP_MAX_AUTOSCROLL_SPEED;
        }, 0);
      }

      return;
    };

    let target = event.target;

    
    while (!target.container) {
      if (target.tagName.toLowerCase() === "body") {
        return;
      }
      target = target.parentNode;
    }

    let container = target.container;
    if (this._hoveredNode !== container.node) {
      if (container.node.nodeType !== Ci.nsIDOMNode.TEXT_NODE) {
        this._showBoxModel(container.node);
      } else {
        this._hideBoxModel();
      }
    }
    this._showContainerAsHovered(container.node);
  },

  _onMouseClick: function(event) {
    
    
    let parentNode = event.target;
    let container;
    while (parentNode !== this.doc.body) {
      if (parentNode.container) {
        container = parentNode.container;
        break;
      }
      parentNode = parentNode.parentNode;
    }

    if (container instanceof MarkupElementContainer) {
      
      
      container._buildEventTooltipContent(event.target, this.tooltip);
    }
  },

  _onMouseUp: function() {
    if (this._lastDropTarget) {
      this.indicateDropTarget(null);
    }
    if (this._lastDragTarget) {
      this.indicateDragTarget(null);
    }
    if (this._scrollInterval) {
      clearInterval(this._scrollInterval);
    }
  },

  _hoveredNode: null,

  



  _showContainerAsHovered: function(nodeFront) {
    if (this._hoveredNode === nodeFront) {
      return;
    }

    if (this._hoveredNode) {
      this.getContainer(this._hoveredNode).hovered = false;
    }

    this.getContainer(nodeFront).hovered = true;
    this._hoveredNode = nodeFront;
  },

  _onMouseLeave: function() {
    if (this._scrollInterval) {
      clearInterval(this._scrollInterval);
    }
    if (this.isDragging) return;

    this._hideBoxModel(true);
    if (this._hoveredNode) {
      this.getContainer(this._hoveredNode).hovered = false;
    }
    this._hoveredNode = null;
  },

  






  _showBoxModel: function(nodeFront) {
    return this._inspector.toolbox.highlighterUtils.highlightNodeFront(nodeFront);
  },

  







  _hideBoxModel: function(forceHide) {
    return this._inspector.toolbox.highlighterUtils.unhighlight(forceHide);
  },

  _briefBoxModelTimer: null,

  _clearBriefBoxModelTimer: function() {
    if (this._briefBoxModelTimer) {
      clearTimeout(this._briefBoxModelTimer);
      this._briefBoxModelTimer = null;
    }
  },

  _brieflyShowBoxModel: function(nodeFront) {
    this._clearBriefBoxModelTimer();
    this._showBoxModel(nodeFront);

    this._briefBoxModelTimer = setTimeout(() => {
      this._hideBoxModel();
    }, NEW_SELECTION_HIGHLIGHTER_TIMER);
  },

  template: function(aName, aDest, aOptions={stack: "markup-view.xhtml"}) {
    let node = this.doc.getElementById("template-" + aName).cloneNode(true);
    node.removeAttribute("id");
    template(node, aDest, aOptions);
    return node;
  },

  



  getContainer: function(aNode) {
    return this._containers.get(aNode);
  },

  update: function() {
    let updateChildren = (node) => {
      this.getContainer(node).update();
      for (let child of node.treeChildren()) {
        updateChildren(child);
      }
    };

    
    let documentElement;
    for (let node of this._rootNode.treeChildren()) {
      if (node.isDocumentElement === true) {
        documentElement = node;
        break;
      }
    }

    
    updateChildren(documentElement);
  },

  







  _isImagePreviewTarget: function(target) {
    
    
    let parent = target, container;
    while (parent !== this.doc.body) {
      if (parent.container) {
        container = parent.container;
        break;
      }
      parent = parent.parentNode;
    }

    if (container instanceof MarkupElementContainer) {
      
      
      return container.isImagePreviewTarget(target, this.tooltip);
    }
  },

  












  _shouldNewSelectionBeHighlighted: function() {
    let reason = this._inspector.selection.reason;
    let unwantedReasons = ["inspector-open", "navigateaway", "nodeselected", "test"];
    let isHighlitNode = this._hoveredNode === this._inspector.selection.nodeFront;
    return !isHighlitNode && reason && unwantedReasons.indexOf(reason) === -1;
  },

  


  _onNewSelection: function() {
    let selection = this._inspector.selection;

    this.htmlEditor.hide();
    if (this._hoveredNode && this._hoveredNode !== selection.nodeFront) {
      this.getContainer(this._hoveredNode).hovered = false;
      this._hoveredNode = null;
    }

    let done = this._inspector.updating("markup-view");
    if (selection.isNode()) {
      if (this._shouldNewSelectionBeHighlighted()) {
        this._brieflyShowBoxModel(selection.nodeFront);
      }

      this.showNode(selection.nodeFront, true).then(() => {
        if (this._destroyer) {
          return promise.reject("markupview destroyed");
        }
        if (selection.reason !== "treepanel") {
          this.markNodeAsSelected(selection.nodeFront);
        }
        done();
      }).then(null, e => {
        if (!this._destroyer) {
          console.error(e);
        } else {
          console.warn("Could not mark node as selected, the markup-view was " +
            "destroyed while showing the node.");
        }

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
        if (aEvent.metaKey || aEvent.shiftKey) {
          handled = false;
        } else {
          let node = this._selectedContainer.node;
          if (node.hidden) {
            this.walker.unhideNode(node).then(() => this.nodeChanged(node));
          } else {
            this.walker.hideNode(node).then(() => this.nodeChanged(node));
          }
        }
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_DELETE:
        this.deleteNode(this._selectedContainer.node);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_BACK_SPACE:
        this.deleteNode(this._selectedContainer.node, true);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_HOME:
        let rootContainer = this.getContainer(this._rootNode);
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
      case Ci.nsIDOMKeyEvent.DOM_VK_F2: {
        this.beginEditingOuterHTML(this._selectedContainer.node);
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

  







  deleteNode: function(aNode, moveBackward) {
    if (aNode.isDocumentElement ||
        aNode.nodeType == Ci.nsIDOMNode.DOCUMENT_TYPE_NODE ||
        aNode.isAnonymous) {
      return;
    }

    let container = this.getContainer(aNode);

    
    this.walker.retainNode(aNode).then(() => {
      let parent = aNode.parentNode();
      let nextSibling = null;
      this.undo.do(() => {
        this.walker.removeNode(aNode).then(siblings => {
          nextSibling = siblings.nextSibling;
          let focusNode = moveBackward ? siblings.previousSibling : nextSibling;

          
          
          if (!focusNode) {
            focusNode = nextSibling || siblings.previousSibling || parent;
          }

          if (container.selected) {
            this.navigate(this.getContainer(focusNode));
          }
        });
      }, () => {
        this.walker.insertBefore(aNode, parent, nextSibling);
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

    if (!aIgnoreFocus) {
      aContainer.focus();
    }
  },

  








  importNode: function(aNode, aFlashNode) {
    if (!aNode) {
      return null;
    }

    if (this._containers.has(aNode)) {
      return this.getContainer(aNode);
    }

    let container;
    let {nodeType, isPseudoElement} = aNode;
    if (aNode === this.walker.rootNode) {
      container = new RootContainer(this, aNode);
      this._elt.appendChild(container.elt);
      this._rootNode = aNode;
    } else if (nodeType == Ci.nsIDOMNode.ELEMENT_NODE && !isPseudoElement) {
      container = new MarkupElementContainer(this, aNode, this._inspector);
    } else if (nodeType == Ci.nsIDOMNode.COMMENT_NODE ||
               nodeType == Ci.nsIDOMNode.TEXT_NODE) {
      container = new MarkupTextContainer(this, aNode, this._inspector);
    } else {
      container = new MarkupReadOnlyContainer(this, aNode, this._inspector);
    }

    if (aFlashNode) {
      container.flashMutation();
    }

    this._containers.set(aNode, container);
    container.childrenDirty = true;

    this._updateChildren(container);

    return container;
  },

  


  _mutationObserver: function(aMutations) {
    let requiresLayoutChange = false;

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

      let container = this.getContainer(target);
      if (!container) {
        
        
        continue;
      }
      if (type === "attributes" || type === "characterData") {
        container.update();

        
        if (type === "attributes" && container.selected) {
          requiresLayoutChange = true;
        }
      } else if (type === "childList") {
        container.childrenDirty = true;
        
        this._updateChildren(container, {flash: true});
      }
    }

    if (requiresLayoutChange) {
      this._inspector.immediateLayoutChange();
    }
    this._waitForChildren().then((nodes) => {
      if (this._destroyer) {
        console.warn("Could not fully update after markup mutations, " +
          "the markup-view was destroyed while waiting for children.");
        return;
      }
      this._flashMutatedNodes(aMutations);
      this._inspector.emit("markupmutation", aMutations);

      
      
      this.htmlEditor.refresh();
    });
  },

  



  _onDisplayChange: function(nodes) {
    for (let node of nodes) {
      let container = this.getContainer(node);
      if (container) {
        container.isDisplayed = node.isDisplayed;
      }
    }
  },

  



  _flashMutatedNodes: function(aMutations) {
    let addedOrEditedContainers = new Set();
    let removedContainers = new Set();

    for (let {type, target, added, removed, newValue} of aMutations) {
      let container = this.getContainer(target);

      if (container) {
        if (type === "characterData") {
          addedOrEditedContainers.add(container);
        } else if (type === "attributes" && newValue === null) {
          
          
          
          addedOrEditedContainers.add(container);
        } else if (type === "childList") {
          
          if (removed.length) {
            removedContainers.add(container);
          }

          
          
          added.forEach(added => {
            let addedContainer = this.getContainer(added);
            if (addedContainer) {
              addedOrEditedContainers.add(addedContainer);

              
              
              
              
              removedContainers.delete(container);
            }
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
    let parent = aNode;

    this.importNode(aNode);

    while ((parent = parent.parentNode())) {
      this.importNode(parent);
      this.expandNode(parent);
    }

    return this._waitForChildren().then(() => {
      if (this._destroyer) {
        return promise.reject("markupview destroyed");
      }
      return this._ensureVisible(aNode);
    }).then(() => {
      
      this.layoutHelpers.scrollIntoViewIfNeeded(this.getContainer(aNode).editor.elt, centered);
    }, e => {
      
      
      if (!this._destroyer) {
        console.error(e);
      } else {
        console.warn("Could not show the node, the markup-view was destroyed " +
          "while waiting for children");
      }
    });
  },

  


  _expandContainer: function(aContainer) {
    return this._updateChildren(aContainer, {expand: true}).then(() => {
      if (this._destroyer) {
        console.warn("Could not expand the node, the markup-view was destroyed");
        return;
      }
      aContainer.expanded = true;
    });
  },

  


  expandNode: function(aNode) {
    let container = this.getContainer(aNode);
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
    return this._expandAll(this.getContainer(aNode));
  },

  


  collapseNode: function(aNode) {
    let container = this.getContainer(aNode);
    container.expanded = false;
  },

  






  _getNodeHTML: function(aNode, isOuter) {
    let walkerPromise = null;

    if (isOuter) {
      walkerPromise = this.walker.outerHTML(aNode);
    } else {
      walkerPromise = this.walker.innerHTML(aNode);
    }

    return walkerPromise.then(longstr => {
      return longstr.string().then(html => {
        longstr.release().then(null, console.error);
        return html;
      });
    });
  },

  




  getNodeOuterHTML: function(aNode) {
    return this._getNodeHTML(aNode, true);
  },

  




  getNodeInnerHTML: function(aNode) {
    return this._getNodeHTML(aNode);
  },

  





  reselectOnRemoved: function(removedNode, reason) {
    
    
    this.cancelReselectOnRemoved();

    
    let isHTMLTag = removedNode.tagName.toLowerCase() === "html";
    let oldContainer = this.getContainer(removedNode);
    let parentContainer = this.getContainer(removedNode.parentNode());
    let childIndex = parentContainer.getChildContainers().indexOf(oldContainer);

    let onMutations = this._removedNodeObserver = (e, mutations) => {
      let isNodeRemovalMutation = false;
      for (let mutation of mutations) {
        let containsRemovedNode = mutation.removed &&
                                  mutation.removed.some(n => n === removedNode);
        if (mutation.type === "childList" && (containsRemovedNode || isHTMLTag)) {
          isNodeRemovalMutation = true;
          break;
        }
      }
      if (!isNodeRemovalMutation) {
        return;
      }

      this._inspector.off("markupmutation", onMutations);
      this._removedNodeObserver = null;

      
      
      if (this._inspector.selection.nodeFront === parentContainer.node ||
          (this._inspector.selection.nodeFront === removedNode && isHTMLTag)) {
        let childContainers = parentContainer.getChildContainers();
        if (childContainers && childContainers[childIndex]) {
          this.markNodeAsSelected(childContainers[childIndex].node, reason);
          if (childContainers[childIndex].hasChildren) {
            this.expandNode(childContainers[childIndex].node);
          }
          this.emit("reselectedonremoved");
        }
      }
    };

    
    
    this._inspector.on("markupmutation", onMutations);
  },

  




  cancelReselectOnRemoved: function() {
    if (this._removedNodeObserver) {
      this._inspector.off("markupmutation", this._removedNodeObserver);
      this._removedNodeObserver = null;
      this.emit("canceledreselectonremoved");
    }
  },

  








  updateNodeOuterHTML: function(node, newValue, oldValue) {
    let container = this.getContainer(node);
    if (!container) {
      return promise.reject();
    }

    
    
    this.reselectOnRemoved(node, "outerhtml");
    return this.walker.setOuterHTML(node, newValue).then(null, () => {
      this.cancelReselectOnRemoved();
    });
  },

  








  updateNodeInnerHTML: function(node, newValue, oldValue) {
    let container = this.getContainer(node);
    if (!container) {
      return promise.reject();
    }

    let def = promise.defer();

    container.undo.do(() => {
      this.walker.setInnerHTML(node, newValue).then(def.resolve, def.reject);
    }, () => {
      this.walker.setInnerHTML(node, oldValue);
    });

    return def.promise;
  },

  









  insertAdjacentHTMLToNode: function(node, position, value) {
    let container = this.getContainer(node);
    if (!container) {
      return promise.reject();
    }

    let def = promise.defer();

    let injectedNodes = [];
    container.undo.do(() => {
      this.walker.insertAdjacentHTML(node, position, value).then(nodeArray => {
        injectedNodes = nodeArray.nodes;
        return nodeArray;
      }).then(def.resolve, def.reject);
    }, () => {
      this.walker.removeNodes(injectedNodes);
    });

    return def.promise;
  },

  



  beginEditingOuterHTML: function(aNode) {
    this.getNodeOuterHTML(aNode).then(oldValue => {
      let container = this.getContainer(aNode);
      if (!container) {
        return;
      }
      this.htmlEditor.show(container.tagLine, oldValue);
      this.htmlEditor.once("popuphidden", (e, aCommit, aValue) => {
        
        
        this._frame.contentDocument.documentElement.focus();

        if (aCommit) {
          this.updateNodeOuterHTML(aNode, aValue, oldValue);
        }
      });
    });
  },

  





  setNodeExpanded: function(aNode, aExpanded, aExpandDescendants) {
    if (aExpanded) {
      if (aExpandDescendants) {
        this.expandAll(aNode);
      } else {
        this.expandNode(aNode);
      }
    } else {
      this.collapseNode(aNode);
    }
  },

  




  markNodeAsSelected: function(aNode, reason) {
    let container = this.getContainer(aNode);
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
      let container = this.getContainer(node);
      let parent = node.parentNode();
      if (!container.elt.parentNode) {
        let parentContainer = this.getContainer(parent);
        if (parentContainer) {
          parentContainer.childrenDirty = true;
          this._updateChildren(parentContainer, {expand: true});
        }
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
    if (this._destroyer) {
      return this._destroyer;
    }

    this._destroyer = promise.resolve();

    
    
    
    
    this._hideBoxModel();
    this._clearBriefBoxModelTimer();

    this._elt.removeEventListener("click", this._onMouseClick, false);

    this._hoveredNode = null;
    this._inspector.toolbox.off("picker-node-hovered", this._onToolboxPickerHover);

    this.htmlEditor.destroy();
    this.htmlEditor = null;

    this.undo.destroy();
    this.undo = null;

    this.popup.destroy();
    this.popup = null;

    this._frame.removeEventListener("focus", this._boundFocus, false);
    this._boundFocus = null;

    if (this._boundUpdatePreview) {
      this._frame.contentWindow.removeEventListener("scroll",
        this._boundUpdatePreview, true);
      this._boundUpdatePreview = null;
    }

    if (this._boundResizePreview) {
      this._frame.contentWindow.removeEventListener("resize",
        this._boundResizePreview, true);
      this._frame.contentWindow.removeEventListener("overflow",
        this._boundResizePreview, true);
      this._frame.contentWindow.removeEventListener("underflow",
        this._boundResizePreview, true);
      this._boundResizePreview = null;
    }

    this._frame.contentWindow.removeEventListener("keydown",
      this._boundKeyDown, false);
    this._boundKeyDown = null;

    this._inspector.selection.off("new-node-front", this._boundOnNewSelection);
    this._boundOnNewSelection = null;

    this.walker.off("mutations", this._boundMutationObserver);
    this._boundMutationObserver = null;

    this.walker.off("display-change", this._boundOnDisplayChange);
    this._boundOnDisplayChange = null;

    this._elt.removeEventListener("mousemove", this._onMouseMove, false);
    this._elt.removeEventListener("mouseleave", this._onMouseLeave, false);
    this._elt = null;

    for (let [key, container] of this._containers) {
      container.destroy();
    }
    this._containers = null;

    this.tooltip.destroy();
    this.tooltip = null;

    this.win = null;
    this.doc = null;

    this._lastDropTarget = null;
    this._lastDragTarget = null;

    return this._destroyer;
  },

  


  _initPreview: function() {
    this._previewEnabled = Services.prefs.getBoolPref("devtools.inspector.markupPreview");
    if (!this._previewEnabled) {
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
    if (!this._previewEnabled) {
      return;
    }
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

    height = ~~(win.innerHeight * ratio) + "px";
    let top = ~~(win.scrollY * ratio) + "px";
    this._viewbox.setAttribute("style", "height:" + height +
      ";transform: translateY(" + top + ")");
  },

  


  _resizePreview: function() {
    if (!this._previewEnabled) {
      return;
    }
    let win = this._frame.contentWindow;
    this._previewBar.classList.add("hide");
    clearTimeout(this._resizePreviewTimeout);

    setTimeout(() => {
      this._updatePreview();
      this._previewBar.classList.remove("hide");
    }, 1000);
  },

  



  indicateDropTarget: function(el) {
    if (this._lastDropTarget) {
      this._lastDropTarget.classList.remove("drop-target");
    }

    if (!el) return;

    let target = el.classList.contains("tag-line") ?
                 el : el.querySelector(".tag-line") || el.closest(".tag-line");
    if (!target) return;

    target.classList.add("drop-target");
    this._lastDropTarget = target;
  },

  


  indicateDragTarget: function(el) {
    if (this._lastDragTarget) {
      this._lastDragTarget.classList.remove("drag-target");
    }

    if (!el) return;

    let target = el.classList.contains("tag-line") ?
                 el : el.querySelector(".tag-line") || el.closest(".tag-line");

    if (!target) return;

    target.classList.add("drag-target");
    this._lastDragTarget = target;
  },

  


  get dropTargetNodes() {
    let target = this._lastDropTarget;

    if (!target) {
      return null;
    }

    let parent, nextSibling;

    if (this._lastDropTarget.previousElementSibling &&
        this._lastDropTarget.previousElementSibling.nodeName.toLowerCase() === "ul") {
      parent = target.parentNode.container.node;
      nextSibling = null;
    } else {
      parent = target.parentNode.container.node.parentNode();
      nextSibling = target.parentNode.container.node;
    }

    if (nextSibling && nextSibling.isBeforePseudoElement) {
      nextSibling = target.parentNode.parentNode.children[1].container.node;
    }
    if (nextSibling && nextSibling.isAfterPseudoElement) {
      parent = target.parentNode.container.node.parentNode();
      nextSibling = null;
    }

    if (parent.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE) {
      return null;
    }

    return {parent, nextSibling};
  }
};












function MarkupContainer() { }

MarkupContainer.prototype = {

  










  initialize: function(markupView, node, templateID) {
    this.markup = markupView;
    this.node = node;
    this.undo = this.markup.undo;
    this.win = this.markup._frame.contentWindow;

    
    this.elt = null;
    this.expander = null;
    this.tagState = null;
    this.tagLine = null;
    this.children = null;
    this.markup.template(templateID, this);
    this.elt.container = this;

    this._onMouseDown = this._onMouseDown.bind(this);
    this._onToggle = this._onToggle.bind(this);
    this._onMouseUp = this._onMouseUp.bind(this);
    this._onMouseMove = this._onMouseMove.bind(this);

    
    this.elt.addEventListener("mousedown", this._onMouseDown, false);
    this.markup.doc.body.addEventListener("mouseup", this._onMouseUp, true);
    this.markup.doc.body.addEventListener("mousemove", this._onMouseMove, true);
    this.elt.addEventListener("dblclick", this._onToggle, false);
    if (this.expander) {
      this.expander.addEventListener("click", this._onToggle, false);
    }

    
    this.isDisplayed = this.node.isDisplayed;
  },

  toString: function() {
    return "[MarkupContainer for " + this.node + "]";
  },

  isPreviewable: function() {
    if (this.node.tagName && !this.node.isPseudoElement) {
      let tagName = this.node.tagName.toLowerCase();
      let srcAttr = this.editor.getAttributeElement("src");
      let isImage = tagName === "img" && srcAttr;
      let isCanvas = tagName === "canvas";

      return isImage || isCanvas;
    } else {
      return false;
    }
  },

  


  set isDisplayed(isDisplayed) {
    this.elt.classList.remove("not-displayed");
    if (!isDisplayed) {
      this.elt.classList.add("not-displayed");
    }
  },

  



  _hasChildren: false,

  get hasChildren() {
    return this._hasChildren;
  },

  set hasChildren(aValue) {
    this._hasChildren = aValue;
    if (!this.expander) {
      return;
    }

    if (aValue) {
      this.expander.style.visibility = "visible";
    } else {
      this.expander.style.visibility = "hidden";
    }
  },

  



  getChildContainers: function() {
    if (!this.hasChildren) {
      return null;
    }

    return [...this.children.children].map(node => node.container);
  },

  


  get expanded() {
    return !this.elt.classList.contains("collapsed");
  },

  set expanded(aValue) {
    if (!this.expander) {
      return;
    }

    if (aValue && this.elt.classList.contains("collapsed")) {
      
      
      let closingTag = this.elt.querySelector(".close");
      if (closingTag) {
        if (!this.closeTagLine) {
          let line = this.markup.doc.createElement("div");
          line.classList.add("tag-line");

          let tagState = this.markup.doc.createElement("div");
          tagState.classList.add("tag-state");
          line.appendChild(tagState);

          line.appendChild(closingTag.cloneNode(true));

          this.closeTagLine = line;
        }
        this.elt.appendChild(this.closeTagLine);
      }

      this.elt.classList.remove("collapsed");
      this.expander.setAttribute("open", "");
      this.hovered = false;
    } else if (!aValue) {
      if (this.closeTagLine) {
        this.elt.removeChild(this.closeTagLine);
      }
      this.elt.classList.add("collapsed");
      this.expander.removeAttribute("open");
    }
  },

  parentContainer: function() {
    return this.elt.parentNode ? this.elt.parentNode.container : null;
  },

  _isMouseDown: false,
  _isDragging: false,
  _dragStartY: 0,

  set isDragging(isDragging) {
    this._isDragging = isDragging;
    this.markup.isDragging = isDragging;

    if (isDragging) {
      this.elt.classList.add("dragging");
      this.markup.doc.body.classList.add("dragging");
    } else {
      this.elt.classList.remove("dragging");
      this.markup.doc.body.classList.remove("dragging");
    }
  },

  get isDragging() {
    return this._isDragging;
  },

  _onMouseDown: function(event) {
    let target = event.target;

    
    if (target.nodeName === "button") {
      return;
    }

    
    if (target.nodeName === "a") {
      event.stopPropagation();
      event.preventDefault();
      let browserWin = this.markup._inspector.target
                           .tab.ownerDocument.defaultView;
      browserWin.openUILinkIn(target.href, "tab");
      return;
    }

    
    this._isMouseDown = true;
    this.hovered = false;
    this.markup.navigate(this);
    event.stopPropagation();
    event.preventDefault();

    
    this.markup._dragStartEl = target;
    setTimeout(() => {
      
      if (!this._isMouseDown || this.markup._dragStartEl !== target ||
          this.node.isPseudoElement || this.node.isAnonymous ||
          !this.win.getSelection().isCollapsed) {
        return;
      }
      this.isDragging = true;

      this._dragStartY = event.pageY;
      this.markup.indicateDropTarget(this.elt);

      
      this.markup.indicateDragTarget(this.elt.nextElementSibling ||
                                     this.markup.getContainer(this.node.parentNode()).closeTagLine);
    }, GRAB_DELAY);
  },

  


  _onMouseUp: function(event) {
    this._isMouseDown = false;

    if (!this.isDragging) {
      return;
    }

    this.isDragging = false;
    this.elt.style.removeProperty("top");

    let dropTargetNodes = this.markup.dropTargetNodes;

    if(!dropTargetNodes) {
      return;
    }

    this.markup.walker.insertBefore(this.node, dropTargetNodes.parent,
                                    dropTargetNodes.nextSibling);
  },

  


  _onMouseMove: function(event) {
    if (!this.isDragging) {
      return;
    }

    let diff = event.pageY - this._dragStartY;
    this.elt.style.top = diff + "px";

    let el = this.markup.doc.elementFromPoint(event.pageX - this.win.scrollX,
                                              event.pageY - this.win.scrollY);

    this.markup.indicateDropTarget(el);
  },

  



  flashMutation: function() {
    if (!this.selected) {
      let contentWin = this.win;
      flashElementOn(this.tagState, this.editor.elt);
      if (this._flashMutationTimer) {
        clearTimeout(this._flashMutationTimer);
        this._flashMutationTimer = null;
      }
      this._flashMutationTimer = setTimeout(() => {
        flashElementOff(this.tagState, this.editor.elt);
      }, this.markup.CONTAINER_FLASHING_DURATION);
    }
  },

  _hovered: false,

  



  set hovered(aValue) {
    this.tagState.classList.remove("flash-out");
    this._hovered = aValue;
    if (aValue) {
      if (!this.selected) {
        this.tagState.classList.add("theme-bg-darker");
      }
      if (this.closeTagLine) {
        this.closeTagLine.querySelector(".tag-state").classList.add(
          "theme-bg-darker");
      }
    } else {
      this.tagState.classList.remove("theme-bg-darker");
      if (this.closeTagLine) {
        this.closeTagLine.querySelector(".tag-state").classList.remove(
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
    this.tagState.classList.remove("flash-out");
    this._selected = aValue;
    this.editor.selected = aValue;
    if (this._selected) {
      this.tagLine.setAttribute("selected", "");
      this.tagState.classList.add("theme-selected");
    } else {
      this.tagLine.removeAttribute("selected");
      this.tagState.classList.remove("theme-selected");
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

  _onToggle: function(event) {
    this.markup.navigate(this);
    if (this.hasChildren) {
      this.markup.setNodeExpanded(this.node, !this.expanded, event.altKey);
    }
    event.stopPropagation();
  },

  



  destroy: function() {
    
    this.elt.removeEventListener("mousedown", this._onMouseDown, false);
    this.elt.removeEventListener("dblclick", this._onToggle, false);
    this.markup.doc.body.removeEventListener("mouseup", this._onMouseUp, true);
    this.markup.doc.body.removeEventListener("mousemove", this._onMouseMove, true);

    this.win = null;

    if (this.expander) {
      this.expander.removeEventListener("click", this._onToggle, false);
    }

    
    let firstChild;
    while (firstChild = this.children.firstChild) {
      
      
      if (firstChild.container) {
        firstChild.container.destroy();
      }
      this.children.removeChild(firstChild);
    }

    this.editor.destroy();
  }
};












function MarkupReadOnlyContainer(markupView, node) {
  MarkupContainer.prototype.initialize.call(this, markupView, node, "readonlycontainer");

  this.editor = new GenericEditor(this, node);
  this.tagLine.appendChild(this.editor.elt);
}

MarkupReadOnlyContainer.prototype = Heritage.extend(MarkupContainer.prototype, {});












function MarkupTextContainer(markupView, node) {
  MarkupContainer.prototype.initialize.call(this, markupView, node, "textcontainer");

  if (node.nodeType == Ci.nsIDOMNode.TEXT_NODE) {
    this.editor = new TextEditor(this, node, "text");
  } else if (node.nodeType == Ci.nsIDOMNode.COMMENT_NODE) {
    this.editor = new TextEditor(this, node, "comment");
  } else {
    throw "Invalid node for MarkupTextContainer";
  }

  this.tagLine.appendChild(this.editor.elt);
}

MarkupTextContainer.prototype = Heritage.extend(MarkupContainer.prototype, {});











function MarkupElementContainer(markupView, node) {
  MarkupContainer.prototype.initialize.call(this, markupView, node, "elementcontainer");

  if (node.nodeType === Ci.nsIDOMNode.ELEMENT_NODE) {
    this.editor = new ElementEditor(this, node);
  } else {
    throw "Invalid node for MarkupElementContainer";
  }

  this.tagLine.appendChild(this.editor.elt);

  
  this._prepareImagePreview();
}

MarkupElementContainer.prototype = Heritage.extend(MarkupContainer.prototype, {

  _buildEventTooltipContent: function(target, tooltip) {
    if (target.hasAttribute("data-event")) {
      tooltip.hide(target);

      this.node.getEventListenerInfo().then(listenerInfo => {
        tooltip.setEventContent({
          eventListenerInfos: listenerInfo,
          toolbox: this.markup._inspector.toolbox
        });

        this.markup._makeTooltipPersistent(true);
        tooltip.once("hidden", () => {
          this.markup._makeTooltipPersistent(false);
        });
        tooltip.show(target);
      });
      return true;
    }
  },

  






  _prepareImagePreview: function() {
    if (this.isPreviewable()) {
      
      
      let def = promise.defer();

      this.tooltipData = {
        target: this.editor.getAttributeElement("src") || this.editor.tag,
        data: def.promise
      };

      let maxDim = Services.prefs.getIntPref("devtools.inspector.imagePreviewTooltipSize");
      this.node.getImageData(maxDim).then(data => {
        data.data.string().then(str => {
          let res = {data: str, size: data.size};
          
          
          def.resolve(res);
          this.tooltipData.data = promise.resolve(res);
        });
      }, () => {
        this.tooltipData.data = promise.resolve({});
      });
    }
  },

  








  isImagePreviewTarget: function(target, tooltip) {
    if (!this.tooltipData || this.tooltipData.target !== target) {
      return promise.reject(false);
    }

    return this.tooltipData.data.then(({data, size}) => {
      if (data && size) {
        tooltip.setImageContent(data, size);
      } else {
        tooltip.setBrokenImageContent();
      }
    });
  },

  copyImageDataUri: function() {
    
    
    this.node.getImageData().then(data => {
      data.data.string().then(str => {
        clipboardHelper.copyString(str, this.markup.doc);
      });
    });
  }
});




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
  destroy: function() {},

  



  getChildContainers: function() {
    return [...this.children.children].map(node => node.container);
  }
};




function GenericEditor(aContainer, aNode) {
  this.container = aContainer;
  this.markup = this.container.markup;
  this.template = this.markup.template.bind(this.markup);
  this.elt = null;
  this.template("generic", this);

  if (aNode.isPseudoElement) {
    this.tag.classList.add("theme-fg-color5");
    this.tag.textContent = aNode.isBeforePseudoElement ? "::before" : "::after";
  } else if (aNode.nodeType == Ci.nsIDOMNode.DOCUMENT_TYPE_NODE) {
    this.elt.classList.add("comment");
    this.tag.textContent = '<!DOCTYPE ' + aNode.name +
       (aNode.publicId ? ' PUBLIC "' +  aNode.publicId + '"': '') +
       (aNode.systemId ? ' "' + aNode.systemId + '"' : '') +
       '>';
  } else {
    this.tag.textContent = aNode.nodeName;
  }
}

GenericEditor.prototype = {
  destroy: function() {
    this.elt.remove();
  }
};









function TextEditor(aContainer, aNode, aTemplate) {
  this.container = aContainer;
  this.markup = this.container.markup;
  this.node = aNode;
  this.template = this.markup.template.bind(aTemplate);
  this._selected = false;

  this.markup.template(aTemplate, this);

  editableField({
    element: this.value,
    stopOnReturn: true,
    trigger: "dblclick",
    multiline: true,
    trimOutput: false,
    done: (aVal, aCommit) => {
      if (!aCommit) {
        return;
      }
      this.node.getNodeValue().then(longstr => {
        longstr.string().then(oldValue => {
          longstr.release().then(null, console.error);

          this.container.undo.do(() => {
            this.node.setNodeValue(aVal).then(() => {
              this.markup.nodeChanged(this.node);
            });
          }, () => {
            this.node.setNodeValue(oldValue).then(() => {
              this.markup.nodeChanged(this.node);
            });
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
        text += ELLIPSIS;
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
  this.container = aContainer;
  this.node = aNode;
  this.markup = this.container.markup;
  this.template = this.markup.template.bind(this.markup);
  this.doc = this.markup.doc;

  this.attrElements = new Map();
  this.animationTimers = {};

  
  this.elt = null;
  this.tag = null;
  this.closeTag = null;
  this.attrList = null;
  this.newAttr = null;
  this.closeElt = null;

  
  this.template("element", this);

  
  
  if (!aNode.isDocumentElement) {
    this.tag.setAttribute("tabindex", "0");
    editableField({
      element: this.tag,
      trigger: "dblclick",
      stopOnReturn: true,
      done: this.onTagEdit.bind(this),
    });
  }

  
  this.newAttr.editMode = editableField({
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
        this.container.undo.do(() => {
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
  this.eventNode.style.display = this.node.hasEventListeners ? "inline-block" : "none";

  this.update();
  this.initialized = true;
}

ElementEditor.prototype = {

  flashAttribute: function(attrName) {
    if (this.animationTimers[attrName]) {
      clearTimeout(this.animationTimers[attrName]);
    }

    flashElementOn(this.getAttributeElement(attrName));

    this.animationTimers[attrName] = setTimeout(() => {
      flashElementOff(this.getAttributeElement(attrName));
    }, this.markup.CONTAINER_FLASHING_DURATION);
  },

  


  update: function() {
    let nodeAttributes = this.node.attributes || [];

    
    let currentAttributes = new Set(nodeAttributes.map(a=>a.name));
    for (let name of this.attrElements.keys()) {
      if (!currentAttributes.has(name)) {
        this.removeAttribute(name);
      }
    }

    
    
    for (let attr of nodeAttributes) {
      let el = this.attrElements.get(attr.name);
      let valueChanged = el && el.querySelector(".attr-value").innerHTML !== attr.value;
      let isEditing = el && el.querySelector(".editable").inplaceEditor;
      let canSimplyShowEditor = el && (!valueChanged || isEditing);

      if (canSimplyShowEditor) {
        
        
        el.style.removeProperty("display");
      } else {
        
        
        let attribute = this._createAttribute(attr);
        attribute.style.removeProperty("display");

        
        
        
        if (this.initialized) {
          this.flashAttribute(attr.name);
        }
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

  



  removeAttribute: function(attrName) {
    let attr = this.attrElements.get(attrName);
    if (attr) {
      this.attrElements.delete(attrName);
      attr.remove();
    }
  },

  _createAttribute: function(aAttr, aBefore = null) {
    
    let data = {
      attrName: aAttr.name,
    };
    this.template("attribute", data);
    var {attr, inner, name, val} = data;

    
    
    
    let editValueDisplayed = aAttr.value || "";
    let hasDoubleQuote = editValueDisplayed.includes('"');
    let hasSingleQuote = editValueDisplayed.includes("'");
    let initial = aAttr.name + '="' + editValueDisplayed + '"';

    
    if (hasDoubleQuote && hasSingleQuote) {
        editValueDisplayed = editValueDisplayed.replace(/\"/g, "&quot;");
        initial = aAttr.name + '="' + editValueDisplayed + '"';
    }

    
    if (hasDoubleQuote && !hasSingleQuote) {
        initial = aAttr.name + "='" + editValueDisplayed + "'";
    }

    
    attr.editMode = editableField({
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
      done: (aVal, aCommit, direction) => {
        if (!aCommit || aVal === initial) {
          return;
        }

        let doMods = this._startModifyingAttributes();
        let undoMods = this._startModifyingAttributes();

        
        
        
        try {
          this.refocusOnEdit(aAttr.name, attr, direction);
          this._saveAttribute(aAttr.name, undoMods);
          doMods.removeAttribute(aAttr.name);
          this._applyAttributes(aVal, attr, doMods, undoMods);
          this.container.undo.do(() => {
            doMods.apply();
          }, () => {
            undoMods.apply();
          });
        } catch(ex) {
          console.error(ex);
        }
      }
    });

    
    let before = aBefore;
    if (aAttr.name == "id") {
      before = this.attrList.firstChild;
    } else if (aAttr.name == "class") {
      let idNode = this.attrElements.get("id");
      before = idNode ? idNode.nextSibling : this.attrList.firstChild;
    }
    this.attrList.insertBefore(attr, before);

    this.removeAttribute(aAttr.name);
    this.attrElements.set(aAttr.name, attr);

    let collapsedValue;
    if (aAttr.value.match(COLLAPSE_DATA_URL_REGEX)) {
      collapsedValue = truncateString(aAttr.value, COLLAPSE_DATA_URL_LENGTH);
    } else {
      collapsedValue = truncateString(aAttr.value, COLLAPSE_ATTRIBUTE_LENGTH);
    }

    name.textContent = aAttr.name;
    val.textContent = collapsedValue;

    return attr;
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

  




  refocusOnEdit: function(attrName, attrNode, direction) {
    
    
    if (this._editedAttributeObserver) {
      this.markup._inspector.off("markupmutation", this._editedAttributeObserver);
      this._editedAttributeObserver = null;
    }

    let container = this.markup.getContainer(this.node);

    let activeAttrs = [...this.attrList.childNodes].filter(el => el.style.display != "none");
    let attributeIndex = activeAttrs.indexOf(attrNode);

    let onMutations = this._editedAttributeObserver = (e, mutations) => {
      let isDeletedAttribute = false;
      let isNewAttribute = false;
      for (let mutation of mutations) {
        let inContainer = this.markup.getContainer(mutation.target) === container;
        if (!inContainer) {
          continue;
        }

        let isOriginalAttribute = mutation.attributeName === attrName;

        isDeletedAttribute = isDeletedAttribute || isOriginalAttribute && mutation.newValue === null;
        isNewAttribute = isNewAttribute || mutation.attributeName !== attrName;
      }
      let isModifiedOrder = isDeletedAttribute && isNewAttribute;
      this._editedAttributeObserver = null;

      
      let visibleAttrs = [...this.attrList.childNodes].filter(el => el.style.display != "none");
      let activeEditor;
      if (visibleAttrs.length > 0) {
        if (!direction) {
          
          activeEditor = visibleAttrs[attributeIndex];
        } else if (isModifiedOrder) {
          
          
          activeEditor = visibleAttrs[0];
        } else {
          let newAttributeIndex;
          if (isDeletedAttribute) {
            newAttributeIndex = attributeIndex;
          } else {
            if (direction == Ci.nsIFocusManager.MOVEFOCUS_FORWARD) {
              newAttributeIndex = attributeIndex + 1;
            } else if (direction == Ci.nsIFocusManager.MOVEFOCUS_BACKWARD) {
              newAttributeIndex = attributeIndex - 1;
            }
          }

          
          
          if (newAttributeIndex >= 0 && newAttributeIndex <= visibleAttrs.length - 1) {
            activeEditor = visibleAttrs[newAttributeIndex];
          }
        }
      }

      
      
      if (!activeEditor) {
        activeEditor = this.newAttr;
      }

      
      
      if (direction) {
        activeEditor.editMode();
      } else {
        
        
        let editable = activeEditor === this.newAttr ? activeEditor : activeEditor.querySelector(".editable");
        editable.focus();
      }

      this.markup.emit("refocusedonedit");
    };

    
    
    this.markup._inspector.once("markupmutation", onMutations);
  },

  


  onTagEdit: function(newTagName, isCommit) {
    if (!isCommit || newTagName.toLowerCase() === this.node.tagName.toLowerCase() ||
        !("editTagName" in this.markup.walker)) {
      return;
    }

    
    
    this.markup.reselectOnRemoved(this.node, "edittagname");
    this.markup.walker.editTagName(this.node, newTagName).then(null, () => {
      
      this.markup.cancelReselectOnRemoved();
    });
  },

  destroy: function() {
    for (let key in this.animationTimers) {
      clearTimeout(this.animationTimers[key]);
    }
    this.animationTimers = null;
  }
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

  
  
  
  let el = DOMParser.parseFromString("<svg " + attr + "></svg>", "text/html").body.childNodes[0] ||
           DOMParser.parseFromString("<svg " + attr + "\"></svg>", "text/html").body.childNodes[0] ||
           DOMParser.parseFromString("<svg " + attr + "'></svg>", "text/html").body.childNodes[0];

  let div = doc.createElement("div");
  let attributes = [];
  for (let {name, value} of el.attributes) {
    
    
    try {
      div.setAttribute(name, value);
      attributes.push({ name, value });
    }
    catch(e) { }
  }

  
  return attributes.reverse();
}











function flashElementOn(backgroundElt, foregroundElt=backgroundElt) {
  if (!backgroundElt || !foregroundElt) {
    return;
  }

  
  backgroundElt.classList.remove("flash-out");

  
  backgroundElt.classList.add("theme-bg-contrast");

  foregroundElt.classList.add("theme-fg-contrast");
  [].forEach.call(
    foregroundElt.querySelectorAll("[class*=theme-fg-color]"),
    span => span.classList.add("theme-fg-contrast")
  );
}











function flashElementOff(backgroundElt, foregroundElt=backgroundElt) {
  if (!backgroundElt || !foregroundElt) {
    return;
  }

  
  backgroundElt.classList.add("flash-out");

  
  backgroundElt.classList.remove("theme-bg-contrast");

  foregroundElt.classList.remove("theme-fg-contrast");
  [].forEach.call(
    foregroundElt.querySelectorAll("[class*=theme-fg-color]"),
    span => span.classList.remove("theme-fg-contrast")
  );
}




function map(value, oldMin, oldMax, newMin, newMax) {
  let ratio = oldMax - oldMin;
  if (ratio == 0) {
    return value;
  }
  return newMin + (newMax - newMin) * ((value - oldMin) / ratio);
}

loader.lazyGetter(MarkupView.prototype, "strings", () => Services.strings.createBundle(
  "chrome://browser/locale/devtools/inspector.properties"
));

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});
