





const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const ENSURE_SELECTION_VISIBLE_DELAY = 50; 

this.EXPORTED_SYMBOLS = ["HTMLBreadcrumbs"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/DOMHelpers.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");

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
















this.HTMLBreadcrumbs = function HTMLBreadcrumbs(aInspector)
{
  this.inspector = aInspector;
  this.selection = this.inspector.selection;
  this.chromeWin = this.inspector.panelWin;
  this.chromeDoc = this.inspector.panelDoc;
  this.DOMHelpers = new DOMHelpers(this.chromeWin);
  this._init();
}

HTMLBreadcrumbs.prototype = {
  _init: function BC__init()
  {
    this.container = this.chromeDoc.getElementById("inspector-breadcrumbs");
    this.container.addEventListener("mousedown", this, true);
    this.container.addEventListener("keypress", this, true);

    
    this.nodeHierarchy = [];

    
    this.currentIndex = -1;

    
    
    this.container.removeAttribute("overflows");
    this.container._scrollButtonUp.collapsed = true;
    this.container._scrollButtonDown.collapsed = true;

    this.onscrollboxreflow = function() {
      if (this.container._scrollButtonDown.collapsed)
        this.container.removeAttribute("overflows");
      else
        this.container.setAttribute("overflows", true);
    }.bind(this);

    this.container.addEventListener("underflow", this.onscrollboxreflow, false);
    this.container.addEventListener("overflow", this.onscrollboxreflow, false);

    this.update = this.update.bind(this);
    this.updateSelectors = this.updateSelectors.bind(this);
    this.selection.on("new-node", this.update);
    this.selection.on("pseudoclass", this.updateSelectors);
    this.selection.on("attribute-changed", this.updateSelectors);
    this.update();
  },

  





  prettyPrintNodeAsText: function BC_prettyPrintNodeText(aNode)
  {
    let text = aNode.tagName.toLowerCase();
    if (aNode.id) {
      text += "#" + aNode.id;
    }
    for (let i = 0; i < aNode.classList.length; i++) {
      text += "." + aNode.classList[i];
    }
    for (let i = 0; i < PSEUDO_CLASSES.length; i++) {
      let pseudo = PSEUDO_CLASSES[i];
      if (DOMUtils.hasPseudoClassLock(aNode, pseudo)) {
        text += pseudo;
      }
    }

    return text;
  },


  








  prettyPrintNodeAsXUL: function BC_prettyPrintNodeXUL(aNode)
  {
    let fragment = this.chromeDoc.createDocumentFragment();

    let tagLabel = this.chromeDoc.createElement("label");
    tagLabel.className = "breadcrumbs-widget-item-tag plain";

    let idLabel = this.chromeDoc.createElement("label");
    idLabel.className = "breadcrumbs-widget-item-id plain";

    let classesLabel = this.chromeDoc.createElement("label");
    classesLabel.className = "breadcrumbs-widget-item-classes plain";

    let pseudosLabel = this.chromeDoc.createElement("label");
    pseudosLabel.className = "breadcrumbs-widget-item-pseudo-classes plain";

    tagLabel.textContent = aNode.tagName.toLowerCase();
    idLabel.textContent = aNode.id ? ("#" + aNode.id) : "";

    let classesText = "";
    for (let i = 0; i < aNode.classList.length; i++) {
      classesText += "." + aNode.classList[i];
    }
    classesLabel.textContent = classesText;

    let pseudos = PSEUDO_CLASSES.filter(function(pseudo) {
      return DOMUtils.hasPseudoClassLock(aNode, pseudo);
    }, this);
    pseudosLabel.textContent = pseudos.join("");

    fragment.appendChild(tagLabel);
    fragment.appendChild(idLabel);
    fragment.appendChild(classesLabel);
    fragment.appendChild(pseudosLabel);

    return fragment;
  },

  





  openSiblingMenu: function BC_openSiblingMenu(aButton, aNode)
  {
    
    
    
    this.selection.setNode(aNode, "breadcrumbs");

    let title = this.chromeDoc.createElement("menuitem");
    title.setAttribute("label", this.inspector.strings.GetStringFromName("breadcrumbs.siblings"));
    title.setAttribute("disabled", "true");

    let separator = this.chromeDoc.createElement("menuseparator");

    let items = [title, separator];

    let nodes = aNode.parentNode.childNodes;
    for (let i = 0; i < nodes.length; i++) {
      if (nodes[i].nodeType == aNode.ELEMENT_NODE) {
        let item = this.chromeDoc.createElement("menuitem");
        if (nodes[i] === aNode) {
          item.setAttribute("disabled", "true");
          item.setAttribute("checked", "true");
        }

        item.setAttribute("type", "radio");
        item.setAttribute("label", this.prettyPrintNodeAsText(nodes[i]));

        let selection = this.selection;
        item.onmouseup = (function(aNode) {
          return function() {
            selection.setNode(aNode, "breadcrumbs");
          }
        })(nodes[i]);

        items.push(item);
      }
    }
    this.inspector.showNodeMenu(aButton, "before_start", items);
  },

  





  handleEvent: function BC_handleEvent(event)
  {
    if (event.type == "mousedown" && event.button == 0) {
      

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
    }

    if (event.type == "keypress" && this.selection.isElementNode()) {
      let node = null;
      switch (event.keyCode) {
        case this.chromeWin.KeyEvent.DOM_VK_LEFT:
          if (this.currentIndex != 0) {
            node = this.nodeHierarchy[this.currentIndex - 1].node;
          }
          break;
        case this.chromeWin.KeyEvent.DOM_VK_RIGHT:
          if (this.currentIndex < this.nodeHierarchy.length - 1) {
            node = this.nodeHierarchy[this.currentIndex + 1].node;
          }
          break;
        case this.chromeWin.KeyEvent.DOM_VK_UP:
          node = this.selection.node.previousSibling;
          while (node && (node.nodeType != node.ELEMENT_NODE)) {
            node = node.previousSibling;
          }
          break;
        case this.chromeWin.KeyEvent.DOM_VK_DOWN:
          node = this.selection.node.nextSibling;
          while (node && (node.nodeType != node.ELEMENT_NODE)) {
            node = node.nextSibling;
          }
          break;
      }
      if (node) {
        this.selection.setNode(node, "breadcrumbs");
      }
      event.preventDefault();
      event.stopPropagation();
    }
  },

  


  destroy: function BC_destroy()
  {
    this.nodeHierarchy.forEach(function(crumb) {
      if (LayoutHelpers.isNodeConnected(crumb.node)) {
        DOMUtils.clearPseudoClassLocks(crumb.node);
      }
    });

    this.selection.off("new-node", this.update);
    this.selection.off("pseudoclass", this.updateSelectors);
    this.selection.off("attribute-changed", this.updateSelectors);

    this.container.removeEventListener("underflow", this.onscrollboxreflow, false);
    this.container.removeEventListener("overflow", this.onscrollboxreflow, false);
    this.onscrollboxreflow = null;

    this.empty();
    this.container.removeEventListener("mousedown", this, true);
    this.container.removeEventListener("keypress", this, true);
    this.container = null;
    this.nodeHierarchy = null;
  },

  


  empty: function BC_empty()
  {
    while (this.container.hasChildNodes()) {
      this.container.removeChild(this.container.firstChild);
    }
  },

  


  invalidateHierarchy: function BC_invalidateHierarchy()
  {
    this.inspector.hideNodeMenu();
    this.nodeHierarchy = [];
    this.empty();
  },

  




  setCursor: function BC_setCursor(aIdx)
  {
    
    if (this.currentIndex > -1 && this.currentIndex < this.nodeHierarchy.length) {
      this.nodeHierarchy[this.currentIndex].button.removeAttribute("checked");
    }
    if (aIdx > -1) {
      this.nodeHierarchy[aIdx].button.setAttribute("checked", "true");
      if (this.hadFocus)
        this.nodeHierarchy[aIdx].button.focus();
    }
    this.currentIndex = aIdx;
  },

  





  indexOf: function BC_indexOf(aNode)
  {
    let i = this.nodeHierarchy.length - 1;
    for (let i = this.nodeHierarchy.length - 1; i >= 0; i--) {
      if (this.nodeHierarchy[i].node === aNode) {
        return i;
      }
    }
    return -1;
  },

  





  cutAfter: function BC_cutAfter(aIdx)
  {
    while (this.nodeHierarchy.length > (aIdx + 1)) {
      let toRemove = this.nodeHierarchy.pop();
      this.container.removeChild(toRemove.button);
    }
  },

  





  buildButton: function BC_buildButton(aNode)
  {
    let button = this.chromeDoc.createElement("button");
    button.appendChild(this.prettyPrintNodeAsXUL(aNode));
    button.className = "breadcrumbs-widget-item";

    button.setAttribute("tooltiptext", this.prettyPrintNodeAsText(aNode));

    button.onkeypress = function onBreadcrumbsKeypress(e) {
      if (e.charCode == Ci.nsIDOMKeyEvent.DOM_VK_SPACE ||
          e.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN)
        button.click();
    }

    button.onBreadcrumbsClick = function onBreadcrumbsClick() {
      this.selection.setNode(aNode, "breadcrumbs");
    }.bind(this);

    button.onclick = (function _onBreadcrumbsRightClick(event) {
      button.focus();
      if (event.button == 2) {
        this.openSiblingMenu(button, aNode);
      }
    }).bind(this);

    button.onBreadcrumbsHold = (function _onBreadcrumbsHold() {
      this.openSiblingMenu(button, aNode);
    }).bind(this);
    return button;
  },

  




  expand: function BC_expand(aNode)
  {
      let fragment = this.chromeDoc.createDocumentFragment();
      let toAppend = aNode;
      let lastButtonInserted = null;
      let originalLength = this.nodeHierarchy.length;
      let stopNode = null;
      if (originalLength > 0) {
        stopNode = this.nodeHierarchy[originalLength - 1].node;
      }
      while (toAppend && toAppend.tagName && toAppend != stopNode) {
        let button = this.buildButton(toAppend);
        fragment.insertBefore(button, lastButtonInserted);
        lastButtonInserted = button;
        this.nodeHierarchy.splice(originalLength, 0, {node: toAppend, button: button});
        toAppend = this.DOMHelpers.getParentObject(toAppend);
      }
      this.container.appendChild(fragment, this.container.firstChild);
  },

  






  getInterestingFirstNode: function BC_getInterestingFirstNode(aNode)
  {
    let nextChild = this.DOMHelpers.getChildObject(aNode, 0);
    let fallback = null;

    while (nextChild) {
      if (nextChild.nodeType == aNode.ELEMENT_NODE) {
        if (!(nextChild.tagName in LOW_PRIORITY_ELEMENTS)) {
          return nextChild;
        }
        if (!fallback) {
          fallback = nextChild;
        }
      }
      nextChild = this.DOMHelpers.getNextSibling(nextChild);
    }
    return fallback;
  },


  





  getCommonAncestor: function BC_getCommonAncestor(aNode)
  {
    let node = aNode;
    while (node) {
      let idx = this.indexOf(node);
      if (idx > -1) {
        return idx;
      } else {
        node = this.DOMHelpers.getParentObject(node);
      }
    }
    return -1;
  },

  



  ensureFirstChild: function BC_ensureFirstChild()
  {
    
    if (this.currentIndex == this.nodeHierarchy.length - 1) {
      let node = this.nodeHierarchy[this.currentIndex].node;
      let child = this.getInterestingFirstNode(node);
      
      if (child) {
        
        this.expand(child);
      }
    }
  },

  


  scroll: function BC_scroll()
  {
    

    let scrollbox = this.container;
    let element = this.nodeHierarchy[this.currentIndex].button;

    
    
    this.chromeWin.clearTimeout(this._ensureVisibleTimeout);
    this._ensureVisibleTimeout = this.chromeWin.setTimeout(function() {
      scrollbox.ensureElementIsVisible(element);
    }, ENSURE_SELECTION_VISIBLE_DELAY);
  },

  updateSelectors: function BC_updateSelectors()
  {
    for (let i = this.nodeHierarchy.length - 1; i >= 0; i--) {
      let crumb = this.nodeHierarchy[i];
      let button = crumb.button;

      while(button.hasChildNodes()) {
        button.removeChild(button.firstChild);
      }
      button.appendChild(this.prettyPrintNodeAsXUL(crumb.node));
      button.setAttribute("tooltiptext", this.prettyPrintNodeAsText(crumb.node));
    }
  },

  


  update: function BC_update()
  {
    this.inspector.hideNodeMenu();

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

    let idx = this.indexOf(this.selection.node);

    
    if (idx > -1) {
      
      this.setCursor(idx);
    } else {
      
      if (this.nodeHierarchy.length > 0) {
        
        
        let parent = this.DOMHelpers.getParentObject(this.selection.node);
        let idx = this.getCommonAncestor(parent);
        this.cutAfter(idx);
      }
      
      
      this.expand(this.selection.node);

      
      idx = this.indexOf(this.selection.node);
      this.setCursor(idx);
    }
    
    this.ensureFirstChild();
    this.updateSelectors();

    
    this.scroll();
  },
}

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
