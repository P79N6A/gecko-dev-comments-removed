




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const LAZY_EXPAND_DELAY = 50; 
const LAZY_APPEND_DELAY = 100; 
const LAZY_APPEND_BATCH = 100; 
const PAGE_SIZE_SCROLL_HEIGHT_RATIO = 100;
const PAGE_SIZE_MAX_JUMPS = 30;
const SEARCH_ACTION_MAX_DELAY = 300; 

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "WebConsoleUtils", "resource://gre/modules/devtools/WebConsoleUtils.jsm");

this.EXPORTED_SYMBOLS = ["VariablesView"];




const STR = Services.strings.createBundle(DBG_STRINGS_URI);














this.VariablesView = function VariablesView(aParentNode) {
  this._store = new Map();
  this._itemsByElement = new WeakMap();
  this._prevHierarchy = new Map();
  this._currHierarchy = new Map();

  this._parent = aParentNode;
  this._parent.classList.add("variables-view-container");
  this._appendEmptyNotice();

  this._onSearchboxInput = this._onSearchboxInput.bind(this);
  this._onSearchboxKeyPress = this._onSearchboxKeyPress.bind(this);
  this._onViewKeyPress = this._onViewKeyPress.bind(this);

  
  this._list = this.document.createElement("scrollbox");
  this._list.setAttribute("orient", "vertical");
  this._list.addEventListener("keypress", this._onViewKeyPress, false);
  this._parent.appendChild(this._list);
  this._boxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
};

