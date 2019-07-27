




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const PANE_APPEARANCE_DELAY = 50;
const PAGE_SIZE_ITEM_COUNT_RATIO = 5;
const WIDGET_FOCUSABLE_NODES = new Set(["vbox", "hbox"]);

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm");

this.EXPORTED_SYMBOLS = [
  "Heritage", "ViewHelpers", "WidgetMethods",
  "setNamedTimeout", "clearNamedTimeout",
  "setConditionalTimeout", "clearConditionalTimeout",
];





this.Heritage = {
  


  extend: function(aPrototype, aProperties = {}) {
    return Object.create(aPrototype, this.getOwnPropertyDescriptors(aProperties));
  },

  


  getOwnPropertyDescriptors: function(aObject) {
    return Object.getOwnPropertyNames(aObject).reduce((aDescriptor, aName) => {
      aDescriptor[aName] = Object.getOwnPropertyDescriptor(aObject, aName);
      return aDescriptor;
    }, {});
  }
};












this.setNamedTimeout = function setNamedTimeout(aId, aWait, aCallback) {
  clearNamedTimeout(aId);

  namedTimeoutsStore.set(aId, setTimeout(() =>
    namedTimeoutsStore.delete(aId) && aCallback(), aWait));
};








this.clearNamedTimeout = function clearNamedTimeout(aId) {
  if (!namedTimeoutsStore) {
    return;
  }
  clearTimeout(namedTimeoutsStore.get(aId));
  namedTimeoutsStore.delete(aId);
};















this.setConditionalTimeout = function setConditionalTimeout(aId, aWait, aPredicate, aCallback) {
  setNamedTimeout(aId, aWait, function maybeCallback() {
    if (aPredicate()) {
      aCallback();
      return;
    }
    setConditionalTimeout(aId, aWait, aPredicate, aCallback);
  });
};








this.clearConditionalTimeout = function clearConditionalTimeout(aId) {
  clearNamedTimeout(aId);
};

XPCOMUtils.defineLazyGetter(this, "namedTimeoutsStore", () => new Map());




this.ViewHelpers = {
  












  dispatchEvent: function(aTarget, aType, aDetail) {
    if (!(aTarget instanceof Ci.nsIDOMNode)) {
      return true; 
    }
    let document = aTarget.ownerDocument || aTarget;
    let dispatcher = aTarget.ownerDocument ? aTarget : document.documentElement;

    let event = document.createEvent("CustomEvent");
    event.initCustomEvent(aType, true, true, aDetail);
    return dispatcher.dispatchEvent(event);
  },

  







  delegateWidgetAttributeMethods: function(aWidget, aNode) {
    aWidget.getAttribute =
      aWidget.getAttribute || aNode.getAttribute.bind(aNode);
    aWidget.setAttribute =
      aWidget.setAttribute || aNode.setAttribute.bind(aNode);
    aWidget.removeAttribute =
      aWidget.removeAttribute || aNode.removeAttribute.bind(aNode);
  },

  







  delegateWidgetEventMethods: function(aWidget, aNode) {
    aWidget.addEventListener =
      aWidget.addEventListener || aNode.addEventListener.bind(aNode);
    aWidget.removeEventListener =
      aWidget.removeEventListener || aNode.removeEventListener.bind(aNode);
  },

  






  isEventEmitter: function(aObject) {
    return aObject && aObject.on && aObject.off && aObject.once && aObject.emit;
  },

  





  isNode: function(aObject) {
    return aObject instanceof Ci.nsIDOMNode ||
           aObject instanceof Ci.nsIDOMElement ||
           aObject instanceof Ci.nsIDOMDocumentFragment;
  },

  





  preventScrolling: function(e) {
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
  },

  











  togglePane: function(aFlags, aPane) {
    
    if (!aPane) {
      return;
    }

    
    aPane.removeAttribute("hidden");

    
    if (!aPane.classList.contains("generic-toggled-side-pane")) {
      aPane.classList.add("generic-toggled-side-pane");
    }

    
    if (aFlags.visible == !aPane.hasAttribute("pane-collapsed")) {
      if (aFlags.callback) aFlags.callback();
      return;
    }

    
    if (aFlags.animated) {
      aPane.setAttribute("animated", "");
    } else {
      aPane.removeAttribute("animated");
    }

    
    let doToggle = () => {
      if (aFlags.visible) {
        aPane.style.marginLeft = "0";
        aPane.style.marginRight = "0";
        aPane.removeAttribute("pane-collapsed");
      } else {
        let margin = ~~(aPane.getAttribute("width")) + 1;
        aPane.style.marginLeft = -margin + "px";
        aPane.style.marginRight = -margin + "px";
        aPane.setAttribute("pane-collapsed", "");
      }

      
      if (aFlags.animated) {
        aPane.addEventListener("transitionend", function onEvent() {
          aPane.removeEventListener("transitionend", onEvent, false);
          if (aFlags.callback) aFlags.callback();
        }, false);
      }
      
      else {
        if (aFlags.callback) aFlags.callback();
      }
    }

    
    
    if (aFlags.delayed) {
      aPane.ownerDocument.defaultView.setTimeout(doToggle, PANE_APPEARANCE_DELAY);
    } else {
      doToggle();
    }
  }
};







