





"use strict";

const {Cc, Cu, Ci} = require("chrome");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/DOMHelpers.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
const {Promise: promise} = require("resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const ENSURE_SELECTION_VISIBLE_DELAY = 50; 
const ELLIPSIS = Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data;
const MAX_LABEL_LENGTH = 40;
const LOW_PRIORITY_ELEMENTS = {
  "HEAD": true,
  "BASE": true,
  "BASEFONT": true,
  "ISINDEX": true,
  "LINK": true,
  "META": true,
  "SCRIPT": true,
  "STYLE": true,
  "TITLE": true,
};















function HTMLBreadcrumbs(inspector) {
  this.inspector = inspector;
  this.selection = this.inspector.selection;
  this.chromeWin = this.inspector.panelWin;
  this.chromeDoc = this.inspector.panelDoc;
  this.DOMHelpers = new DOMHelpers(this.chromeWin);
  this._init();
}

exports.HTMLBreadcrumbs = HTMLBreadcrumbs;

HTMLBreadcrumbs.prototype = {
  get walker() this.inspector.walker,

  _init: function() {
    this.container = this.chromeDoc.getElementById("inspector-breadcrumbs");

    
    
    this.separators = this.chromeDoc.createElement("box");
    this.separators.className = "breadcrumb-separator-container";
    this.separators.innerHTML =
                      "<box id='breadcrumb-separator-before'></box>" +
                      "<box id='breadcrumb-separator-after'></box>" +
                      "<box id='breadcrumb-separator-normal'></box>";
    this.container.parentNode.appendChild(this.separators);

    this.container.addEventListener("mousedown", this, true);
    this.container.addEventListener("keypress", this, true);
    this.container.addEventListener("mouseover", this, true);
    this.container.addEventListener("mouseleave", this, true);

    
    this.nodeHierarchy = [];

    
    this.currentIndex = -1;

    
    
    this.container.removeAttribute("overflows");
    this.container._scrollButtonUp.collapsed = true;
    this.container._scrollButtonDown.collapsed = true;

    this.onscrollboxreflow = () => {
      if (this.container._scrollButtonDown.collapsed)
        this.container.removeAttribute("overflows");
      else
        this.container.setAttribute("overflows", true);
    };

    this.container.addEventListener("underflow", this.onscrollboxreflow, false);
    this.container.addEventListener("overflow", this.onscrollboxreflow, false);

    this.update = this.update.bind(this);
    this.updateSelectors = this.updateSelectors.bind(this);
    this.selection.on("new-node-front", this.update);
    this.selection.on("pseudoclass", this.updateSelectors);
    this.selection.on("attribute-changed", this.updateSelectors);
    this.inspector.on("markupmutation", this.update);
    this.update();
  },

  




  selectionGuard: function() {
    let selection = this.selection.nodeFront;
    return (result) => {
      if (selection != this.selection.nodeFront) {
        return promise.reject("selection-changed");
      }
      return result;
    }
  },

  



  selectionGuardEnd: function(err) {
    if (err === "selection-changed") {
      console.warn("Asynchronous operation was aborted as selection changed.");
    } else {
      console.error(err);
    }
  },

  




  prettyPrintNodeAsText: function(node) {
    let text = node.tagName.toLowerCase();
    if (node.isPseudoElement) {
      text = node.isBeforePseudoElement ? "::before" : "::after";
    }

    if (node.id) {
      text += "#" + node.id;
    }

    if (node.className) {
      let classList = node.className.split(/\s+/);
      for (let i = 0; i < classList.length; i++) {
        text += "." + classList[i];
      }
    }

    for (let pseudo of node.pseudoClassLocks) {
      text += pseudo;
    }

    return text;
  },

  







  prettyPrintNodeAsXUL: function(node) {
    let fragment = this.chromeDoc.createDocumentFragment();

    let tagLabel = this.chromeDoc.createElement("label");
    tagLabel.className = "breadcrumbs-widget-item-tag plain";

    let idLabel = this.chromeDoc.createElement("label");
    idLabel.className = "breadcrumbs-widget-item-id plain";

    let classesLabel = this.chromeDoc.createElement("label");
    classesLabel.className = "breadcrumbs-widget-item-classes plain";

    let pseudosLabel = this.chromeDoc.createElement("label");
    pseudosLabel.className = "breadcrumbs-widget-item-pseudo-classes plain";

    let tagText = node.tagName.toLowerCase();
    if (node.isPseudoElement) {
      tagText = node.isBeforePseudoElement ? "::before" : "::after";
    }
    let idText = node.id ? ("#" + node.id) : "";
    let classesText = "";

    if (node.className) {
      let classList = node.className.split(/\s+/);
      for (let i = 0; i < classList.length; i++) {
        classesText += "." + classList[i];
      }
    }

    
    for (let pseudo of node.pseudoClassLocks) {

    }

    
    
    
    let maxTagLength = MAX_LABEL_LENGTH;
    let maxIdLength = MAX_LABEL_LENGTH - tagText.length;
    let maxClassLength = MAX_LABEL_LENGTH - tagText.length - idText.length;

    if (tagText.length > maxTagLength) {
       tagText = tagText.substr(0, maxTagLength) + ELLIPSIS;
       idText = classesText = "";
    } else if (idText.length > maxIdLength) {
       idText = idText.substr(0, maxIdLength) + ELLIPSIS;
       classesText = "";
    } else if (classesText.length > maxClassLength) {
      classesText = classesText.substr(0, maxClassLength) + ELLIPSIS;
    }

    tagLabel.textContent = tagText;
    idLabel.textContent = idText;
    classesLabel.textContent = classesText;
    pseudosLabel.textContent = node.pseudoClassLocks.join("");

    fragment.appendChild(tagLabel);
    fragment.appendChild(idLabel);
    fragment.appendChild(classesLabel);
    fragment.appendChild(pseudosLabel);

    return fragment;
  },

  




  openSiblingMenu: function(button, node) {
    
    
    
    this.selection.setNodeFront(node, "breadcrumbs");

    let title = this.chromeDoc.createElement("menuitem");
    title.setAttribute("label", this.inspector.strings.GetStringFromName("breadcrumbs.siblings"));
    title.setAttribute("disabled", "true");

    let separator = this.chromeDoc.createElement("menuseparator");

    let items = [title, separator];

    this.walker.siblings(node, {
      whatToShow: Ci.nsIDOMNodeFilter.SHOW_ELEMENT
    }).then(siblings => {
      let nodes = siblings.nodes;
      for (let i = 0; i < nodes.length; i++) {
        let item = this.chromeDoc.createElement("menuitem");
        if (nodes[i] === node) {
          item.setAttribute("disabled", "true");
          item.setAttribute("checked", "true");
        }

        item.setAttribute("type", "radio");
        item.setAttribute("label", this.prettyPrintNodeAsText(nodes[i]));

        let selection = this.selection;
        item.onmouseup = (function(node) {
          return function() {
            selection.setNodeFront(node, "breadcrumbs");
          }
        })(nodes[i]);

        items.push(item);
        this.inspector.showNodeMenu(button, "before_start", items);
      }
    });
  },

  



  handleEvent: function(event) {
    if (event.type == "mousedown" && event.button == 0) {
      this.handleMouseDown(event);
    } else if (event.type == "keypress" && this.selection.isElementNode()) {
      this.handleKeyPress(event);
    } else if (event.type == "mouseover") {
      this.handleMouseOver(event);
    } else if (event.type == "mouseleave") {
      this.handleMouseLeave(event);
    }
  },

  



  handleMouseDown: function(event) {
    let timer;
    let container = this.container;

    function openMenu(event) {
      cancelHold();
      let target = event.originalTarget;
      if (target.tagName == "button") {
        target.onBreadcrumbsHold();
      }
    }

    function handleClick(event) {
      cancelHold();
      let target = event.originalTarget;
      if (target.tagName == "button") {
        target.onBreadcrumbsClick();
      }
    }

    let window = this.chromeWin;
    function cancelHold(event) {
      window.clearTimeout(timer);
      container.removeEventListener("mouseout", cancelHold, false);
      container.removeEventListener("mouseup", handleClick, false);
    }

    container.addEventListener("mouseout", cancelHold, false);
    container.addEventListener("mouseup", handleClick, false);
    timer = window.setTimeout(openMenu, 500, event);
  },

  



  handleMouseOver: function(event) {
    let target = event.originalTarget;
    if (target.tagName == "button") {
      target.onBreadcrumbsHover();
    }
  },

  



  handleMouseLeave: function(event) {
    this.inspector.toolbox.highlighterUtils.unhighlight();
  },

  



  handleKeyPress: function(event) {
    let node = null;
    this._keyPromise = this._keyPromise || promise.resolve(null);

    this._keyPromise = (this._keyPromise || promise.resolve(null)).then(() => {
      switch (event.keyCode) {
        case this.chromeWin.KeyEvent.DOM_VK_LEFT:
          if (this.currentIndex != 0) {
            node = promise.resolve(this.nodeHierarchy[this.currentIndex - 1].node);
          }
          break;
        case this.chromeWin.KeyEvent.DOM_VK_RIGHT:
          if (this.currentIndex < this.nodeHierarchy.length - 1) {
            node = promise.resolve(this.nodeHierarchy[this.currentIndex + 1].node);
          }
          break;
        case this.chromeWin.KeyEvent.DOM_VK_UP:
          node = this.walker.previousSibling(this.selection.nodeFront, {
            whatToShow: Ci.nsIDOMNodeFilter.SHOW_ELEMENT
          });
          break;
        case this.chromeWin.KeyEvent.DOM_VK_DOWN:
          node = this.walker.nextSibling(this.selection.nodeFront, {
            whatToShow: Ci.nsIDOMNodeFilter.SHOW_ELEMENT
          });
          break;
      }

      return node.then((node) => {
        if (node) {
          this.selection.setNodeFront(node, "breadcrumbs");
        }
      });
    });
    event.preventDefault();
    event.stopPropagation();
  },

  


  destroy: function() {
    this.selection.off("new-node-front", this.update);
    this.selection.off("pseudoclass", this.updateSelectors);
    this.selection.off("attribute-changed", this.updateSelectors);
    this.inspector.off("markupmutation", this.update);

    this.container.removeEventListener("underflow", this.onscrollboxreflow, false);
    this.container.removeEventListener("overflow", this.onscrollboxreflow, false);
    this.container.removeEventListener("mousedown", this, true);
    this.container.removeEventListener("keypress", this, true);
    this.container.removeEventListener("mouseover", this, true);
    this.container.removeEventListener("mouseleave", this, true);

    this.empty();
    this.separators.remove();

    this.onscrollboxreflow = null;
    this.container = null;
    this.separators = null;
    this.nodeHierarchy = null;

    this.isDestroyed = true;
  },

  


  empty: function() {
    while (this.container.hasChildNodes()) {
      this.container.firstChild.remove();
    }
  },

  



  setCursor: function(index) {
    
    if (this.currentIndex > -1 && this.currentIndex < this.nodeHierarchy.length) {
      this.nodeHierarchy[this.currentIndex].button.removeAttribute("checked");
    }
    if (index > -1) {
      this.nodeHierarchy[index].button.setAttribute("checked", "true");
      if (this.hadFocus)
        this.nodeHierarchy[index].button.focus();
    }
    this.currentIndex = index;
  },

  




  indexOf: function(node) {
    let i = this.nodeHierarchy.length - 1;
    for (let i = this.nodeHierarchy.length - 1; i >= 0; i--) {
      if (this.nodeHierarchy[i].node === node) {
        return i;
      }
    }
    return -1;
  },

  




  cutAfter: function(index) {
    while (this.nodeHierarchy.length > (index + 1)) {
      let toRemove = this.nodeHierarchy.pop();
      this.container.removeChild(toRemove.button);
    }
  },

  




  buildButton: function(node) {
    let button = this.chromeDoc.createElement("button");
    button.appendChild(this.prettyPrintNodeAsXUL(node));
    button.className = "breadcrumbs-widget-item";

    button.setAttribute("tooltiptext", this.prettyPrintNodeAsText(node));

    button.onkeypress = function onBreadcrumbsKeypress(e) {
      if (e.charCode == Ci.nsIDOMKeyEvent.DOM_VK_SPACE ||
          e.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN)
        button.click();
    }

    button.onBreadcrumbsClick = () => {
      this.selection.setNodeFront(node, "breadcrumbs");
    };

    button.onBreadcrumbsHover = () => {
      this.inspector.toolbox.highlighterUtils.highlightNodeFront(node);
    };

    button.onclick = (function _onBreadcrumbsRightClick(event) {
      button.focus();
      if (event.button == 2) {
        this.openSiblingMenu(button, node);
      }
    }).bind(this);

    button.onBreadcrumbsHold = (function _onBreadcrumbsHold() {
      this.openSiblingMenu(button, node);
    }).bind(this);
    return button;
  },

  



  expand: function(node) {
    let fragment = this.chromeDoc.createDocumentFragment();
    let lastButtonInserted = null;
    let originalLength = this.nodeHierarchy.length;
    let stopNode = null;
    if (originalLength > 0) {
      stopNode = this.nodeHierarchy[originalLength - 1].node;
    }
    while (node && node != stopNode) {
      if (node.tagName) {
        let button = this.buildButton(node);
        fragment.insertBefore(button, lastButtonInserted);
        lastButtonInserted = button;
        this.nodeHierarchy.splice(originalLength, 0, {
          node,
          button,
          currentPrettyPrintText: this.prettyPrintNodeAsText(node)
        });
      }
      node = node.parentNode();
    }
    this.container.appendChild(fragment, this.container.firstChild);
  },

  





  getInterestingFirstNode: function(node) {
    let deferred = promise.defer();

    let fallback = null;

    let moreChildren = () => {
      this.walker.children(node, {
        start: fallback,
        maxNodes: 10,
        whatToShow: Ci.nsIDOMNodeFilter.SHOW_ELEMENT
      }).then(this.selectionGuard()).then(response => {
        for (let node of response.nodes) {
          if (!(node.tagName in LOW_PRIORITY_ELEMENTS)) {
            deferred.resolve(node);
            return;
          }
          if (!fallback) {
            fallback = node;
          }
        }
        if (response.hasLast) {
          deferred.resolve(fallback);
          return;
        } else {
          moreChildren();
        }
      }).then(null, this.selectionGuardEnd);
    };

    moreChildren();
    return deferred.promise;
  },

  




  getCommonAncestor: function(node) {
    while (node) {
      let idx = this.indexOf(node);
      if (idx > -1) {
        return idx;
      } else {
        node = node.parentNode();
      }
    }
    return -1;
  },

  




  ensureFirstChild: function() {
    
    if (this.currentIndex == this.nodeHierarchy.length - 1) {
      let node = this.nodeHierarchy[this.currentIndex].node;
      return this.getInterestingFirstNode(node).then(child => {
        
        if (child && !this.isDestroyed) {
          
          this.expand(child);
        }
      });
    }

    return resolveNextTick(true);
  },

  


  scroll: function() {
    

    let scrollbox = this.container;
    let element = this.nodeHierarchy[this.currentIndex].button;

    
    
    this.chromeWin.clearTimeout(this._ensureVisibleTimeout);
    this._ensureVisibleTimeout = this.chromeWin.setTimeout(function() {
      scrollbox.ensureElementIsVisible(element);
    }, ENSURE_SELECTION_VISIBLE_DELAY);
  },

  


  updateSelectors: function() {
    if (this.isDestroyed) {
      return;
    }

    for (let i = this.nodeHierarchy.length - 1; i >= 0; i--) {
      let {node, button, currentPrettyPrintText} = this.nodeHierarchy[i];

      
      let textOutput = this.prettyPrintNodeAsText(node);
      if (currentPrettyPrintText === textOutput) {
        continue;
      }

      
      while (button.hasChildNodes()) {
        button.firstChild.remove();
      }
      button.appendChild(this.prettyPrintNodeAsXUL(node));
      button.setAttribute("tooltiptext", textOutput);

      this.nodeHierarchy[i].currentPrettyPrintText = textOutput;
    }
  },

  







  _hasInterestingMutations: function(mutations) {
    if (!mutations || !mutations.length) {
      return false;
    }

    for (let {type, added, removed, target, attributeName} of mutations) {
      if (type === "childList") {
        
        
        
        return added.some(node => this.indexOf(node) > -1) ||
               removed.some(node => this.indexOf(node) > -1) ||
               this.indexOf(target) === this.nodeHierarchy.length - 1;
      } else if (type === "attributes" && this.indexOf(target) > -1) {
        
        
        return attributeName === "class" || attributeName === "id";
      }
    }

    
    
    return false;
  },

  





  update: function(reason, mutations) {
    if (this.isDestroyed) {
      return;
    }

    if (reason !== "markupmutation") {
      this.inspector.hideNodeMenu();
    }

    let hasInterestingMutations = this._hasInterestingMutations(mutations);
    if (reason === "markupmutation" && !hasInterestingMutations) {
      return;
    }

    let cmdDispatcher = this.chromeDoc.commandDispatcher;
    this.hadFocus = (cmdDispatcher.focusedElement &&
                     cmdDispatcher.focusedElement.parentNode == this.container);

    if (!this.selection.isConnected()) {
      this.cutAfter(-1); 
      return;
    }

    if (!this.selection.isElementNode()) {
      this.setCursor(-1); 
      return;
    }

    let idx = this.indexOf(this.selection.nodeFront);

    
    
    if (idx > -1 && !hasInterestingMutations) {
      
      this.setCursor(idx);
    } else {
      
      if (this.nodeHierarchy.length > 0) {
        
        
        let parent = this.selection.nodeFront.parentNode();
        let idx = this.getCommonAncestor(parent);
        this.cutAfter(idx);
      }
      
      
      this.expand(this.selection.nodeFront);

      
      idx = this.indexOf(this.selection.nodeFront);
      this.setCursor(idx);
    }

    let doneUpdating = this.inspector.updating("breadcrumbs");
    
    this.ensureFirstChild().then(this.selectionGuard()).then(() => {
      if (this.isDestroyed) {
        return;
      }

      this.updateSelectors();

      
      this.scroll();
      return resolveNextTick().then(() => {
        this.inspector.emit("breadcrumbs-updated", this.selection.nodeFront);
        doneUpdating();
      });
    }).then(null, err => {
      doneUpdating(this.selection.nodeFront);
      this.selectionGuardEnd(err);
    });
  }
};




function resolveNextTick(value) {
  let deferred = promise.defer();
  Services.tm.mainThread.dispatch(() => {
    try {
      deferred.resolve(value);
    } catch(e) {
      deferred.reject(e);
    }
  }, Ci.nsIThread.DISPATCH_NORMAL);
  return deferred.promise;
}