VariablesView.prototype = {
  






  set rawObject(aObject) {
    this.empty();
    this.addScope().addVar().populate(aObject);
  },

  







  addScope: function VV_addScope(aName = "") {
    this._removeEmptyNotice();
    this._toggleSearchVisibility(true);

    let scope = new Scope(this, aName);
    this._store.set(scope.id, scope);
    this._currHierarchy.set(aName, scope);
    this._itemsByElement.set(scope._target, scope);
    scope.header = !!aName;
    return scope;
  },

  






  empty: function VV_empty(aTimeout = this.lazyEmptyDelay) {
    
    if (!this._store.size) {
      return;
    }
    
    if (this.lazyEmpty && aTimeout > 0) {
      this._emptySoon(aTimeout);
      return;
    }

    let list = this._list;
    let firstChild;

    while (firstChild = list.firstChild) {
      list.removeChild(firstChild);
    }

    this._store = new Map();
    this._itemsByElement = new WeakMap();

    this._appendEmptyNotice();
    this._toggleSearchVisibility(false);
  },

  














  _emptySoon: function VV__emptySoon(aTimeout) {
    let prevList = this._list;
    let currList = this._list = this.document.createElement("scrollbox");

    this._store = new Map();
    this._itemsByElement = new WeakMap();

    this._emptyTimeout = this.window.setTimeout(function() {
      this._emptyTimeout = null;

      prevList.removeEventListener("keypress", this._onViewKeyPress, false);
      currList.addEventListener("keypress", this._onViewKeyPress, false);
      currList.setAttribute("orient", "vertical");

      this._parent.removeChild(prevList);
      this._parent.appendChild(currList);
      this._boxObject = currList.boxObject.QueryInterface(Ci.nsIScrollBoxObject);

      if (!this._store.size) {
        this._appendEmptyNotice();
        this._toggleSearchVisibility(false);
      }
    }.bind(this), aTimeout);
  },

  


  lazyEmptyDelay: 150,

  



  lazyEmpty: false,

  



  lazyAppend: true,

  






  eval: null,

  






  switch: null,

  






  delete: null,

  






  editableValueTooltip: STR.GetStringFromName("variablesEditableValueTooltip"),

  






  editableNameTooltip: STR.GetStringFromName("variablesEditableNameTooltip"),

  







  editButtonTooltip: STR.GetStringFromName("variablesEditButtonTooltip"),

  






  deleteButtonTooltip: STR.GetStringFromName("variablesCloseButtonTooltip"),

  






  descriptorTooltip: true,

  





  contextMenuId: "",

  





  separatorStr: STR.GetStringFromName("variablesSeparatorLabel"),

  




  set enumVisible(aFlag) {
    this._enumVisible = aFlag;

    for (let [, scope] of this._store) {
      scope._enumVisible = aFlag;
    }
  },

  




  set nonEnumVisible(aFlag) {
    this._nonEnumVisible = aFlag;

    for (let [, scope] of this._store) {
      scope._nonEnumVisible = aFlag;
    }
  },

  




  set onlyEnumVisible(aFlag) {
    if (aFlag) {
      this.enumVisible = true;
      this.nonEnumVisible = false;
    } else {
      this.enumVisible = true;
      this.nonEnumVisible = true;
    }
  },

  



  set searchEnabled(aFlag) aFlag ? this._enableSearch() : this._disableSearch(),

  



  get searchEnabled() !!this._searchboxContainer,

  



  set searchPlaceholder(aValue) {
    if (this._searchboxNode) {
      this._searchboxNode.setAttribute("placeholder", aValue);
    }
    this._searchboxPlaceholder = aValue;
  },

  



  get searchPlaceholder() this._searchboxPlaceholder,

  



  _enableSearch: function VV__enableSearch() {
    
    if (this._searchboxContainer) {
      return;
    }
    let document = this.document;
    let ownerView = this._parent.parentNode;

    let container = this._searchboxContainer = document.createElement("hbox");
    container.className = "devtools-toolbar";

    
    
    container.hidden = !this._store.size;

    let searchbox = this._searchboxNode = document.createElement("textbox");
    searchbox.className = "variables-view-searchinput devtools-searchinput";
    searchbox.setAttribute("placeholder", this._searchboxPlaceholder);
    searchbox.setAttribute("type", "search");
    searchbox.setAttribute("flex", "1");
    searchbox.addEventListener("input", this._onSearchboxInput, false);
    searchbox.addEventListener("keypress", this._onSearchboxKeyPress, false);

    container.appendChild(searchbox);
    ownerView.insertBefore(container, this._parent);
  },

  



  _disableSearch: function VV__disableSearch() {
    
    if (!this._searchboxContainer) {
      return;
    }
    this._searchboxContainer.parentNode.removeChild(this._searchboxContainer);
    this._searchboxNode.addEventListener("input", this._onSearchboxInput, false);
    this._searchboxNode.addEventListener("keypress", this._onSearchboxKeyPress, false);

    this._searchboxContainer = null;
    this._searchboxNode = null;
  },

  






  _toggleSearchVisibility: function VV__toggleSearchVisibility(aVisibleFlag) {
    
    if (!this._searchboxContainer) {
      return;
    }
    this._searchboxContainer.hidden = !aVisibleFlag;
  },

  


  _onSearchboxInput: function VV__onSearchboxInput() {
    this.performSearch(this._searchboxNode.value);
  },

  


  _onSearchboxKeyPress: function VV__onSearchboxKeyPress(e) {
    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._onSearchboxInput();
        return;
      case e.DOM_VK_ESCAPE:
        this._searchboxNode.value = "";
        this._onSearchboxInput();
        return;
    }
  },

  


  delayedSearch: true,

  





  scheduleSearch: function VV_scheduleSearch(aQuery) {
    if (!this.delayedSearch) {
      this.performSearch(aQuery);
      return;
    }
    let delay = Math.max(SEARCH_ACTION_MAX_DELAY / aQuery.length, 0);

    this.window.clearTimeout(this._searchTimeout);
    this._searchFunction = this._startSearch.bind(this, aQuery);
    this._searchTimeout = this.window.setTimeout(this._searchFunction, delay);
  },

  





  performSearch: function VV_performSearch(aQuery) {
    this.window.clearTimeout(this._searchTimeout);
    this._searchFunction = null;
    this._startSearch(aQuery);
  },

  














  _startSearch: function VV__startSearch(aQuery) {
    for (let [, scope] of this._store) {
      switch (aQuery) {
        case "":
          scope.expand();
          
        case null:
        case undefined:
          scope._performSearch("");
          break;
        default:
          scope._performSearch(aQuery.toLowerCase());
          break;
      }
    }
  },

  


  expandFirstSearchResults: function VV_expandFirstSearchResults() {
    for (let [, scope] of this._store) {
      let match = scope._firstMatch;
      if (match) {
        match.expand();
      }
    }
  },

  


  focusFirstVisibleNode: function VV_focusFirstVisibleNode() {
    let property, variable, scope;

    for (let [, item] of this._currHierarchy) {
      if (!item.focusable) {
        continue;
      }
      if (item instanceof Property) {
        property = item;
        break;
      } else if (item instanceof Variable) {
        variable = item;
        break;
      } else if (item instanceof Scope) {
        scope = item;
        break;
      }
    }
    if (scope) {
      this._focusItem(scope);
    } else if (variable) {
      this._focusItem(variable);
    } else if (property) {
      this._focusItem(property);
    }
    this._parent.scrollTop = 0;
    this._parent.scrollLeft = 0;
  },

  


  focusLastVisibleNode: function VV_focusLastVisibleNode() {
    let property, variable, scope;

    for (let [, item] of this._currHierarchy) {
      if (!item.focusable) {
        continue;
      }
      if (item instanceof Property) {
        property = item;
      } else if (item instanceof Variable) {
        variable = item;
      } else if (item instanceof Scope) {
        scope = item;
      }
    }
    if (property && (!variable || property.isDescendantOf(variable))) {
      this._focusItem(property);
    } else if (variable && (!scope || variable.isDescendantOf(scope))) {
      this._focusItem(variable);
    } else if (scope) {
      this._focusItem(scope);
      this._parent.scrollTop = this._parent.scrollHeight;
      this._parent.scrollLeft = 0;
    }
  },

  







  getScopeForNode: function VV_getScopeForNode(aNode) {
    let item = this._itemsByElement.get(aNode);
    if (item && !(item instanceof Variable) && !(item instanceof Property)) {
      return item;
    }
    return null;
  },

  








  getItemForNode: function VV_getItemForNode(aNode) {
    return this._itemsByElement.get(aNode);
  },

  





  getFocusedItem: function VV_getFocusedItem() {
    let focused = this.document.commandDispatcher.focusedElement;
    return this.getItemForNode(focused);
  },

  



  focusNextItem: function VV_focusNextItem(aMaintainViewFocusedFlag)
    this._focusChange("advanceFocus", aMaintainViewFocusedFlag),

  



  focusPrevItem: function VV_focusPrevItem(aMaintainViewFocusedFlag)
    this._focusChange("rewindFocus", aMaintainViewFocusedFlag),

  










  _focusChange: function VV__focusChange(aDirection, aMaintainViewFocusedFlag) {
    let commandDispatcher = this.document.commandDispatcher;
    let item;

    do {
      commandDispatcher[aDirection]();

      
      
      if (!aMaintainViewFocusedFlag) {
        return false;
      }

      
      item = this.getFocusedItem();
      if (!item) {
        if (aDirection == "advanceFocus") {
          this.focusLastVisibleNode();
        } else {
          this.focusFirstVisibleNode();
        }
        
        
        return true;
      }
    } while (!item.focusable);

    
    return false;
  },

  









  _focusItem: function VV__focusItem(aItem, aCollapseFlag) {
    if (!aItem.focusable) {
      return false;
    }
    if (aCollapseFlag) {
      aItem.collapse();
    }
    aItem._target.focus();
    this._boxObject.ensureElementIsVisible(aItem._arrow);
    return true;
  },

  


  _onViewKeyPress: function VV__onViewKeyPress(e) {
    let item = this.getFocusedItem();

    switch (e.keyCode) {
      case e.DOM_VK_UP:
      case e.DOM_VK_DOWN:
      case e.DOM_VK_LEFT:
      case e.DOM_VK_RIGHT:
      case e.DOM_VK_PAGE_UP:
      case e.DOM_VK_PAGE_DOWN:
      case e.DOM_VK_HOME:
      case e.DOM_VK_END:
        
        e.preventDefault();
        e.stopPropagation();
    }

    switch (e.keyCode) {
      case e.DOM_VK_UP:
        
        this.focusPrevItem(true);
        return;

      case e.DOM_VK_DOWN:
        
        if (!(item instanceof Variable) &&
            !(item instanceof Property) &&
            !item._isExpanded && item._isArrowVisible) {
          item.expand();
        } else {
          this.focusNextItem(true);
        }
        return;

      case e.DOM_VK_LEFT:
        
        
        if (!item._isExpanded || !item._isArrowVisible) {
          let ownerView = item.ownerView;
          if ((ownerView instanceof Variable ||
               ownerView instanceof Property) &&
               ownerView._isExpanded && ownerView._isArrowVisible) {
            if (this._focusItem(ownerView, true)) {
              return;
            }
          }
        }
        
        if (item._isExpanded && item._isArrowVisible) {
          item.collapse();
        } else {
          this.focusPrevItem(true);
        }
        return;

      case e.DOM_VK_RIGHT:
        
        if (!item._isExpanded && item._isArrowVisible) {
          item.expand();
        } else {
          this.focusNextItem(true);
        }
        return;

      case e.DOM_VK_PAGE_UP:
        
        var jumps = this.pageSize || Math.min(Math.floor(this._list.scrollHeight /
          PAGE_SIZE_SCROLL_HEIGHT_RATIO),
          PAGE_SIZE_MAX_JUMPS);

        while (jumps--) {
          if (this.focusPrevItem(true)) {
            return;
          }
        }
        return;

      case e.DOM_VK_PAGE_DOWN:
        
        var jumps = this.pageSize || Math.min(Math.floor(this._list.scrollHeight /
          PAGE_SIZE_SCROLL_HEIGHT_RATIO),
          PAGE_SIZE_MAX_JUMPS);

        while (jumps--) {
          if (this.focusNextItem(true)) {
            return;
          }
        }
        return;

      case e.DOM_VK_HOME:
        this.focusFirstVisibleNode();
        return;

      case e.DOM_VK_END:
        this.focusLastVisibleNode();
        return;

      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        
        if (item instanceof Variable ||
            item instanceof Property) {
          if (e.metaKey || e.altKey || e.shiftKey) {
            item._activateNameInput();
          } else {
            item._activateValueInput();
          }
        }
        return;

      case e.DOM_VK_DELETE:
      case e.DOM_VK_BACK_SPACE:
        
        if (item instanceof Variable ||
            item instanceof Property) {
          item._onDelete(e);
        }
        return;
    }
  },

  




  pageSize: 0,

  



  set emptyText(aValue) {
    if (this._emptyTextNode) {
      this._emptyTextNode.setAttribute("value", aValue);
    }
    this._emptyTextValue = aValue;
    this._appendEmptyNotice();
  },

  


  _appendEmptyNotice: function VV__appendEmptyNotice() {
    if (this._emptyTextNode || !this._emptyTextValue) {
      return;
    }

    let label = this.document.createElement("label");
    label.className = "variables-view-empty-notice";
    label.setAttribute("value", this._emptyTextValue);

    this._parent.appendChild(label);
    this._emptyTextNode = label;
  },

  


  _removeEmptyNotice: function VV__removeEmptyNotice() {
    if (!this._emptyTextNode) {
      return;
    }

    this._parent.removeChild(this._emptyTextNode);
    this._emptyTextNode = null;
  },

  



  get parentNode() this._parent,

  



  get document() this._document || (this._document = this._parent.ownerDocument),

  



  get window() this._window || (this._window = this.document.defaultView),

  _document: null,
  _window: null,

  _store: null,
  _prevHierarchy: null,
  _currHierarchy: null,
  _enumVisible: true,
  _nonEnumVisible: true,
  _emptyTimeout: null,
  _searchTimeout: null,
  _searchFunction: null,
  _parent: null,
  _list: null,
  _boxObject: null,
  _searchboxNode: null,
  _searchboxContainer: null,
  _searchboxPlaceholder: "",
  _emptyTextNode: null,
  _emptyTextValue: ""
};