ViewHelpers.L10N = function(aStringBundleName) {
  XPCOMUtils.defineLazyGetter(this, "stringBundle", () =>
    Services.strings.createBundle(aStringBundleName));

  XPCOMUtils.defineLazyGetter(this, "ellipsis", () =>
    Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data);
};

ViewHelpers.L10N.prototype = {
  stringBundle: null,

  





  getStr: function(aName) {
    return this.stringBundle.GetStringFromName(aName);
  },

  






  getFormatStr: function(aName, ...aArgs) {
    return this.stringBundle.formatStringFromName(aName, aArgs, aArgs.length);
  },

  








  getFormatStrWithNumbers: function(aName, ...aArgs) {
    let newArgs = aArgs.map(x => typeof x == "number" ? this.numberWithDecimals(x, 2) : x);
    return this.stringBundle.formatStringFromName(aName, newArgs, newArgs.length);
  },

  










  numberWithDecimals: function(aNumber, aDecimals = 0) {
    
    if (aNumber == (aNumber | 0)) {
      return aNumber;
    }
    if (isNaN(aNumber) || aNumber == null) {
      return "0";
    }
    
    
    
    
    let localized = aNumber.toLocaleString(); 

    
    
    if (!localized.match(/[^\d]/)) {
      return localized;
    }

    let padded = localized + new Array(aDecimals).join("0"); 
    let match = padded.match("([^]*?\\d{" + aDecimals + "})\\d*$");
    return match.pop();
  }
};


















ViewHelpers.Prefs = function(aPrefsRoot = "", aPrefsObject = {}) {
  this._root = aPrefsRoot;
  this._cache = new Map();

  for (let accessorName in aPrefsObject) {
    let [prefType, prefName] = aPrefsObject[accessorName];
    this.map(accessorName, prefType, prefName);
  }
};

ViewHelpers.Prefs.prototype = {
  






  _get: function(aType, aPrefName) {
    let cachedPref = this._cache.get(aPrefName);
    if (cachedPref !== undefined) {
      return cachedPref;
    }
    let value = Services.prefs["get" + aType + "Pref"](aPrefName);
    this._cache.set(aPrefName, value);
    return value;
  },

  






  _set: function(aType, aPrefName, aValue) {
    Services.prefs["set" + aType + "Pref"](aPrefName, aValue);
    this._cache.set(aPrefName, aValue);
  },

  









  map: function(aAccessorName, aType, aPrefName, aSerializer = { in: e => e, out: e => e }) {
    if (aType == "Json") {
      this.map(aAccessorName, "Char", aPrefName, { in: JSON.parse, out: JSON.stringify });
      return;
    }

    Object.defineProperty(this, aAccessorName, {
      get: () => aSerializer.in(this._get(aType, [this._root, aPrefName].join("."))),
      set: (e) => this._set(aType, [this._root, aPrefName].join("."), aSerializer.out(e))
    });
  },

  


  refresh: function() {
    this._cache.clear();
  }
};






















