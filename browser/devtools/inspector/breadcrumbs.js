





const {Cc, Cu, Ci} = require("chrome");

const PSEUDO_CLASSES = [":hover", ":active", ":focus"];
const ENSURE_SELECTION_VISIBLE_DELAY = 50; 

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/DOMHelpers.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
const ELLIPSIS = Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data;
const MAX_LABEL_LENGTH = 40;

let promise = require("devtools/toolkit/deprecated-sync-thenables");

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

function resolveNextTick(value) {
  let deferred = promise.defer();
  Services.tm.mainThread.dispatch(() => {
    try {
      deferred.resolve(value);
    } catch(ex) {
      console.error(ex);
    }
  }, Ci.nsIThread.DISPATCH_NORMAL);
  return deferred.promise;
}
















function HTMLBreadcrumbs(aInspector)
{
  this.inspector = aInspector;
  this.selection = this.inspector.selection;
  this.chromeWin = this.inspector.panelWin;
  this.chromeDoc = this.inspector.panelDoc;
  this.DOMHelpers = new DOMHelpers(this.chromeWin);
  this._init();
}

exports.HTMLBreadcrumbs = HTMLBreadcrumbs;

HTMLBreadcrumbs.prototype = {
  get walker() this.inspector.walker,

  _init: function BC__init()
  {
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
    if (err != "selection-changed") {
      console.error(err);
    }
    promise.reject(err);
  },

  





  prettyPrintNodeAsText: function BC_prettyPrintNodeText(aNode)
  {
    let text = aNode.tagName.toLowerCase();
    if (aNode.id) {
      text += "#" + aNode.id;
    }

    if (aNode.className) {
      let classList = aNode.className.split(/\s+/);
      for (let i = 0; i < classList.length; i++) {
        text += "." + classList[i];
      }
    }

    for (let pseudo of aNode.pseudoClassLocks) {
      text += pseudo;
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

    let tagText = aNode.tagName.toLowerCase();
    let idText = aNode.id ? ("#" + aNode.id) : "";
    let classesText = "";

    if (aNode.className) {
      let classList = aNode.className.split(/\s+/);
      for (let i = 0; i < classList.length; i++) {
        classesText += "." + classList[i];
      }
    }

    
    for (let pseudo of aNode.pseudoClassLocks) {

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
    pseudosLabel.textContent = aNode.pseudoClassLocks.join("");

    fragment.appendChild(tagLabel);
    fragment.appendChild(idLabel);
    fragment.appendChild(classesLabel);
    fragment.appendChild(pseudosLabel);

    return fragment;
  },

  





  openSiblingMenu: function BC_openSiblingMenu(aButton, aNode)
  {
    
    
    
    this.selection.setNodeFront(aNode, "breadcrumbs");

    let title = this.chromeDoc.createElement("menuitem");
    title.setAttribute("label", this.inspector.strings.GetStringFromName("breadcrumbs.siblings"));
    title.setAttribute("disabled", "true");

    let separator = this.chromeDoc.createElement("menuseparator");

    let items = [title, separator];

    this.walker.siblings(aNode, {
      whatToShow: Ci.nsIDOMNodeFilter.SHOW_ELEMENT
    }).then(siblings => {
      let nodes = siblings.nodes;
      for (let i = 0; i < nodes.length; i++) {
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
            selection.setNodeFront(aNode, "breadcrumbs");
          }
        })(nodes[i]);

        items.push(item);
        this.inspector.showNodeMenu(aButton, "before_start", items);
      }
    });
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
    }

    if (event.type == "mouseover") {
      let target = event.originalTarget;
      if (target.tagName == "button") {
        target.onBreadcrumbsHover();
      }
    }

    if (event.type == "mouseleave") {
      this.inspector.toolbox.highlighterUtils.unhighlight();
    }
  },

  


  destroy: function BC_destroy()
  {
    this.selection.off("new-node-front", this.update);
    this.selection.off("pseudoclass", this.updateSelectors);
    this.selection.off("attribute-changed", this.updateSelectors);
    this.inspector.off("markupmutation", this.update);

    this.container.removeEventListener("underflow", this.onscrollboxreflow, false);
    this.container.removeEventListener("overflow", this.onscrollboxreflow, false);
    this.onscrollboxreflow = null;

    this.empty();
    this.container.removeEventListener("mousedown", this, true);
    this.container.removeEventListener("keypress", this, true);
    this.container.removeEventListener("mouseover", this, true);
    this.container.removeEventListener("mouseleave", this, true);
    this.container = null;

    this.separators.remove();
    this.separators = null;

    this.nodeHierarchy = null;

    this.isDestroyed = true;
  },

  


  empty: function BC_empty()
  {
    while (this.container.hasChildNodes()) {
      this.container.removeChild(this.container.firstChild);
    }
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

    button.onBreadcrumbsClick = () => {
      this.selection.setNodeFront(aNode, "breadcrumbs");
    };

    button.onBreadcrumbsHover = () => {
      this.inspector.toolbox.highlighterUtils.highlightNodeFront(aNode);
    };

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
      while (toAppend && toAppend != stopNode) {
        if (toAppend.tagName) {
          let button = this.buildButton(toAppend);
          fragment.insertBefore(button, lastButtonInserted);
          lastButtonInserted = button;
          this.nodeHierarchy.splice(originalLength, 0, {node: toAppend, button: button});
        }
        toAppend = toAppend.parentNode();
      }
      this.container.appendChild(fragment, this.container.firstChild);
  },

  






  getInterestingFirstNode: function BC_getInterestingFirstNode(aNode)
  {
    let deferred = promise.defer();

    var fallback = null;

    var moreChildren = () => {
      this.walker.children(aNode, {
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
    }
    moreChildren();
    return deferred.promise;
  },

  





  getCommonAncestor: function BC_getCommonAncestor(aNode)
  {
    let node = aNode;
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

  



  ensureFirstChild: function BC_ensureFirstChild()
  {
    
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
    if (this.isDestroyed) {
      return;
    }

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

  


  update: function BC_update(reason)
  {
    if (this.isDestroyed) {
      return;
    }

    if (reason !== "markupmutation") {
      this.inspector.hideNodeMenu();
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

    
    
    if (idx > -1 && reason !== "markupmutation") {
      
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

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