VariablesView.simpleValueEvalMacro = function(aItem, aCurrentString) {
  return aItem._symbolicName + "=" + aCurrentString;
};












VariablesView.overrideValueEvalMacro = function(aItem, aCurrentString) {
  let property = "\"" + aItem._nameString + "\"";
  let parent = aItem.ownerView._symbolicName || "this";

  return "Object.defineProperty(" + parent + "," + property + "," +
    "{ value: " + aCurrentString +
    ", enumerable: " + parent + ".propertyIsEnumerable(" + property + ")" +
    ", configurable: true" +
    ", writable: true" +
    "})";
};











VariablesView.getterOrSetterEvalMacro = function(aItem, aCurrentString) {
  let type = aItem._nameString;
  let propertyObject = aItem.ownerView;
  let parentObject = propertyObject.ownerView;
  let property = "\"" + propertyObject._nameString + "\"";
  let parent = parentObject._symbolicName || "this";

  switch (aCurrentString) {
    case "":
    case "null":
    case "undefined":
      let mirrorType = type == "get" ? "set" : "get";
      let mirrorLookup = type == "get" ? "__lookupSetter__" : "__lookupGetter__";

      
      
      if ((type == "set" && propertyObject.getter.type == "undefined") ||
          (type == "get" && propertyObject.setter.type == "undefined")) {
        return VariablesView.overrideValueEvalMacro(propertyObject, "undefined");
      }

      
      
      
      
      
      
      
      return "Object.defineProperty(" + parent + "," + property + "," +
        "{" + mirrorType + ":" + parent + "." + mirrorLookup + "(" + property + ")" +
        "," + type + ":" + undefined +
        ", enumerable: " + parent + ".propertyIsEnumerable(" + property + ")" +
        ", configurable: true" +
        "})";

    default:
      
      if (aCurrentString.indexOf("function") != 0) {
        let header = "function(" + (type == "set" ? "value" : "") + ")";
        let body = "";
        
        
        if (aCurrentString.indexOf("return ") != -1) {
          body = "{" + aCurrentString + "}";
        }
        
        else if (aCurrentString.indexOf("{") == 0) {
          body = aCurrentString;
        }
        
        else {
          body = "(" + aCurrentString + ")";
        }
        aCurrentString = header + body;
      }

      
      let defineType = type == "get" ? "__defineGetter__" : "__defineSetter__";

      
      let defineFunc = "eval(\"(" + aCurrentString.replace(/"/g, "\\$&") + ")\")";

      
      
      return parent + "." + defineType + "(" + property + "," + defineFunc + ")";
  }
};







VariablesView.getterOrSetterDeleteCallback = function(aItem) {
  aItem._disable();
  aItem.ownerView.eval(VariablesView.getterOrSetterEvalMacro(aItem, ""));
  return true; 
};












function Scope(aView, aName, aFlags = {}) {
  this.ownerView = aView;

  this._onClick = this._onClick.bind(this);
  this._openEnum = this._openEnum.bind(this);
  this._openNonEnum = this._openNonEnum.bind(this);
  this._batchAppend = this._batchAppend.bind(this);
  this._batchItems = [];

  
  
  this.eval = aView.eval;
  this.switch = aView.switch;
  this.delete = aView.delete;
  this.editableValueTooltip = aView.editableValueTooltip;
  this.editableNameTooltip = aView.editableNameTooltip;
  this.editButtonTooltip = aView.editButtonTooltip;
  this.deleteButtonTooltip = aView.deleteButtonTooltip;
  this.descriptorTooltip = aView.descriptorTooltip;
  this.contextMenuId = aView.contextMenuId;
  this.separatorStr = aView.separatorStr;

  this._store = new Map();
  this._init(aName.trim(), aFlags);
}