function Item(aOwnerView, aElement, aValue, aAttachment) {
  this.ownerView = aOwnerView;
  this.attachment = aAttachment;
  this._value = aValue + "";
  this._prebuiltNode = aElement;
};

Item.prototype = {
  get value() { return this._value; },
  get target() { return this._target; },
  get prebuiltNode() { return this._prebuiltNode; },

  












  append: function(aElement, aOptions = {}) {
    let item = new Item(this, aElement, "", aOptions.attachment);

    
    
    
    this._entangleItem(item, this._target.appendChild(aElement));

    
    if (aOptions.attributes) {
      aOptions.attributes.forEach(e => item._target.setAttribute(e[0], e[1]));
    }
    if (aOptions.finalize) {
      item.finalize = aOptions.finalize;
    }

    
    return item;
  },

  





  remove: function(aItem) {
    if (!aItem) {
      return;
    }
    this._target.removeChild(aItem._target);
    this._untangleItem(aItem);
  },

  







  _entangleItem: function(aItem, aElement) {
    this._itemsByElement.set(aElement, aItem);
    aItem._target = aElement;
  },

  





  _untangleItem: function(aItem) {
    if (aItem.finalize) {
      aItem.finalize(aItem);
    }
    for (let childItem of aItem) {
      aItem.remove(childItem);
    }

    this._unlinkItem(aItem);
    aItem._target = null;
  },

  





  _unlinkItem: function(aItem) {
    this._itemsByElement.delete(aItem._target);
  },

  




  stringify: function() {
    return JSON.stringify({
      value: this._value,
      target: this._target + "",
      prebuiltNode: this._prebuiltNode + "",
      attachment: this.attachment
    }, null, 2);
  },

  _value: "",
  _target: null,
  _prebuiltNode: null,
  finalize: null,
  attachment: null
};



DevToolsUtils.defineLazyPrototypeGetter(Item.prototype, "_itemsByElement", Map);















































