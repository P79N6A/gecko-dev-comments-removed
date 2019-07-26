




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

this.EXPORTED_SYMBOLS = [
  "Heritage", "ViewHelpers", "WidgetMethods",
  "setNamedTimeout", "clearNamedTimeout"
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












this.setNamedTimeout = function(aId, aWait, aCallback) {
  clearNamedTimeout(aId);

  namedTimeoutsStore.set(aId, setTimeout(() =>
    namedTimeoutsStore.delete(aId) && aCallback(), aWait));
};








this.clearNamedTimeout = function(aId) {
  if (!namedTimeoutsStore) {
    return;
  }
  clearTimeout(namedTimeoutsStore.get(aId));
  namedTimeoutsStore.delete(aId);
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
    aWidget.getAttribute = aNode.getAttribute.bind(aNode);
    aWidget.setAttribute = aNode.setAttribute.bind(aNode);
    aWidget.removeAttribute = aNode.removeAttribute.bind(aNode);
  },

  







  delegateWidgetEventMethods: function(aWidget, aNode) {
    aWidget.addEventListener = aNode.addEventListener.bind(aNode);
    aWidget.removeEventListener = aNode.removeEventListener.bind(aNode);
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
    
    
    
    
    let localized = aNumber.toLocaleString(); 
    let padded = localized + new Array(aDecimals).join("0"); 
    let match = padded.match("([^]*?\\d{" + aDecimals + "})\\d*$");
    return match.pop();
  }
};


















ViewHelpers.Prefs = function(aPrefsRoot = "", aPrefsObject = {}) {
  this.root = aPrefsRoot;

  for (let accessorName in aPrefsObject) {
    let [prefType, prefName] = aPrefsObject[accessorName];
    this.map(accessorName, prefType, prefName);
  }
};

ViewHelpers.Prefs.prototype = {
  






  _get: function(aType, aPrefName) {
    if (this[aPrefName] === undefined) {
      this[aPrefName] = Services.prefs["get" + aType + "Pref"](aPrefName);
    }
    return this[aPrefName];
  },

  






  _set: function(aType, aPrefName, aValue) {
    Services.prefs["set" + aType + "Pref"](aPrefName, aValue);
    this[aPrefName] = aValue;
  },

  






  map: function(aAccessorName, aType, aPrefName) {
    Object.defineProperty(this, aAccessorName, {
      get: () => this._get(aType, [this.root, aPrefName].join(".")),
      set: (aValue) => this._set(aType, [this.root, aPrefName].join("."), aValue)
    });
  }
};
















function Item(aOwnerView, aAttachment, aContents = []) {
  this.ownerView = aOwnerView;
  this.attachment = aAttachment;

  let [aLabel, aValue, aDescription] = aContents;
  
  this._label = aLabel + "";
  this._value = aValue + "";
  
  if (aDescription !== undefined) {
    this._description = aDescription + "";
  }

  
  
  if (ViewHelpers.isNode(aLabel)) {
    this._prebuiltTarget = aLabel;
  }

  XPCOMUtils.defineLazyGetter(this, "_itemsByElement", () => new Map());
};

Item.prototype = {
  get label() this._label,
  get value() this._value,
  get description() this._description,
  get target() this._target,

  












  append: function(aElement, aOptions = {}) {
    let item = new Item(this, aOptions.attachment);

    
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
    for (let childItem in aItem) {
      aItem.remove(childItem);
    }

    this._unlinkItem(aItem);
    aItem._prebuiltTarget = null;
    aItem._target = null;
  },

  





  _unlinkItem: function(aItem) {
    this._itemsByElement.delete(aItem._target);
  },

  



  toString: function() {
    if (this._label != "undefined" && this._value != "undefined") {
      return this._label + " -> " + this._value;
    }
    if (this.attachment) {
      return this.attachment.toString();
    }
    return "(null)";
  },

  _label: "",
  _value: "",
  _description: undefined,
  _prebuiltTarget: null,
  _target: null,
  finalize: null,
  attachment: null
};








