Scope.prototype = {
  



















  addVar: function S_addVar(aName = "", aDescriptor = {}) {
    if (this._store.has(aName)) {
      return null;
    }

    let variable = new Variable(this, aName, aDescriptor);
    this._store.set(aName, variable);
    this._variablesView._currHierarchy.set(variable._absoluteName, variable);
    this._variablesView._itemsByElement.set(variable._target, variable);
    variable.header = !!aName;
    return variable;
  },

  







  get: function S_get(aName) {
    return this._store.get(aName);
  },

  








  find: function S_find(aNode) {
    for (let [, variable] of this._store) {
      let match;
      if (variable._target == aNode) {
        match = variable;
      } else {
        match = variable.find(aNode);
      }
      if (match) {
        return match;
      }
    }
    return null;
  },

  








  isChildOf: function S_isChildOf(aParent) {
    return this.ownerView == aParent;
  },

  








  isDescendantOf: function S_isDescendantOf(aParent) {
    if (this.isChildOf(aParent)) {
      return true;
    }
    if (this.ownerView instanceof Scope ||
        this.ownerView instanceof Variable ||
        this.ownerView instanceof Property) {
      return this.ownerView.isDescendantOf(aParent);
    }
  },

  


  show: function S_show() {
    this._target.hidden = false;
    this._isContentVisible = true;

    if (this.onshow) {
      this.onshow(this);
    }
  },

  


  hide: function S_hide() {
    this._target.hidden = true;
    this._isContentVisible = false;

    if (this.onhide) {
      this.onhide(this);
    }
  },

  


  expand: function S_expand() {
    if (this._isExpanded || this._locked) {
      return;
    }
    
    
    
    
    if (!this._isExpanding &&
         this._variablesView.lazyAppend && this._store.size > LAZY_APPEND_BATCH) {
      this._isExpanding = true;

      
      
      this._startThrobber();
      this.window.setTimeout(this.expand.bind(this), LAZY_EXPAND_DELAY);
      return;
    }

    if (this._variablesView._enumVisible) {
      this._openEnum();
    }
    if (this._variablesView._nonEnumVisible) {
      Services.tm.currentThread.dispatch({ run: this._openNonEnum }, 0);
    }
    this._isExpanding = false;
    this._isExpanded = true;

    if (this.onexpand) {
      this.onexpand(this);
    }
  },

  


  collapse: function S_collapse() {
    if (!this._isExpanded || this._locked) {
      return;
    }
    this._arrow.removeAttribute("open");
    this._enum.removeAttribute("open");
    this._nonenum.removeAttribute("open");
    this._isExpanded = false;

    if (this.oncollapse) {
      this.oncollapse(this);
    }
  },

  


  toggle: function S_toggle(e) {
    if (e && e.button != 0) {
      
      return;
    }
    this._wasToggled = true;
    this.expanded ^= 1;

    
    for (let [, variable] of this._store) {
      variable.header = true;
      variable._matched = true;
    }
    if (this.ontoggle) {
      this.ontoggle(this);
    }
  },

  


  showHeader: function S_showHeader() {
    if (this._isHeaderVisible || !this._nameString) {
      return;
    }
    this._target.removeAttribute("non-header");
    this._isHeaderVisible = true;
  },

  



  hideHeader: function S_hideHeader() {
    if (!this._isHeaderVisible) {
      return;
    }
    this.expand();
    this._target.setAttribute("non-header", "");
    this._isHeaderVisible = false;
  },

  


  showArrow: function S_showArrow() {
    if (this._isArrowVisible) {
      return;
    }
    this._arrow.removeAttribute("invisible");
    this._isArrowVisible = true;
  },

  


  hideArrow: function S_hideArrow() {
    if (!this._isArrowVisible) {
      return;
    }
    this._arrow.setAttribute("invisible", "");
    this._isArrowVisible = false;
  },

  



  get visible() this._isContentVisible,

  



  get expanded() this._isExpanded,

  



  get header() this._isHeaderVisible,

  



  get twisty() this._isArrowVisible,

  



  get locked() this._locked,

  



  set visible(aFlag) aFlag ? this.show() : this.hide(),

  



  set expanded(aFlag) aFlag ? this.expand() : this.collapse(),

  



  set header(aFlag) aFlag ? this.showHeader() : this.hideHeader(),

  



  set twisty(aFlag) aFlag ? this.showArrow() : this.hideArrow(),

  



  set locked(aFlag) this._locked = aFlag,

  



  get focusable() {
    
    if (!this._nameString ||
        !this._isContentVisible ||
        !this._isHeaderVisible ||
        !this._isMatch) {
      return false;
    }
    
    let item = this;
    while ((item = item.ownerView) &&  
           (item instanceof Scope ||
            item instanceof Variable ||
            item instanceof Property)) {
      if (!item._isExpanded) {
        return false;
      }
    }
    return true;
  },

  





  addEventListener: function S_addEventListener(aName, aCallback, aCapture) {
    this._title.addEventListener(aName, aCallback, aCapture);
  },

  





  removeEventListener: function S_removeEventListener(aName, aCallback, aCapture) {
    this._title.removeEventListener(aName, aCallback, aCapture);
  },

  



  get id() this._idString,

  



  get name() this._nameString,

  



  get target() this._target,

  







  _init: function S__init(aName, aFlags) {
    this._idString = generateId(this._nameString = aName);
    this._displayScope(aName, "variables-view-scope", "devtools-toolbar");
    this._addEventListeners();
    this.parentNode.appendChild(this._target);
  },

  









  _displayScope: function S__createScope(aName, aClassName, aTitleClassName) {
    let document = this.document;

    let element = this._target = document.createElement("vbox");
    element.id = this._idString;
    element.className = aClassName;

    let arrow = this._arrow = document.createElement("hbox");
    arrow.className = "arrow";

    let name = this._name = document.createElement("label");
    name.className = "plain name";
    name.setAttribute("value", aName);

    let title = this._title = document.createElement("hbox");
    title.className = "title " + (aTitleClassName || "");
    title.setAttribute("align", "center");

    let enumerable = this._enum = document.createElement("vbox");
    let nonenum = this._nonenum = document.createElement("vbox");
    enumerable.className = "variables-view-element-details enum";
    nonenum.className = "variables-view-element-details nonenum";

    title.appendChild(arrow);
    title.appendChild(name);

    element.appendChild(title);
    element.appendChild(enumerable);
    element.appendChild(nonenum);
  },

  


  _addEventListeners: function S__addEventListeners() {
    this._title.addEventListener("mousedown", this._onClick, false);
  },

  


  _onClick: function S__onClick(e) {
    if (e.target == this._inputNode) {
      return;
    }
    this.toggle();
    this._variablesView._focusItem(this);
  },

  












  _lazyAppend: function S__lazyAppend(aImmediateFlag, aEnumerableFlag, aChild) {
    
    if (aImmediateFlag || !this._variablesView.lazyAppend) {
      if (aEnumerableFlag) {
        this._enum.appendChild(aChild);
      } else {
        this._nonenum.appendChild(aChild);
      }
      return;
    }

    let window = this.window;
    let batchItems = this._batchItems;

    window.clearTimeout(this._batchTimeout);
    batchItems.push({ enumerableFlag: aEnumerableFlag, child: aChild });

    
    
    if (batchItems.length > LAZY_APPEND_BATCH) {
      
      Services.tm.currentThread.dispatch({ run: this._batchAppend }, 1);
      return;
    }
    
    
    this._batchTimeout = window.setTimeout(this._batchAppend, LAZY_APPEND_DELAY);
  },

  



  _batchAppend: function S__batchAppend() {
    let document = this.document;
    let batchItems = this._batchItems;

    
    
    let frags = [document.createDocumentFragment(), document.createDocumentFragment()];

    for (let item of batchItems) {
      frags[~~item.enumerableFlag].appendChild(item.child);
    }
    batchItems.length = 0;
    this._enum.appendChild(frags[1]);
    this._nonenum.appendChild(frags[0]);
  },

  


  _startThrobber: function S__startThrobber() {
    if (this._throbber) {
      this._throbber.hidden = false;
      return;
    }
    let throbber = this._throbber = this.document.createElement("hbox");
    throbber.className = "variables-view-throbber";
    this._title.appendChild(throbber);
  },

  


  _stopThrobber: function S__stopThrobber() {
    if (!this._throbber) {
      return;
    }
    this._throbber.hidden = true;
  },

  


  _openEnum: function S__openEnum() {
    this._arrow.setAttribute("open", "");
    this._enum.setAttribute("open", "");
    this._stopThrobber();
  },

  


  _openNonEnum: function S__openNonEnum() {
    this._nonenum.setAttribute("open", "");
    this._stopThrobber();
  },

  



  set _enumVisible(aFlag) {
    for (let [, variable] of this._store) {
      variable._enumVisible = aFlag;

      if (!this._isExpanded) {
        continue;
      }
      if (aFlag) {
        this._enum.setAttribute("open", "");
      } else {
        this._enum.removeAttribute("open");
      }
    }
  },

  



  set _nonEnumVisible(aFlag) {
    for (let [, variable] of this._store) {
      variable._nonEnumVisible = aFlag;

      if (!this._isExpanded) {
        continue;
      }
      if (aFlag) {
        this._nonenum.setAttribute("open", "");
      } else {
        this._nonenum.removeAttribute("open");
      }
    }
  },

  






  _performSearch: function S__performSearch(aLowerCaseQuery) {
    for (let [, variable] of this._store) {
      let currentObject = variable;
      let lowerCaseName = variable._nameString.toLowerCase();
      let lowerCaseValue = variable._valueString.toLowerCase();

      
      if (!lowerCaseName.contains(aLowerCaseQuery) &&
          !lowerCaseValue.contains(aLowerCaseQuery)) {
        variable._matched = false;
      }
      
      else {
        variable._matched = true;

        
        
        

        if (variable._wasToggled && aLowerCaseQuery) {
          variable.expand();
        }
        if (variable._isExpanded && !aLowerCaseQuery) {
          variable._wasToggled = true;
        }

        
        
        

        while ((variable = variable.ownerView) &&  
               (variable instanceof Scope ||
                variable instanceof Variable ||
                variable instanceof Property)) {

          
          variable._matched = true;
          aLowerCaseQuery && variable.expand();
        }
      }

      
      if (currentObject._wasToggled ||
          currentObject.getter ||
          currentObject.setter) {
        currentObject._performSearch(aLowerCaseQuery);
      }
    }
  },

  



  set _matched(aStatus) {
    if (this._isMatch == aStatus) {
      return;
    }
    if (aStatus) {
      this._isMatch = true;
      this.target.removeAttribute("non-match");
    } else {
      this._isMatch = false;
      this.target.setAttribute("non-match", "");
    }
  },

  



  get _firstMatch() {
    for (let [, variable] of this._store) {
      let match;
      if (variable._isMatch) {
        match = variable;
      } else {
        match = variable._firstMatch;
      }
      if (match) {
        return match;
      }
    }
    return null;
  },

  



  get _variablesView() this._topView || (this._topView = (function(self) {
    let parentView = self.ownerView;
    let topView;

    while (topView = parentView.ownerView) {
      parentView = topView;
    }
    return parentView;
  })(this)),

  



  get parentNode() this.ownerView._list,

  



  get document() this._document || (this._document = this.ownerView.document),

  



  get window() this._window || (this._window = this.ownerView.window),

  _topView: null,
  _document: null,
  _window: null,

  ownerView: null,
  eval: null,
  switch: null,
  delete: null,
  editableValueTooltip: "",
  editableNameTooltip: "",
  editButtonTooltip: "",
  deleteButtonTooltip: "",
  descriptorTooltip: true,
  contextMenuId: "",
  separatorStr: "",

  _store: null,
  _fetched: false,
  _retrieved: false,
  _committed: false,
  _batchItems: null,
  _batchTimeout: null,
  _locked: false,
  _isExpanding: false,
  _isExpanded: false,
  _wasToggled: false,
  _isContentVisible: true,
  _isHeaderVisible: true,
  _isArrowVisible: true,
  _isMatch: true,
  _idString: "",
  _nameString: "",
  _target: null,
  _arrow: null,
  _name: null,
  _title: null,
  _enum: null,
  _nonenum: null,
  _throbber: null
};