this.WidgetMethods = {
  



  set widget(aWidget) {
    this._widget = aWidget;


    
    
    XPCOMUtils.defineLazyGetter(this, "_itemsByValue", () => new Map());
    XPCOMUtils.defineLazyGetter(this, "_itemsByElement", () => new Map());
    XPCOMUtils.defineLazyGetter(this, "_stagedItems", () => []);

    
    if (ViewHelpers.isEventEmitter(aWidget)) {
      aWidget.on("keyPress", this._onWidgetKeyPress.bind(this));
      aWidget.on("mousePress", this._onWidgetMousePress.bind(this));
    }
  },

  



  get widget() this._widget,

  





























  push: function([aElement, aValue], aOptions = {}) {
    let item = new Item(this, aElement, aValue, aOptions.attachment);

    
    if (aOptions.staged) {
      
      
      aOptions.index = undefined;
      return void this._stagedItems.push({ item: item, options: aOptions });
    }
    
    if (!("index" in aOptions)) {
      return this._insertItemAt(this._findExpectedIndexFor(item), item, aOptions);
    }
    
    
    return this._insertItemAt(aOptions.index, item, aOptions);
  },

  







  commit: function(aOptions = {}) {
    let stagedItems = this._stagedItems;

    
    if (aOptions.sorted) {
      stagedItems.sort((a, b) => this._currentSortPredicate(a.item, b.item));
    }
    
    for (let { item, options } of stagedItems) {
      this._insertItemAt(-1, item, options);
    }
    
    this._stagedItems.length = 0;
  },

  





  remove: function(aItem) {
    if (!aItem) {
      return;
    }
    this._widget.removeChild(aItem._target);
    this._untangleItem(aItem);

    if (!this._itemsByElement.size) {
      this._preferredValue = this.selectedValue;
      this._widget.selectedItem = null;
      this._widget.setAttribute("emptyText", this._emptyText);
    }
  },

  





  removeAt: function(aIndex) {
    this.remove(this.getItemAtIndex(aIndex));
  },

  


  removeForPredicate: function(aPredicate) {
    let item;
    while (item = this.getItemForPredicate(aPredicate)) {
      this.remove(item);
    }
  },

  


  empty: function() {
    this._preferredValue = this.selectedValue;
    this._widget.selectedItem = null;
    this._widget.removeAllItems();
    this._widget.setAttribute("emptyText", this._emptyText);

    for (let [, item] of this._itemsByElement) {
      this._untangleItem(item);
    }

    this._itemsByValue.clear();
    this._itemsByElement.clear();
    this._stagedItems.length = 0;
  },

  





  ensureItemIsVisible: function(aItem) {
    this._widget.ensureElementIsVisible(aItem._target);
  },

  





  ensureIndexIsVisible: function(aIndex) {
    this.ensureItemIsVisible(this.getItemAtIndex(aIndex));
  },

  


  ensureSelectedItemIsVisible: function() {
    this.ensureItemIsVisible(this.selectedItem);
  },

  



  set emptyText(aValue) {
    this._emptyText = aValue;

    
    if (!this._itemsByElement.size) {
      this._widget.setAttribute("emptyText", aValue);
    }
  },

  




  set headerText(aValue) {
    this._headerText = aValue;
    this._widget.setAttribute("headerText", aValue);
  },

  








  toggleContents: function(aVisibleFlag) {
    for (let [element, item] of this._itemsByElement) {
      element.hidden = !aVisibleFlag;
    }
  },

  







  filterContents: function(aPredicate = this._currentFilterPredicate) {
    this._currentFilterPredicate = aPredicate;

    for (let [element, item] of this._itemsByElement) {
      element.hidden = !aPredicate(item);
    }
  },

  







  sortContents: function(aPredicate = this._currentSortPredicate) {
    let sortedItems = this.items.sort(this._currentSortPredicate = aPredicate);

    for (let i = 0, len = sortedItems.length; i < len; i++) {
      this.swapItems(this.getItemAtIndex(i), sortedItems[i]);
    }
  },

  







  swapItems: function(aFirst, aSecond) {
    if (aFirst == aSecond) { 
      return;
    }
    let { _prebuiltNode: firstPrebuiltTarget, _target: firstTarget } = aFirst;
    let { _prebuiltNode: secondPrebuiltTarget, _target: secondTarget } = aSecond;

    
    
    if (firstPrebuiltTarget instanceof Ci.nsIDOMDocumentFragment) {
      for (let node of firstTarget.childNodes) {
        firstPrebuiltTarget.appendChild(node.cloneNode(true));
      }
    }
    if (secondPrebuiltTarget instanceof Ci.nsIDOMDocumentFragment) {
      for (let node of secondTarget.childNodes) {
        secondPrebuiltTarget.appendChild(node.cloneNode(true));
      }
    }

    
    let i = this._indexOfElement(firstTarget);
    let j = this._indexOfElement(secondTarget);

    
    let selectedTarget = this._widget.selectedItem;
    let selectedIndex = -1;
    if (selectedTarget == firstTarget) {
      selectedIndex = i;
    } else if (selectedTarget == secondTarget) {
      selectedIndex = j;
    }

    
    this._widget.removeChild(firstTarget);
    this._widget.removeChild(secondTarget);
    this._unlinkItem(aFirst);
    this._unlinkItem(aSecond);

    
    this._insertItemAt.apply(this, i < j ? [i, aSecond] : [j, aFirst]);
    this._insertItemAt.apply(this, i < j ? [j, aFirst] : [i, aSecond]);

    
    if (selectedIndex == i) {
      this._widget.selectedItem = aFirst._target;
    } else if (selectedIndex == j) {
      this._widget.selectedItem = aSecond._target;
    }

    
    ViewHelpers.dispatchEvent(aFirst.target, "swap", [aSecond, aFirst]);
  },

  







  swapItemsAtIndices: function(aFirst, aSecond) {
    this.swapItems(this.getItemAtIndex(aFirst), this.getItemAtIndex(aSecond));
  },

  








  containsValue: function(aValue) {
    return this._itemsByValue.has(aValue) ||
           this._stagedItems.some(({ item }) => item._value == aValue);
  },

  




  get preferredValue() {
    return this._preferredValue;
  },

  



  get selectedItem() {
    let selectedElement = this._widget.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement);
    }
    return null;
  },

  



  get selectedIndex() {
    let selectedElement = this._widget.selectedItem;
    if (selectedElement) {
      return this._indexOfElement(selectedElement);
    }
    return -1;
  },

  



  get selectedValue() {
    let selectedElement = this._widget.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement)._value;
    }
    return "";
  },

  



  get selectedAttachment() {
    let selectedElement = this._widget.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement).attachment;
    }
    return null;
  },

  



  set selectedItem(aItem) {
    
    
    if (typeof aItem == "function") {
      aItem = this.getItemForPredicate(aItem);
    }

    
    let targetElement = aItem ? aItem._target : null;
    let prevElement = this._widget.selectedItem;

    
    if (this.autoFocusOnSelection && targetElement) {
      targetElement.focus();
    }
    if (this.maintainSelectionVisible && targetElement) {
      
      
      if ("ensureElementIsVisible" in this._widget) {
        this._widget.ensureElementIsVisible(targetElement);
      }
    }

    
    
    if (targetElement != prevElement) {
      this._widget.selectedItem = targetElement;
      let dispTarget = targetElement || prevElement;
      let dispName = this.suppressSelectionEvents ? "suppressed-select" : "select";
      ViewHelpers.dispatchEvent(dispTarget, dispName, aItem);
    }
  },

  



  set selectedIndex(aIndex) {
    let targetElement = this._widget.getItemAtIndex(aIndex);
    if (targetElement) {
      this.selectedItem = this._itemsByElement.get(targetElement);
      return;
    }
    this.selectedItem = null;
  },

  



  set selectedValue(aValue) {
    this.selectedItem = this._itemsByValue.get(aValue);
  },

  








  forceSelect: function(aItem) {
    this.selectedItem = null;
    this.selectedItem = aItem;
  },

  



  maintainSelectionVisible: true,

  






  suppressSelectionEvents: false,

  






  autoFocusOnFirstItem: true,

  









  autoFocusOnSelection: true,

  





  autoFocusOnInput: true,

  



  allowFocusOnRightClick: false,

  




  pageSize: 0,

  


  focusFirstVisibleItem: function() {
    this.focusItemAtDelta(-this.itemCount);
  },

  


  focusLastVisibleItem: function() {
    this.focusItemAtDelta(+this.itemCount);
  },

  


  focusNextItem: function() {
    this.focusItemAtDelta(+1);
  },

  


  focusPrevItem: function() {
    this.focusItemAtDelta(-1);
  },

  






  focusItemAtDelta: function(aDelta) {
    
    
    
    
    let selectedElement = this._widget.selectedItem;
    if (selectedElement) {
      selectedElement.focus();
    } else {
      this.selectedIndex = Math.max(0, aDelta - 1);
      return;
    }

    let direction = aDelta > 0 ? "advanceFocus" : "rewindFocus";
    let distance = Math.abs(Math[aDelta > 0 ? "ceil" : "floor"](aDelta));
    while (distance--) {
      if (!this._focusChange(direction)) {
        break; 
      }
    }

    
    this.selectedItem = this.getItemForElement(this._focusedElement);
  },

  








  _focusChange: function(aDirection) {
    let commandDispatcher = this._commandDispatcher;
    let prevFocusedElement = commandDispatcher.focusedElement;
    let currFocusedElement;

    do {
      commandDispatcher.suppressFocusScroll = true;
      commandDispatcher[aDirection]();
      currFocusedElement = commandDispatcher.focusedElement;

      
      
      if (!this.getItemForElement(currFocusedElement)) {
        prevFocusedElement.focus();
        return false;
      }
    } while (!WIDGET_FOCUSABLE_NODES.has(currFocusedElement.tagName));

    
    return true;
  },

  




  get _commandDispatcher() {
    if (this._cachedCommandDispatcher) {
      return this._cachedCommandDispatcher;
    }
    let someElement = this._widget.getItemAtIndex(0);
    if (someElement) {
      let commandDispatcher = someElement.ownerDocument.commandDispatcher;
      return this._cachedCommandDispatcher = commandDispatcher;
    }
    return null;
  },

  





  get _focusedElement() {
    let commandDispatcher = this._commandDispatcher;
    if (commandDispatcher) {
      return commandDispatcher.focusedElement;
    }
    return null;
  },

  







  getItemAtIndex: function(aIndex) {
    return this.getItemForElement(this._widget.getItemAtIndex(aIndex));
  },

  







  getItemByValue: function(aValue) {
    return this._itemsByValue.get(aValue);
  },

  











  getItemForElement: function(aElement, aFlags = {}) {
    while (aElement) {
      let item = this._itemsByElement.get(aElement);

      
      if (!aFlags.noSiblings) {
        item = item ||
          this._itemsByElement.get(aElement.nextElementSibling) ||
          this._itemsByElement.get(aElement.previousElementSibling);
      }
      if (item) {
        return item;
      }
      aElement = aElement.parentNode;
    }
    return null;
  },

  







  getItemForPredicate: function(aPredicate, aOwner = this) {
    
    for (let [element, item] of aOwner._itemsByElement) {
      let match;
      if (aPredicate(item) && !element.hidden) {
        match = item;
      } else {
        match = this.getItemForPredicate(aPredicate, item);
      }
      if (match) {
        return match;
      }
    }
    
    
    for (let { item } of this._stagedItems) {
      if (aPredicate(item)) {
        return item;
      }
    }
    return null;
  },

  



  getItemForAttachment: function(aPredicate, aOwner = this) {
    return this.getItemForPredicate(e => aPredicate(e.attachment));
  },

  







  indexOfItem: function(aItem) {
    return this._indexOfElement(aItem._target);
  },

  







  _indexOfElement: function(aElement) {
    for (let i = 0; i < this._itemsByElement.size; i++) {
      if (this._widget.getItemAtIndex(i) == aElement) {
        return i;
      }
    }
    return -1;
  },

  



  get itemCount() {
    return this._itemsByElement.size;
  },

  



  get items() {
    let store = [];
    let itemCount = this.itemCount;
    for (let i = 0; i < itemCount; i++) {
      store.push(this.getItemAtIndex(i));
    }
    return store;
  },

  



  get values() {
    return this.items.map(e => e._value);
  },

  



  get attachments() {
    return this.items.map(e => e.attachment);
  },

  




  get visibleItems() {
    return this.items.filter(e => !e._target.hidden);
  },

  








  isUnique: function(aItem) {
    let value = aItem._value;
    if (value == "" || value == "undefined" || value == "null") {
      return true;
    }
    return !this._itemsByValue.has(value);
  },

  








  isEligible: function(aItem) {
    return this.isUnique(aItem) && aItem._prebuiltNode;
  },

  








  _findExpectedIndexFor: function(aItem) {
    let itemCount = this.itemCount;
    for (let i = 0; i < itemCount; i++) {
      if (this._currentSortPredicate(this.getItemAtIndex(i), aItem) > 0) {
        return i;
      }
    }
    return itemCount;
  },

  













  _insertItemAt: function(aIndex, aItem, aOptions = {}) {
    if (!this.isEligible(aItem)) {
      return null;
    }

    
    
    
    let node = aItem._prebuiltNode;
    let attachment = aItem.attachment;
    this._entangleItem(aItem, this._widget.insertItemAt(aIndex, node, attachment));

    
    if (!this._currentFilterPredicate(aItem)) {
      aItem._target.hidden = true;
    }
    if (this.autoFocusOnFirstItem && this._itemsByElement.size == 1) {
      aItem._target.focus();
    }
    if (aOptions.attributes) {
      aOptions.attributes.forEach(e => aItem._target.setAttribute(e[0], e[1]));
    }
    if (aOptions.finalize) {
      aItem.finalize = aOptions.finalize;
    }

    
    this._widget.removeAttribute("emptyText");

    
    return aItem;
  },

  







  _entangleItem: function(aItem, aElement) {
    this._itemsByValue.set(aItem._value, aItem);
    this._itemsByElement.set(aElement, aItem);
    aItem._target = aElement;
  },

  





  _untangleItem: function(aItem) {
    if (aItem.finalize) {
      aItem.finalize(aItem);
    }
    for (let childItem of aItem) {
      aItem.remove(childItem);
    }

    this._unlinkItem(aItem);
    aItem._target = null;
  },

  





  _unlinkItem: function(aItem) {
    this._itemsByValue.delete(aItem._value);
    this._itemsByElement.delete(aItem._target);
  },

  




  _onWidgetKeyPress: function(aName, aEvent) {
    
    ViewHelpers.preventScrolling(aEvent);

    switch (aEvent.keyCode) {
      case aEvent.DOM_VK_UP:
      case aEvent.DOM_VK_LEFT:
        this.focusPrevItem();
        return;
      case aEvent.DOM_VK_DOWN:
      case aEvent.DOM_VK_RIGHT:
        this.focusNextItem();
        return;
      case aEvent.DOM_VK_PAGE_UP:
        this.focusItemAtDelta(-(this.pageSize || (this.itemCount / PAGE_SIZE_ITEM_COUNT_RATIO)));
        return;
      case aEvent.DOM_VK_PAGE_DOWN:
        this.focusItemAtDelta(+(this.pageSize || (this.itemCount / PAGE_SIZE_ITEM_COUNT_RATIO)));
        return;
      case aEvent.DOM_VK_HOME:
        this.focusFirstVisibleItem();
        return;
      case aEvent.DOM_VK_END:
        this.focusLastVisibleItem();
        return;
    }
  },

  




  _onWidgetMousePress: function(aName, aEvent) {
    if (aEvent.button != 0 && !this.allowFocusOnRightClick) {
      
      return;
    }

    let item = this.getItemForElement(aEvent.target);
    if (item) {
      
      this.selectedItem = item;
      
      this.autoFocusOnInput && item._target.focus();
    }
  },

  








  _currentFilterPredicate: function(aItem) {
    return true;
  },

  












  _currentSortPredicate: function(aFirst, aSecond) {
    return +(aFirst._value.toLowerCase() > aSecond._value.toLowerCase());
  },

  








  callMethod: function(aMethodName, ...aArgs) {
    return this._widget[aMethodName].apply(this._widget, aArgs);
  },

  _widget: null,
  _emptyText: "",
  _headerText: "",
  _preferredValue: "",
  _cachedCommandDispatcher: null
};




Item.prototype[Symbol.iterator] =
WidgetMethods[Symbol.iterator] = function*() {
  yield* this._itemsByElement.values();
};