this.WidgetMethods = {
  



  set widget(aWidget) {
    this._widget = aWidget;

    
    
    XPCOMUtils.defineLazyGetter(this, "_itemsByLabel", () => new Map());
    XPCOMUtils.defineLazyGetter(this, "_itemsByValue", () => new Map());
    XPCOMUtils.defineLazyGetter(this, "_itemsByElement", () => new Map());
    XPCOMUtils.defineLazyGetter(this, "_stagedItems", () => []);

    
    if (ViewHelpers.isEventEmitter(aWidget)) {
      aWidget.on("keyPress", this._onWidgetKeyPress.bind(this));
      aWidget.on("mousePress", this._onWidgetMousePress.bind(this));
    }
  },

  



  get widget() this._widget,

  




































  push: function(aContents, aOptions = {}) {
    let item = new Item(this, aOptions.attachment, aContents);

    
    if (aOptions.staged) {
      
      delete aOptions.index;
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

  






  refresh: function() {
    let selectedItem = this.selectedItem;
    if (!selectedItem) {
      return false;
    }

    let { _label: label, _value: value, _description: desc } = selectedItem;
    this._widget.removeAttribute("notice");
    this._widget.setAttribute("label", label);
    this._widget.setAttribute("tooltiptext", desc !== undefined ? desc : value);

    return true;
  },

  





  remove: function(aItem) {
    if (!aItem) {
      return;
    }
    this._widget.removeChild(aItem._target);
    this._untangleItem(aItem);
  },

  





  removeAt: function(aIndex) {
    this.remove(this.getItemAtIndex(aIndex));
  },

  


  empty: function() {
    this._preferredValue = this.selectedValue;
    this._widget.selectedItem = null;
    this._widget.removeAllItems();
    this._widget.setAttribute("notice", this.emptyText);
    this._widget.setAttribute("label", this.emptyText);
    this._widget.removeAttribute("tooltiptext");

    for (let [, item] of this._itemsByElement) {
      this._untangleItem(item);
    }

    this._itemsByLabel.clear();
    this._itemsByValue.clear();
    this._itemsByElement.clear();
    this._stagedItems.length = 0;
  },

  



  emptyText: "",

  








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
    let sortedItems = this.orderedItems.sort(this._currentSortPredicate = aPredicate);

    for (let i = 0, len = sortedItems.length; i < len; i++) {
      this.swapItems(this.getItemAtIndex(i), sortedItems[i]);
    }
  },

  







  swapItems: function(aFirst, aSecond) {
    if (aFirst == aSecond) { 
      return;
    }
    let { _prebuiltTarget: firstPrebuiltTarget, target: firstTarget } = aFirst;
    let { _prebuiltTarget: secondPrebuiltTarget, target: secondTarget } = aSecond;

    
    
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
  },

  







  swapItemsAtIndices: function(aFirst, aSecond) {
    this.swapItems(this.getItemAtIndex(aFirst), this.getItemAtIndex(aSecond));
  },

  








  containsLabel: function(aLabel) {
    return this._itemsByLabel.has(aLabel) ||
           this._stagedItems.some(({ item }) => item._label == aLabel);
  },

  








  containsValue: function(aValue) {
    return this._itemsByValue.has(aValue) ||
           this._stagedItems.some(({ item }) => item._value == aValue);
  },

  




  get preferredValue() this._preferredValue,

  



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

  



  get selectedLabel() {
    let selectedElement = this._widget.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement)._label;
    }
    return "";
  },

  



  get selectedValue() {
    let selectedElement = this._widget.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement)._value;
    }
    return "";
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

    
    
    if (targetElement == prevElement) {
      return;
    }
    this._widget.selectedItem = targetElement;
    ViewHelpers.dispatchEvent(targetElement || prevElement, "select", aItem);

    
    
    this.refresh();
  },

  



  set selectedIndex(aIndex) {
    let targetElement = this._widget.getItemAtIndex(aIndex);
    if (targetElement) {
      this.selectedItem = this._itemsByElement.get(targetElement);
      return;
    }
    this.selectedItem = null;
  },

  



  set selectedLabel(aLabel)
    this.selectedItem = this._itemsByLabel.get(aLabel),

  



  set selectedValue(aValue)
    this.selectedItem = this._itemsByValue.get(aValue),

  






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

  







  getItemByLabel: function(aLabel) {
    return this._itemsByLabel.get(aLabel);
  },

  







  getItemByValue: function(aValue) {
    return this._itemsByValue.get(aValue);
  },

  







  getItemForElement: function(aElement) {
    while (aElement) {
      let item =
        this._itemsByElement.get(aElement) ||
        this._itemsByElement.get(aElement.nextElementSibling) ||
        this._itemsByElement.get(aElement.previousElementSibling);
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
    return null;
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

  



  get itemCount() this._itemsByElement.size,

  



  get items() {
    let items = [];
    for (let [, item] of this._itemsByElement) {
      items.push(item);
    }
    return items;
  },

  



  get labels() {
    let labels = [];
    for (let [label] of this._itemsByLabel) {
      labels.push(label);
    }
    return labels;
  },

  



  get values() {
    let values = [];
    for (let [value] of this._itemsByValue) {
      values.push(value);
    }
    return values;
  },

  




  get visibleItems() {
    let items = [];
    for (let [element, item] of this._itemsByElement) {
      if (!element.hidden) {
        items.push(item);
      }
    }
    return items;
  },

  



  get orderedItems() {
    let items = [];
    let itemCount = this.itemCount;
    for (let i = 0; i < itemCount; i++) {
      items.push(this.getItemAtIndex(i));
    }
    return items;
  },

  




  get orderedVisibleItems() {
    let items = [];
    let itemCount = this.itemCount;
    for (let i = 0; i < itemCount; i++) {
      let item = this.getItemAtIndex(i);
      if (!item._target.hidden) {
        items.push(item);
      }
    }
    return items;
  },

  







  uniquenessQualifier: 1,

  







  isUnique: function(aItem) {
    switch (this.uniquenessQualifier) {
      case 1:
        return !this._itemsByLabel.has(aItem._label) &&
               !this._itemsByValue.has(aItem._value);
      case 2:
        return !this._itemsByLabel.has(aItem._label) ||
               !this._itemsByValue.has(aItem._value);
      case 3:
        return !this._itemsByLabel.has(aItem._label);
      case 4:
        return !this._itemsByValue.has(aItem._value);
    }
    return false;
  },

  







  isEligible: function(aItem) {
    let isUnique = this.isUnique(aItem);
    let isPrebuilt = !!aItem._prebuiltTarget;
    let isDegenerate = aItem._label == "undefined" || aItem._label == "null" ||
                       aItem._value == "undefined" || aItem._value == "null";

    return isPrebuilt || (isUnique && !isDegenerate);
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
    
    if (!aOptions.relaxed && !this.isEligible(aItem)) {
      return null;
    }

    
    this._entangleItem(aItem, this._widget.insertItemAt(aIndex,
      aItem._prebuiltTarget || aItem._label, 
      aItem._value,
      aItem._description,
      aItem.attachment));

    
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

    
    return aItem;
  },

  







  _entangleItem: function(aItem, aElement) {
    this._itemsByLabel.set(aItem._label, aItem);
    this._itemsByValue.set(aItem._value, aItem);
    this._itemsByElement.set(aElement, aItem);
    aItem._target = aElement;
  },

  





  _untangleItem: function(aItem) {
    if (aItem.finalize) {
      aItem.finalize(aItem);
    }
    for (let childItem in aItem) {
      aItem.remove(childItem);
    }

    this._unlinkItem(aItem);
    aItem._prebuiltTarget = null;
    aItem._target = null;
  },

  





  _unlinkItem: function(aItem) {
    this._itemsByLabel.delete(aItem._label);
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
    return +(aFirst._label.toLowerCase() > aSecond._label.toLowerCase());
  },

  








  callMethod: function(aMethodName, ...aArgs) {
    return this._widget[aMethodName].apply(this._widget, aArgs);
  },

  _widget: null,
  _preferredValue: null,
  _cachedCommandDispatcher: null
};




Item.prototype.__iterator__ =
WidgetMethods.__iterator__ = function() {
  for (let [, item] of this._itemsByElement) {
    yield item;
  }
};