function Variable(aScope, aName, aDescriptor) {
  this._displayTooltip = this._displayTooltip.bind(this);
  this._activateNameInput = this._activateNameInput.bind(this);
  this._activateValueInput = this._activateValueInput.bind(this);

  Scope.call(this, aScope, aName, this._initialDescriptor = aDescriptor);
  this.setGrip(aDescriptor.value);
  this._symbolicName = aName;
  this._absoluteName = aScope.name + "[\"" + aName + "\"]";
}

ViewHelpers.create({ constructor: Variable, proto: Scope.prototype }, {
  



















  addProperty: function V_addProperty(aName = "", aDescriptor = {}) {
    if (this._store.has(aName)) {
      return null;
    }

    let property = new Property(this, aName, aDescriptor);
    this._store.set(aName, property);
    this._variablesView._currHierarchy.set(property._absoluteName, property);
    this._variablesView._itemsByElement.set(property._target, property);
    property.header = !!aName;
    return property;
  },

  




















  addProperties: function V_addProperties(aProperties, aOptions = {}) {
    let propertyNames = Object.keys(aProperties);

    
    if (aOptions.sorted) {
      propertyNames.sort();
    }
    
    for (let name of propertyNames) {
      let descriptor = aProperties[name];
      let property = this.addProperty(name, descriptor);

      if (aOptions.callback) {
        aOptions.callback(property, descriptor.value);
      }
    }
  },

  








  populate: function V_populate(aObject, aOptions = {}) {
    
    if (this._fetched) {
      return;
    }
    this._fetched = true;

    let propertyNames = Object.getOwnPropertyNames(aObject);
    let prototype = Object.getPrototypeOf(aObject);

    
    if (aOptions.sorted) {
      propertyNames.sort();
    }
    
    for (let name of propertyNames) {
      let descriptor = Object.getOwnPropertyDescriptor(aObject, name);
      if (descriptor.get || descriptor.set) {
        this._addRawNonValueProperty(name, descriptor);
      } else {
        this._addRawValueProperty(name, descriptor, aObject[name]);
      }
    }
    
    if (prototype) {
      this._addRawValueProperty("__proto__", {}, prototype);
    }
  },

  









  _populateTarget: function V__populateTarget(aVar, aObject = aVar._sourceValue) {
    aVar.populate(aObject);
  },

  










  _addRawValueProperty: function V__addRawValueProperty(aName, aDescriptor, aValue) {
    let descriptor = Object.create(aDescriptor);
    descriptor.value = VariablesView.getGrip(aValue);

    let propertyItem = this.addProperty(aName, descriptor);
    propertyItem._sourceValue = aValue;

    
    
    if (!VariablesView.isPrimitive(descriptor)) {
      propertyItem.onexpand = this._populateTarget;
    }
  },

  








  _addRawNonValueProperty: function V__addRawNonValueProperty(aName, aDescriptor) {
    let descriptor = Object.create(aDescriptor);
    descriptor.get = VariablesView.getGrip(aDescriptor.get);
    descriptor.set = VariablesView.getGrip(aDescriptor.set);

    this.addProperty(aName, descriptor);
  },

  




  get symbolicName() this._symbolicName,

  



  get value() this._initialDescriptor.value,

  



  get getter() this._initialDescriptor.get,

  



  get setter() this._initialDescriptor.set,

  
















  setGrip: function V_setGrip(aGrip) {
    
    if (!this._nameString) {
      return;
    }
    
    if (!this._isUndefined && (this.getter || this.setter)) {
      this._valueLabel.setAttribute("value", "");
      return;
    }

    if (aGrip === undefined) {
      aGrip = { type: "undefined" };
    }
    if (aGrip === null) {
      aGrip = { type: "null" };
    }

    let prevGrip = this._valueGrip;
    if (prevGrip) {
      this._valueLabel.classList.remove(VariablesView.getClass(prevGrip));
    }
    this._valueGrip = aGrip;
    this._valueString = VariablesView.getString(aGrip);
    this._valueClassName = VariablesView.getClass(aGrip);

    this._valueLabel.classList.add(this._valueClassName);
    this._valueLabel.setAttribute("value", this._valueString);
  },

  







  _init: function V__init(aName, aDescriptor) {
    this._idString = generateId(this._nameString = aName);
    this._displayScope(aName, "variables-view-variable variable-or-property");

    
    if (this._nameString) {
      this._displayVariable();
      this._customizeVariable();
      this._prepareTooltip();
      this._setAttributes();
      this._addEventListeners();
    }

    this._onInit(this.ownerView._store.size < LAZY_APPEND_BATCH);
  },

  






  _onInit: function V__onInit(aImmediateFlag) {
    if (this._initialDescriptor.enumerable ||
        this._nameString == "this" ||
        this._nameString == "<exception>") {
      this.ownerView._lazyAppend(aImmediateFlag, true, this._target);
    } else {
      this.ownerView._lazyAppend(aImmediateFlag, false, this._target);
    }
  },

  


  _displayVariable: function V__createVariable() {
    let document = this.document;
    let descriptor = this._initialDescriptor;

    let separatorLabel = this._separatorLabel = document.createElement("label");
    separatorLabel.className = "plain separator";
    separatorLabel.setAttribute("value", this.ownerView.separatorStr);

    let valueLabel = this._valueLabel = document.createElement("label");
    valueLabel.className = "plain value";
    valueLabel.setAttribute("crop", "center");
    valueLabel.setAttribute('flex', "1");

    this._title.appendChild(separatorLabel);
    this._title.appendChild(valueLabel);

    let isPrimitive = this._isPrimitive = VariablesView.isPrimitive(descriptor);
    let isUndefined = this._isUndefined = VariablesView.isUndefined(descriptor);

    if (isPrimitive || isUndefined) {
      this.hideArrow();
    }
    if (!isUndefined && (descriptor.get || descriptor.set)) {
      separatorLabel.hidden = true;
      valueLabel.hidden = true;

      
      this.switch = null;

      
      
      if (this.ownerView.eval) {
        this.delete = VariablesView.getterOrSetterDeleteCallback;
        this.evaluationMacro = VariablesView.overrideValueEvalMacro;
      }
      
      
      else {
        this.delete = null;
      }

      let getter = this.addProperty("get", { value: descriptor.get });
      let setter = this.addProperty("set", { value: descriptor.set });
      getter.evaluationMacro = VariablesView.getterOrSetterEvalMacro;
      setter.evaluationMacro = VariablesView.getterOrSetterEvalMacro;

      getter.hideArrow();
      setter.hideArrow();
      this.expand();
    }
  },

  


  _customizeVariable: function V__customizeVariable() {
    if (this.ownerView.eval) {
      if (!this._isUndefined && (this.getter || this.setter)) {
        let editNode = this._editNode = this.document.createElement("toolbarbutton");
        editNode.className = "plain variables-view-edit";
        editNode.addEventListener("mousedown", this._onEdit.bind(this), false);
        this._title.appendChild(editNode);
      }
    }
    if (this.ownerView.delete) {
      if (!this._isUndefined || !(this.ownerView.getter && this.ownerView.setter)) {
        let deleteNode = this._deleteNode = this.document.createElement("toolbarbutton");
        deleteNode.className = "plain variables-view-delete";
        deleteNode.addEventListener("click", this._onDelete.bind(this), false);
        this._title.appendChild(deleteNode);
      }
    }
    if (this.ownerView.contextMenuId) {
      this._title.setAttribute("context", this.ownerView.contextMenuId);
    }
  },

  


  _prepareTooltip: function V__prepareTooltip() {
    this._target.addEventListener("mouseover", this._displayTooltip, false);
  },

  


  _displayTooltip: function V__displayTooltip() {
    this._target.removeEventListener("mouseover", this._displayTooltip, false);

    if (this.ownerView.descriptorTooltip) {
      let document = this.document;

      let tooltip = document.createElement("tooltip");
      tooltip.id = "tooltip-" + this._idString;

      let configurableLabel = document.createElement("label");
      let enumerableLabel = document.createElement("label");
      let writableLabel = document.createElement("label");
      configurableLabel.setAttribute("value", "configurable");
      enumerableLabel.setAttribute("value", "enumerable");
      writableLabel.setAttribute("value", "writable");

      tooltip.setAttribute("orient", "horizontal");
      tooltip.appendChild(configurableLabel);
      tooltip.appendChild(enumerableLabel);
      tooltip.appendChild(writableLabel);

      this._target.appendChild(tooltip);
      this._target.setAttribute("tooltip", tooltip.id);
    }
    if (this.ownerView.eval && !this._isUndefined && (this.getter || this.setter)) {
      this._editNode.setAttribute("tooltiptext", this.ownerView.editButtonTooltip);
    }
    if (this.ownerView.eval) {
      this._valueLabel.setAttribute("tooltiptext", this.ownerView.editableValueTooltip);
    }
    if (this.ownerView.switch) {
      this._name.setAttribute("tooltiptext", this.ownerView.editableNameTooltip);
    }
    if (this.ownerView.delete) {
      this._deleteNode.setAttribute("tooltiptext", this.ownerView.deleteButtonTooltip);
    }
  },

  



  _setAttributes: function V__setAttributes() {
    let descriptor = this._initialDescriptor;
    let name = this._nameString;

    if (this.ownerView.eval) {
      this._target.setAttribute("editable", "");
    }
    if (!descriptor.configurable) {
      this._target.setAttribute("non-configurable", "");
    }
    if (!descriptor.enumerable) {
      this._target.setAttribute("non-enumerable", "");
    }
    if (!descriptor.writable && !this.ownerView.getter && !this.ownerView.setter) {
      this._target.setAttribute("non-writable", "");
    }
    if (name == "this") {
      this._target.setAttribute("self", "");
    }
    else if (name == "<exception>") {
      this._target.setAttribute("exception", "");
    }
    else if (name == "__proto__") {
      this._target.setAttribute("proto", "");
    }
  },

  


  _addEventListeners: function V__addEventListeners() {
    this._name.addEventListener("dblclick", this._activateNameInput, false);
    this._valueLabel.addEventListener("mousedown", this._activateValueInput, false);
    this._title.addEventListener("mousedown", this._onClick, false);
  },

  









  _activateInput: function V__activateInput(aLabel, aClassName, aCallbacks) {
    let initialString = aLabel.getAttribute("value");

    
    
    let input = this.document.createElement("textbox");
    input.className = "plain " + aClassName;
    input.setAttribute("value", initialString);
    input.setAttribute("flex", "1");

    
    aLabel.parentNode.replaceChild(input, aLabel);
    this._variablesView._boxObject.ensureElementIsVisible(input);
    input.select();

    
    
    
    if (aLabel.getAttribute("value").match(/^".+"$/)) {
      input.selectionEnd--;
      input.selectionStart++;
    }

    input.addEventListener("keypress", aCallbacks.onKeypress, false);
    input.addEventListener("blur", aCallbacks.onBlur, false);

    this._prevExpandable = this.twisty;
    this._prevExpanded = this.expanded;
    this.collapse();
    this.hideArrow();
    this._locked = true;

    this._inputNode = input;
    this._stopThrobber();
  },

  







  _deactivateInput: function V__deactivateInput(aLabel, aInput, aCallbacks) {
    aInput.parentNode.replaceChild(aLabel, aInput);
    aInput.removeEventListener("keypress", aCallbacks.onKeypress, false);
    aInput.removeEventListener("blur", aCallbacks.onBlur, false);

    this._locked = false;
    this.twisty = this._prevExpandable;
    this.expanded = this._prevExpanded;

    this._inputNode = null;
    this._stopThrobber();
  },

  


  _activateNameInput: function V__activateNameInput(e) {
    if (e && e.button != 0) {
      
      return;
    }
    if (!this.ownerView.switch) {
      return;
    }
    if (e) {
      e.preventDefault();
      e.stopPropagation();
    }

    this._onNameInputKeyPress = this._onNameInputKeyPress.bind(this);
    this._deactivateNameInput = this._deactivateNameInput.bind(this);

    this._activateInput(this._name, "element-name-input", {
      onKeypress: this._onNameInputKeyPress,
      onBlur: this._deactivateNameInput
    });
    this._separatorLabel.hidden = true;
    this._valueLabel.hidden = true;
  },

  


  _deactivateNameInput: function V__deactivateNameInput(e) {
    this._deactivateInput(this._name, e.target, {
      onKeypress: this._onNameInputKeyPress,
      onBlur: this._deactivateNameInput
    });
    this._separatorLabel.hidden = false;
    this._valueLabel.hidden = false;
  },

  


  _activateValueInput: function V__activateValueInput(e) {
    if (e && e.button != 0) {
      
      return;
    }
    if (!this.ownerView.eval) {
      return;
    }
    if (e) {
      e.preventDefault();
      e.stopPropagation();
    }

    this._onValueInputKeyPress = this._onValueInputKeyPress.bind(this);
    this._deactivateValueInput = this._deactivateValueInput.bind(this);

    this._activateInput(this._valueLabel, "element-value-input", {
      onKeypress: this._onValueInputKeyPress,
      onBlur: this._deactivateValueInput
    });
  },

  


  _deactivateValueInput: function V__deactivateValueInput(e) {
    this._deactivateInput(this._valueLabel, e.target, {
      onKeypress: this._onValueInputKeyPress,
      onBlur: this._deactivateValueInput
    });
  },

  


  _disable: function V__disable() {
    this.hideArrow();
    this._separatorLabel.hidden = true;
    this._valueLabel.hidden = true;
    this._enum.hidden = true;
    this._nonenum.hidden = true;

    if (this._editNode) {
      this._editNode.hidden = true;
    }
    if (this._deleteNode) {
      this._deleteNode.hidden = true;
    }
  },

  


  _saveNameInput: function V__saveNameInput(e) {
    let input = e.target;
    let initialString = this._name.getAttribute("value");
    let currentString = input.value.trim();
    this._deactivateNameInput(e);

    if (initialString != currentString) {
      this._disable();
      this._name.value = currentString;
      this.ownerView.switch(this, currentString);
    }
  },

  


  _saveValueInput: function V__saveValueInput(e) {
    let input = e.target;
    let initialString = this._valueLabel.getAttribute("value");
    let currentString = input.value.trim();
    this._deactivateValueInput(e);

    if (initialString != currentString) {
      this._disable();
      this.ownerView.eval(this.evaluationMacro(this, currentString.trim()));
    }
  },

  



  evaluationMacro: VariablesView.simpleValueEvalMacro,

  


  _onNameInputKeyPress: function V__onNameInputKeyPress(e) {
    e.stopPropagation();

    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._saveNameInput(e);
        this._variablesView._focusItem(this);
        return;
      case e.DOM_VK_ESCAPE:
        this._deactivateNameInput(e);
        this._variablesView._focusItem(this);
        return;
    }
  },

  


  _onValueInputKeyPress: function V__onValueInputKeyPress(e) {
    e.stopPropagation();

    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._saveValueInput(e);
        this._variablesView._focusItem(this);
        return;
      case e.DOM_VK_ESCAPE:
        this._deactivateValueInput(e);
        this._variablesView._focusItem(this);
        return;
    }
  },

  


  _onEdit: function V__onEdit(e) {
    e.preventDefault();
    e.stopPropagation();
    this._activateValueInput();
  },

  


  _onDelete: function V__onDelete(e) {
    e.preventDefault();
    e.stopPropagation();

    if (this.ownerView.delete) {
      if (!this.ownerView.delete(this)) {
        this.hide();
      }
    }
  },

  _symbolicName: "",
  _absoluteName: "",
  _initialDescriptor: null,
  _isPrimitive: false,
  _isUndefined: false,
  _separatorLabel: null,
  _valueLabel: null,
  _inputNode: null,
  _editNode: null,
  _deleteNode: null,
  _tooltip: null,
  _valueGrip: null,
  _valueString: "",
  _valueClassName: "",
  _prevExpandable: false,
  _prevExpanded: false
});












