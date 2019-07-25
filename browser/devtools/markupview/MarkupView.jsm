















const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;


const PAGE_SIZE = 10;

var EXPORTED_SYMBOLS = ["MarkupView"];

Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource:///modules/devtools/CssRuleView.jsm");
Cu.import("resource:///modules/devtools/Templater.jsm");
Cu.import("resource:///modules/devtools/Undo.jsm")



















function MarkupView(aInspector, aFrame)
{
  this._inspector = aInspector;
  this._frame = aFrame;
  this.doc = this._frame.contentDocument;
  this._elt = this.doc.querySelector("#root");

  this.undo = new UndoStack();
  this.undo.installController(this._frame.ownerDocument.defaultView);

  this._containers = new WeakMap();

  this._observer = new this.doc.defaultView.MutationObserver(this._mutationObserver.bind(this));

  this._boundSelect = this._onSelect.bind(this);
  this._inspector.on("select", this._boundSelect);
  this._onSelect();

  this._boundKeyDown = this._onKeyDown.bind(this);
  this._frame.addEventListener("keydown", this._boundKeyDown, false);

  this._boundFocus = this._onFocus.bind(this);
  this._frame.addEventListener("focus", this._boundFocus, false);

  this._onSelect();
}

MarkupView.prototype = {
  _selectedContainer: null,

  


  get selected() {
    return this._selectedContainer ? this._selectedContainer.node : null;
  },

  template: function MT_template(aName, aDest, aOptions)
  {
    let node = this.doc.getElementById("template-" + aName).cloneNode(true);
    node.removeAttribute("id");
    template(node, aDest, aOptions);
    return node;
  },

  



  getContainer: function MT_getContainer(aNode)
  {
    return this._containers.get(aNode);
  },

  


  _onSelect: function MT__onSelect()
  {
    if (this._inspector.selection) {
      this.showNode(this._inspector.selection);
    }
    this.selectNode(this._inspector.selection);
  },

  



  _selectionWalker: function MT__seletionWalker(aStart)
  {
    let walker = this.doc.createTreeWalker(
      aStart || this._elt,
      Ci.nsIDOMNodeFilter.SHOW_ELEMENT,
      function(aElement) {
        if (aElement.container && aElement.container.visible) {
          return Ci.nsIDOMNodeFilter.FILTER_ACCEPT;
        }
        return Ci.nsIDOMNodeFilter.FILTER_SKIP;
      },
      false
    );
    walker.currentNode = this._selectedContainer.elt;
    return walker;
  },

  


  _onKeyDown: function MT__KeyDown(aEvent)
  {
    let handled = true;

    
    if (aEvent.target.tagName.toLowerCase() === "input" ||
        aEvent.target.tagName.toLowerCase() === "textarea") {
      return;
    }

    switch(aEvent.keyCode) {
      case Ci.nsIDOMKeyEvent.DOM_VK_DELETE:
      case Ci.nsIDOMKeyEvent.DOM_VK_BACK_SPACE:
        let node = this._selectedContainer.node;
        let doc = nodeDocument(node);
        if (node != doc && node != doc.documentElement) {
          this.deleteNode(this._selectedContainer.node);
        }
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_HOME:
        this.navigate(this._containers.get(this._rootNode.firstChild));
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_LEFT:
        this.collapseNode(this._selectedContainer.node);
        break;
      case Ci.nsIDOMKeyEvent.DOM_VK_RIGHT:
        this.expandNode(this._selectedContainer.node);
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

  



  deleteNode: function MC__deleteNode(aNode)
  {
    let parentNode = aNode.parentNode;
    let sibling = aNode.nextSibling;

    this.undo.do(function() {
      parentNode.removeChild(aNode);
    }, function() {
      parentNode.insertBefore(aNode, sibling);
    });
  },

  


  _onFocus: function MC__onFocus(aEvent) {
    let parent = aEvent.target;
    while (!parent.container) {
      parent = parent.parentNode;
    }
    if (parent) {
      this.navigate(parent.container, true);
    }
  },

  








  navigate: function MT__navigate(aContainer, aIgnoreFocus)
  {
    if (!aContainer) {
      return;
    }

    let node = aContainer.node;
    this.showNode(node);
    this.selectNode(node);

    if (this._inspector._IUI.highlighter.isNodeHighlightable(node)) {
      this._inspector._IUI.select(node, true, false, "treepanel");
      this._inspector._IUI.highlighter.highlight(node);
    }

    if (!aIgnoreFocus) {
      aContainer.focus();
    }
  },

  







  importNode: function MT_importNode(aNode, aExpand)
  {
    if (!aNode) {
      return null;
    }

    if (this._containers.has(aNode)) {
      return this._containers.get(aNode);
    }

    this._observer.observe(aNode, {
      attributes: true,
      childList: true,
      characterData: true,
    });

    let walker = documentWalker(aNode);
    let parent = walker.parentNode();
    if (parent) {
      
      var container = new MarkupContainer(this, aNode);
    } else {
      var container = new RootContainer(this, aNode);
      this._elt.appendChild(container.elt);
      this._rootNode = aNode;
      aNode.addEventListener("load", function MP_watch_contentLoaded(aEvent) {
        
        this._mutationObserver([{target: aEvent.target, type: "childList"}]);
      }.bind(this), true);

    }

    this._containers.set(aNode, container);
    container.expanded = aExpand;

    this._updateChildren(container);

    if (parent) {
      this.importNode(parent, true);
    }
    return container;
  },

  


  _mutationObserver: function MT__mutationObserver(aMutations)
  {
    for (let mutation of aMutations) {
      let container = this._containers.get(mutation.target);
      if (mutation.type === "attributes" || mutation.type === "characterData") {
        container.update();
      } else if (mutation.type === "childList") {
        this._updateChildren(container);
      }
    }
    this._inspector._emit("markupmutation");
  },

  



  showNode: function MT_showNode(aNode)
  {
    this.importNode(aNode);
    let walker = documentWalker(aNode);
    let parent;
    while (parent = walker.parentNode()) {
      this.expandNode(parent);
    }
    LayoutHelpers.scrollIntoViewIfNeeded(this._containers.get(aNode).editor.elt, false);
  },

  


  _expandContainer: function MT__expandContainer(aContainer)
  {
    if (aContainer.hasChildren && !aContainer.expanded) {
      aContainer.expanded = true;
      this._updateChildren(aContainer);
    }
  },

  


  expandNode: function MT_expandNode(aNode)
  {
    let container = this._containers.get(aNode);
    this._expandContainer(container);
  },

  




  _expandAll: function MT_expandAll(aContainer)
  {
    this._expandContainer(aContainer);
    let child = aContainer.children.firstChild;
    while (child) {
      this._expandAll(child.container);
      child = child.nextSibling;
    }
  },

  





  expandAll: function MT_expandAll(aNode)
  {
    aNode = aNode || this._rootNode;
    this._expandAll(this._containers.get(aNode));
  },

  


  collapseNode: function MT_collapseNode(aNode)
  {
    let container = this._containers.get(aNode);
    container.expanded = false;
  },

  


  selectNode: function MT_selectNode(aNode)
  {
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

    this._selectedContainer.focus();

    return true;
  },

  



  _updateChildren: function MT__updateChildren(aContainer)
  {
    
    let treeWalker = documentWalker(aContainer.node);
    let child = treeWalker.firstChild();
    aContainer.hasChildren = !!child;
    if (aContainer.expanded) {
      let lastContainer = null;
      while (child) {
        let container = this.importNode(child, false);

        
        let before = lastContainer ? lastContainer.nextSibling : aContainer.children.firstChild;
        aContainer.children.insertBefore(container.elt, before);
        lastContainer = container.elt;
        child = treeWalker.nextSibling();
      }

      while (aContainer.children.lastChild != lastContainer) {
        aContainer.children.removeChild(aContainer.children.lastChild);
      }
    }
  },

  


  destroy: function MT_destroy()
  {
    this.undo.destroy();
    delete this.undo;

    this._frame.addEventListener("focus", this._boundFocus, false);
    delete this._boundFocus;

    this._frame.removeEventListener("keydown", this._boundKeyDown, true);
    delete this._boundKeyDown;

    this._inspector.removeListener("select", this._boundSelect);
    delete this._boundSelect;

    delete this._elt;

    delete this._containers;
    this._observer.disconnect();
    delete this._observer;
  }
};













function MarkupContainer(aMarkupView, aNode)
{
  this.markup = aMarkupView;
  this.doc = this.markup.doc;
  this.undo = this.markup.undo;
  this.node = aNode;

  if (aNode.nodeType == Ci.nsIDOMNode.TEXT_NODE) {
    this.editor = new TextEditor(this, aNode, "text");
  } else if (aNode.nodeType == Ci.nsIDOMNode.COMMENT_NODE) {
    this.editor = new TextEditor(this, aNode, "comment");
  } else if (aNode.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
    this.editor = new ElementEditor(this, aNode);
  } else if (aNode.nodeType == Ci.nsIDOMNode.DOCUMENT_TYPE_NODE) {
    this.editor = new DoctypeEditor(this, aNode);
  } else {
    this.editor = new GenericEditor(this.markup, aNode);
  }

  
  this.elt = null;
  this.expander = null;
  this.codeBox = null;
  this.children = null;
  let options = { stack: "markup-view.xhtml" };
  this.markup.template("container", this, options);

  this.elt.container = this;

  this.expander.addEventListener("click", function() {
    this.markup.navigate(this);

    if (this.expanded) {
      this.markup.collapseNode(this.node);
    } else {
      this.markup.expandNode(this.node);
    }
  }.bind(this));

  this.codeBox.insertBefore(this.editor.elt, this.children);

  this.editor.elt.addEventListener("mousedown", function(evt) {
    this.markup.navigate(this);
  }.bind(this), false);

  if (this.editor.closeElt) {
    this.codeBox.appendChild(this.editor.closeElt);
  }

}

MarkupContainer.prototype = {
  



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

  


  get expanded() {
    return this.children.hasAttribute("expanded");
  },

  set expanded(aValue) {
    if (aValue) {
      this.expander.setAttribute("expanded", "");
      this.children.setAttribute("expanded", "");
    } else {
      this.expander.removeAttribute("expanded");
      this.children.removeAttribute("expanded");
    }
  },

  


  get visible()
  {
    return this.elt.getBoundingClientRect().height > 0;
  },

  


  _selected: false,

  get selected() {
    return this._selected;
  },

  set selected(aValue) {
    this._selected = aValue;
    if (this._selected) {
      this.editor.elt.classList.add("selected");
      if (this.editor.closeElt) {
        this.editor.closeElt.classList.add("selected");
      }
    } else {
      this.editor.elt.classList.remove("selected");
      if (this.editor.closeElt) {
        this.editor.closeElt.classList.remove("selected");
      }
    }
  },

  



  update: function MC_update()
  {
    if (this.editor.update) {
      this.editor.update();
    }
  },

  


  focus: function MC_focus()
  {
    let focusable = this.editor.elt.querySelector("[tabindex]");
    if (focusable) {
      focusable.focus();
    }
  }
}




function RootContainer(aMarkupView, aNode)
{
  this.doc = aMarkupView.doc;
  this.elt = this.doc.createElement("ul");
  this.children = this.elt;
  this.node = aNode;
}




function GenericEditor(aContainer, aNode)
{
  this.elt = aContainer.doc.createElement("span");
  this.elt.className = "editor";
  this.elt.textContent = aNode.nodeName;
}







function DoctypeEditor(aContainer, aNode)
{
  this.elt = aContainer.doc.createElement("span");
  this.elt.className = "editor comment";
  this.elt.textContent = '<!DOCTYPE ' + aNode.name +
     (aNode.publicId ? ' PUBLIC "' +  aNode.publicId + '"': '') +
     (aNode.systemId ? ' "' + aNode.systemId + '"' : '') +
     '>';
}









function TextEditor(aContainer, aNode, aTemplate)
{
  this.node = aNode;

  aContainer.markup.template(aTemplate, this);

  _editableField({
    element: this.value,
    stopOnReturn: true,
    trigger: "dblclick",
    multiline: true,
    done: function TE_done(aVal, aCommit) {
      if (!aCommit) {
        return;
      }
      let oldValue = this.node.nodeValue;
      aContainer.undo.do(function() {
        this.node.nodeValue = aVal;
      }.bind(this), function() {
        this.node.nodeValue = oldValue;
      }.bind(this));
    }.bind(this)
  });

  this.update();
}

TextEditor.prototype = {
  update: function TE_update()
  {
    this.value.textContent = this.node.nodeValue;
  }
};







function ElementEditor(aContainer, aNode)
{
  this.doc = aContainer.doc;
  this.undo = aContainer.undo;
  this.template = aContainer.markup.template.bind(aContainer.markup);
  this.node = aNode;

  this.attrs = [];

  
  this.elt = null;
  this.tag = null;
  this.attrList = null;
  this.newAttr = null;
  this.closeElt = null;
  let options = { stack: "markup-view.xhtml" };

  
  this.template("element", this, options);

  
  this.template("elementClose", this, options);

  
  if (aNode != aNode.ownerDocument.documentElement) {
    this.tag.setAttribute("tabindex", "0");
    _editableField({
      element: this.tag,
      trigger: "dblclick",
      stopOnReturn: true,
      done: this.onTagEdit.bind(this),
    });
  }

  
  _editableField({
    element: this.newAttr,
    trigger: "dblclick",
    stopOnReturn: true,
    done: function EE_onNew(aVal, aCommit) {
      if (!aCommit) {
        return;
      }

      this._applyAttributes(aVal);
    }.bind(this)
  });

  let tagName = this.node.nodeName.toLowerCase();
  this.tag.textContent = tagName;
  this.closeTag.textContent = tagName;

  this.update();
}

ElementEditor.prototype = {
  


  update: function EE_update()
  {
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

    
    
    for (let i = 0; i < attrs.length; i++) {
      let attr = this._createAttribute(attrs[i]);
      if (!attr.inplaceEditor) {
        attr.style.removeProperty("display");
      }
    }
  },

  _createAttribute: function EE_createAttribute(aAttr, aBefore)
  {
    if (aAttr.name in this.attrs) {
      var attr = this.attrs[aAttr.name];
      var name = attr.querySelector(".attrname");
      var val = attr.querySelector(".attrvalue");
    } else {
      
      let data = {
        attrName: aAttr.name,
      };
      let options = { stack: "markup-view.xhtml" };
      this.template("attribute", data, options);
      var {attr, inner, name, val} = data;

      
      let before = aBefore || null;
      if (aAttr.name == "id") {
        before = this.attrList.firstChild;
      } else if (aAttr.name == "class") {
        let idNode = this.attrs["id"];
        before = idNode ? idNode.nextSibling : this.attrList.firstChild;
      }
      this.attrList.insertBefore(attr, before);

      
      _editableField({
        element: inner,
        trigger: "dblclick",
        stopOnReturn: true,
        selectAll: false,
        start: function EE_editAttribute_start(aEditor, aEvent) {
          
          
          if (aEvent.target === name) {
            aEditor.input.setSelectionRange(0, name.textContent.length);
          } else if (aEvent.target === val) {
            let length = val.textContent.length;
            let editorLength = aEditor.input.value.length;
            let start = editorLength - (length + 1);
            aEditor.input.setSelectionRange(start, start + length);
          } else {
            aEditor.input.select();
          }
        },
        done: function EE_editAttribute_done(aVal, aCommit) {
          if (!aCommit) {
            return;
          }

          this.undo.startBatch();

          
          
          this._removeAttribute(this.node, aAttr.name)
          this._applyAttributes(aVal, attr);

          this.undo.endBatch();
        }.bind(this)
      });

      this.attrs[aAttr.name] = attr;
    }

    name.textContent = aAttr.name;
    val.textContent = aAttr.value;

    return attr;
  },

  








  _applyAttributes: function EE__applyAttributes(aValue, aAttrNode)
  {
    
    let dummyNode = this.doc.createElement("div");

    let parseTag = (this.node.namespaceURI.match(/svg/i) ? "svg" :
                   (this.node.namespaceURI.match(/mathml/i) ? "math" : "div"));
    let parseText = "<" + parseTag + " " + aValue + "/>";
    dummyNode.innerHTML = parseText;
    let parsedNode = dummyNode.firstChild;

    let attrs = parsedNode.attributes;

    this.undo.startBatch();

    for (let i = 0; i < attrs.length; i++) {
      
      this._createAttribute(attrs[i], aAttrNode ? aAttrNode.nextSibling : null);
      this._setAttribute(this.node, attrs[i].name, attrs[i].value);
    }

    this.undo.endBatch();
  },

  



  _restoreAttribute: function EE_restoreAttribute(aNode, aName)
  {
    if (aNode.hasAttribute(aName)) {
      let oldValue = aNode.getAttribute(aName);
      return function() { aNode.setAttribute(aName, oldValue); };
    } else {
      return function() { aNode.removeAttribute(aName) };
    }
  },

  


  _setAttribute: function EE_setAttribute(aNode, aName, aValue)
  {
    this.undo.do(function() {
      aNode.setAttribute(aName, aValue);
    }, this._restoreAttribute(aNode, aName));
  },

  


  _removeAttribute: function EE_removeAttribute(aNode, aName)
  {
    this.undo.do(function() {
      aNode.removeAttribute(aName);
    }, this._restoreAttribute(aNode, aName));
  },

  


  _onNewAttribute: function EE_onNewAttribute(aValue, aCommit)
  {
    if (!aValue || !aCommit) {
      return;
    }

    this._setAttribute(this.node, aValue, "");
    let attr = this._createAttribute({ name: aValue, value: ""});
    attr.style.removeAttribute("display");
    attr.querySelector("attrvalue").click();
  },


  


  onTagEdit: function EE_onTagEdit(aVal, aCommit) {
    if (!aCommit || aVal == this.node.tagName) {
      return;
    }

    
    
    
    let newElt = nodeDocument(this.node).createElement(aVal);
    let attrs = this.node.attributes;

    for (let i = 0 ; i < attrs.length; i++) {
      newElt.setAttribute(attrs[i].name, attrs[i].value);
    }

    function swapNodes(aOld, aNew) {
      while (aOld.firstChild) {
        aNew.appendChild(aOld.firstChild);
      }
      aOld.parentNode.insertBefore(aNew, aOld);
      aOld.parentNode.removeChild(aOld);
    }

    
    this.undo.do(function() {
      swapNodes(this.node, newElt);
    }.bind(this), function() {
      swapNodes(newElt, this.node);
    }.bind(this));
  },
}



RootContainer.prototype = {
  hasChildren: true,
  expanded: true,
  update: function RC_update() {}
};

function documentWalker(node) {
  return new DocumentWalker(node, Ci.nsIDOMNodeFilter.SHOW_ALL, whitespaceTextFilter, false);
}

function nodeDocument(node) {
  return node.ownerDocument || (node.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE ? node : null);
}







function DocumentWalker(aNode, aShow, aFilter, aExpandEntityReferences)
{
  let doc = nodeDocument(aNode);
  this.walker = doc.createTreeWalker(nodeDocument(aNode),
    aShow, aFilter, aExpandEntityReferences);
  this.walker.currentNode = aNode;
  this.filter = aFilter;
}

DocumentWalker.prototype = {
  get node() this.walker.node,
  get whatToShow() this.walker.whatToShow,
  get expandEntityReferences() this.walker.expandEntityReferences,
  get currentNode() this.walker.currentNode,
  set currentNode(aVal) this.walker.currentNode = aVal,

  




  _reparentWalker: function DW_reparentWalker(aNewNode) {
    if (!aNewNode) {
      return null;
    }
    let doc = nodeDocument(aNewNode);
    let walker = doc.createTreeWalker(doc,
      this.whatToShow, this.filter, this.expandEntityReferences);
    walker.currentNode = aNewNode;
    this.walker = walker;
    return aNewNode;
  },

  parentNode: function DW_parentNode()
  {
    let currentNode = this.walker.currentNode;
    let parentNode = this.walker.parentNode();

    if (!parentNode) {
      if (currentNode && currentNode.nodeType == Ci.nsIDOMNode.DOCUMENT_NODE
          && currentNode.defaultView) {
        let embeddingFrame = currentNode.defaultView.frameElement;
        if (embeddingFrame) {
          return this._reparentWalker(embeddingFrame);
        }
      }
      return null;
    }

    return parentNode;
  },

  firstChild: function DW_firstChild()
  {
    let node = this.walker.currentNode;
    if (!node)
      return;
    if (node.contentDocument) {
      return this._reparentWalker(node.contentDocument);
    } else if (node instanceof nodeDocument(node).defaultView.GetSVGDocument) {
      return this._reparentWalker(node.getSVGDocument());
    }
    return this.walker.firstChild();
  },

  lastChild: function DW_lastChild()
  {
    let node = this.walker.currentNode;
    if (!node)
      return;
    if (node.contentDocument) {
      return this._reparentWalker(node.contentDocument);
    } else if (node instanceof nodeDocument(node).defaultView.GetSVGDocument) {
      return this._reparentWalker(node.getSVGDocument());
    }
    return this.walker.lastChild();
  },

  previousSibling: function DW_previousSibling() this.walker.previousSibling(),
  nextSibling: function DW_nextSibling() this.walker.nextSibling(),

  
}




function whitespaceTextFilter(aNode)
{
    if (aNode.nodeType == Ci.nsIDOMNode.TEXT_NODE &&
        !/[^\s]/.exec(aNode.nodeValue)) {
      return Ci.nsIDOMNodeFilter.FILTER_SKIP;
    } else {
      return Ci.nsIDOMNodeFilter.FILTER_ACCEPT;
    }
}