function Property(aVar, aName, aDescriptor) {
  Variable.call(this, aVar, aName, aDescriptor);
  this._symbolicName = aVar._symbolicName + "[\"" + aName + "\"]";
  this._absoluteName = aVar._absoluteName + "[\"" + aName + "\"]";
}

ViewHelpers.create({ constructor: Property, proto: Variable.prototype }, {
  







  _init: function P__init(aName, aDescriptor) {
    this._idString = generateId(this._nameString = aName);
    this._displayScope(aName, "variables-view-property variable-or-property");

    
    if (this._nameString) {
      this._displayVariable();
      this._customizeVariable();
      this._prepareTooltip();
      this._setAttributes();
      this._addEventListeners();
    }

    this._onInit(this.ownerView._store.size < LAZY_APPEND_BATCH);
  },

  






  _onInit: function P__onInit(aImmediateFlag) {
    if (this._initialDescriptor.enumerable) {
      this.ownerView._lazyAppend(aImmediateFlag, true, this._target);
    } else {
      this.ownerView._lazyAppend(aImmediateFlag, false, this._target);
    }
  }
});




VariablesView.prototype.__iterator__ =
Scope.prototype.__iterator__ =
Variable.prototype.__iterator__ =
Property.prototype.__iterator__ = function VV_iterator() {
  for (let item of this._store) {
    yield item;
  }
};





VariablesView.prototype.clearHierarchy = function VV_clearHierarchy() {
  this._prevHierarchy = new Map();
  this._currHierarchy = new Map();
};





VariablesView.prototype.createHierarchy = function VV_createHierarchy() {
  this._prevHierarchy = this._currHierarchy;
  this._currHierarchy = new Map();
};





VariablesView.prototype.commitHierarchy = function VV_commitHierarchy() {
  let prevHierarchy = this._prevHierarchy;
  let currHierarchy = this._currHierarchy;

  for (let [absoluteName, currVariable] of currHierarchy) {
    
    if (currVariable._committed) {
      continue;
    }
    
    if (this.commitHierarchyIgnoredItems[currVariable._nameString]) {
      continue;
    }

    
    
    let prevVariable = prevHierarchy.get(absoluteName);
    let expanded = false;
    let changed = false;

    
    
    
    if (prevVariable) {
      expanded = prevVariable._isExpanded;

      
      if (currVariable instanceof Variable ||
          currVariable instanceof Property) {
        changed = prevVariable._valueString != currVariable._valueString;
      }
    }

    
    
    currVariable._committed = true;

    
    if (expanded) {
      currVariable._wasToggled = prevVariable._wasToggled;
      currVariable.expand();
    }
    
    if (!changed) {
      continue;
    }

    
    
    
    this.window.setTimeout(function(aTarget) {
      aTarget.addEventListener("transitionend", function onEvent() {
        aTarget.removeEventListener("transitionend", onEvent, false);
        aTarget.removeAttribute("changed");
      }, false);
      aTarget.setAttribute("changed", "");
    }.bind(this, currVariable.target), this.lazyEmptyDelay + 1);
  }
};



VariablesView.prototype.commitHierarchyIgnoredItems = Object.create(null, {
  "window": { value: true }
});








VariablesView.isPrimitive = function VV_isPrimitive(aDescriptor) {
  
  
  let getter = aDescriptor.get;
  let setter = aDescriptor.set;
  if (getter || setter) {
    return false;
  }

  
  
  let grip = aDescriptor.value;
  if (!grip || typeof grip != "object") {
    return true;
  }

  
  let type = grip.type;
  if (type == "undefined" || type == "null" || type == "longString") {
    return true;
  }

  return false;
};







VariablesView.isUndefined = function VV_isUndefined(aDescriptor) {
  
  
  let getter = aDescriptor.get;
  let setter = aDescriptor.set;
  if (typeof getter == "object" && getter.type == "undefined" &&
      typeof setter == "object" && setter.type == "undefined") {
    return true;
  }

  
  
  
  let grip = aDescriptor.value;
  if (grip && grip.type == "undefined") {
    return true;
  }

  return false;
};







VariablesView.isFalsy = function VV_isFalsy(aDescriptor) {
  
  
  let grip = aDescriptor.value;
  if (typeof grip != "object") {
    return !grip;
  }

  
  let type = grip.type;
  if (type == "undefined" || type == "null") {
    return true;
  }

  return false;
};









VariablesView.getGrip = function VV_getGrip(aValue) {
  if (aValue === undefined) {
    return { type: "undefined" };
  }
  if (aValue === null) {
    return { type: "null" };
  }
  if (typeof aValue == "object" || typeof aValue == "function") {
    return { type: "object", class: WebConsoleUtils.getObjectClassName(aValue) };
  }
  return aValue;
};











VariablesView.getString = function VV_getString(aGrip, aConciseFlag) {
  if (aGrip && typeof aGrip == "object") {
    switch (aGrip.type) {
      case "undefined":
        return "undefined";
      case "null":
        return "null";
      case "longString":
        return "\"" + aGrip.initial + "\"";
      default:
        if (!aConciseFlag) {
          return "[" + aGrip.type + " " + aGrip.class + "]";
        } else {
          return aGrip.class;
        }
    }
  } else {
    switch (typeof aGrip) {
      case "string":
        return "\"" + aGrip + "\"";
      case "boolean":
        return aGrip ? "true" : "false";
    }
  }
  return aGrip + "";
};









VariablesView.getClass = function VV_getClass(aGrip) {
  if (aGrip && typeof aGrip == "object") {
    switch (aGrip.type) {
      case "undefined":
        return "token-undefined";
      case "null":
        return "token-null";
      case "longString":
        return "token-string";
    }
  } else {
    switch (typeof aGrip) {
      case "string":
        return "token-string";
      case "boolean":
        return "token-boolean";
      case "number":
        return "token-number";
    }
  }
  return "token-other";
};










let generateId = (function() {
  let count = 0;
  return function VV_generateId(aName = "") {
    return aName.toLowerCase().trim().replace(/\s+/g, "-") + (++count);
  };
})();
