




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const PANE_APPEARANCE_DELAY = 50;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["ViewHelpers", "MenuItem", "MenuContainer"];




this.ViewHelpers = {
  










  create: function({ constructor, proto }, aProperties = {}) {
    let descriptors = {
      constructor: { value: constructor }
    };
    for (let name in aProperties) {
      descriptors[name] = Object.getOwnPropertyDescriptor(aProperties, name);
    }
    constructor.prototype = Object.create(proto, descriptors);
  },

  












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

  











  togglePane: function(aFlags, aPane) {
    
    aPane.removeAttribute("hidden");

    
    if (!aPane.classList.contains("generic-toggled-side-pane")) {
      aPane.classList.add("generic-toggled-side-pane");
    }

    
    if (aFlags.visible == !aPane.hasAttribute("pane-collapsed")) {
      if (aFlags.callback) aFlags.callback();
      return;
    }

    
    function set() {
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

    
    if (aFlags.animated) {
      aPane.setAttribute("animated", "");
    } else {
      aPane.removeAttribute("animated");
    }

    
    
    if (aFlags.delayed) {
      aPane.ownerDocument.defaultView.setTimeout(set.bind(this), PANE_APPEARANCE_DELAY);
    } else {
      set.call(this);
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
    let newArgs = aArgs.map(x => (typeof x == 'number' ?
                                  this.numberWithDecimals(x, 2) : x));
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














this.MenuItem = function MenuItem(aAttachment, aContents = []) {
  this.attachment = aAttachment;

  
  if (aContents instanceof Ci.nsIDOMNode ||
      aContents instanceof Ci.nsIDOMDocumentFragment) {
    this._prebuiltTarget = aContents;
  }
  
  else {
    let [aLabel, aValue, aDescription] = aContents;
    this._label = aLabel + "";
    this._value = aValue + "";
    this._description = (aDescription || "") + "";
  }
};

MenuItem.prototype = {
  get label() this._label,
  get value() this._value,
  get description() this._description,
  get target() this._target,

  












  append: function(aElement, aOptions = {}) {
    let item = new MenuItem(aOptions.attachment);

    
    if (aOptions.attributes) {
      this.setAttributes(aOptions.attributes);
    }
    if (aOptions.finalize) {
      item.finalize = aOptions.finalize;
    }

    
    this._entangleItem(item, this._target.appendChild(aElement));

    
    return item;
  },

  





  remove: function(aItem) {
    if (!aItem) {
      return;
    }
    this._target.removeChild(aItem._target);
    this._untangleItem(aItem);
  },

  


  markSelected: function() {
    if (!this._target) {
      return;
    }
    this._target.classList.add("selected");
  },

  


  markDeselected: function() {
    if (!this._target) {
      return;
    }
    this._target.classList.remove("selected");
  },

  







  setAttributes: function(aAttributes, aElement = this._target) {
    for (let [name, value] of aAttributes) {
      aElement.setAttribute(name, value);
    }
  },

  







  _entangleItem: function(aItem, aElement) {
    if (!this._itemsByElement) {
      this._itemsByElement = new Map(); 
    }

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
    if (this._label && this._value) {
      return this._label + " -> " + this._value;
    }
    if (this.attachment) {
      return this.attachment.toString();
    }
    return "(null)";
  },

  _label: "",
  _value: "",
  _description: "",
  _prebuiltTarget: null,
  _target: null,
  finalize: null,
  attachment: null
};
























this.MenuContainer = function MenuContainer() {
};

MenuContainer.prototype = {
  



  set node(aWidget) {
    this._container = aWidget;
    this._itemsByLabel = new Map();   
    this._itemsByValue = new Map();   
    this._itemsByElement = new Map(); 
    this._stagedItems = [];
  },

  



  get node() this._container,

  


































  push: function(aContents, aOptions = {}) {
    let item = new MenuItem(aOptions.attachment, aContents);

    
    if (aOptions.staged) {
      
      delete aOptions.index;
      return void this._stagedItems.push({ item: item, options: aOptions });
    }
    
    if (!("index" in aOptions)) {
      return this._insertItemAt(this._findExpectedIndex(item), item, aOptions);
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
    let selectedValue = this.selectedValue;
    if (!selectedValue) {
      return false;
    }
    let entangledLabel = this.getItemByValue(selectedValue)._label;
    this._container.removeAttribute("notice");
    this._container.setAttribute("label", entangledLabel);
    this._container.setAttribute("tooltiptext", selectedValue);
    return true;
  },

  





  remove: function(aItem) {
    if (!aItem) {
      return;
    }
    this._container.removeChild(aItem._target);
    this._untangleItem(aItem);
  },

  


  empty: function() {
    this._preferredValue = this.selectedValue;
    this._container.selectedItem = null;
    this._container.removeAllItems();
    this._container.setAttribute("notice", this.emptyText);
    this._container.setAttribute("label", this.emptyText);
    this._container.removeAttribute("tooltiptext");

    for (let [, item] of this._itemsByElement) {
      this._untangleItem(item);
    }

    this._itemsByLabel.clear();
    this._itemsByValue.clear();
    this._itemsByElement.clear();
    this._stagedItems.length = 0;
  },

  



  setUnavailable: function() {
    this._container.setAttribute("notice", this.unavailableText);
    this._container.setAttribute("label", this.unavailableText);
    this._container.removeAttribute("tooltiptext");
  },

  



  emptyText: "",

  


  unavailableText: "",

  









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
    let sortedItems = this.allItems.sort(this._currentSortPredicate = aPredicate);

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

    
    let selectedTarget = this._container.selectedItem;
    let selectedIndex = -1;
    if (selectedTarget == firstTarget) {
      selectedIndex = i;
    } else if (selectedTarget == secondTarget) {
      selectedIndex = j;
    }

    
    this._container.removeChild(firstTarget);
    this._container.removeChild(secondTarget);
    this._unlinkItem(aFirst);
    this._unlinkItem(aSecond);

    
    this._insertItemAt.apply(this, i < j ? [i, aSecond] : [j, aFirst]);
    this._insertItemAt.apply(this, i < j ? [j, aFirst] : [i, aSecond]);

    
    if (selectedIndex == i) {
      this._container.selectedItem = aFirst._target;
    } else if (selectedIndex == j) {
      this._container.selectedItem = aSecond._target;
    }
  },

  







  swapItemsAtIndices: function(aFirst, aSecond) {
    this.swapItems(this.getItemAtIndex(aFirst), this.getItemAtIndex(aSecond));
  },

  








  containsLabel: function(aLabel) {
    return this._itemsByLabel.has(aLabel) ||
           this._stagedItems.some(function({item}) item._label == aLabel);
  },

  








  containsValue: function(aValue) {
    return this._itemsByValue.has(aValue) ||
           this._stagedItems.some(function({item}) item._value == aValue);
  },

  



  get preferredValue() this._preferredValue,

  



  get selectedItem() {
    let selectedElement = this._container.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement);
    }
    return null;
  },

  



  get selectedIndex() {
    let selectedElement = this._container.selectedItem;
    if (selectedElement) {
      return this._indexOfElement(selectedElement);
    }
    return -1;
  },

  



  get selectedLabel() {
    let selectedElement = this._container.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement)._label;
    }
    return "";
  },

  



  get selectedValue() {
    let selectedElement = this._container.selectedItem;
    if (selectedElement) {
      return this._itemsByElement.get(selectedElement)._value;
    }
    return "";
  },

  



  set selectedItem(aItem) {
    
    let targetElement = aItem ? aItem._target : null;
    let prevElement = this._container.selectedItem;

    
    if (targetElement == prevElement) {
      return;
    }
    this._container.selectedItem = targetElement;
    ViewHelpers.dispatchEvent(targetElement || prevElement, "select", aItem);
  },

  



  set selectedIndex(aIndex) {
    let targetElement = this._container.getItemAtIndex(aIndex);
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

  







  getItemAtIndex: function(aIndex) {
    return this.getItemForElement(this._container.getItemAtIndex(aIndex));
  },

  







  getItemByLabel: function(aLabel) {
    return this._itemsByLabel.get(aLabel);
  },

  







  getItemByValue: function(aValue) {
    return this._itemsByValue.get(aValue);
  },

  







  getItemForElement: function(aElement) {
    while (aElement) {
      let item = this._itemsByElement.get(aElement);
      if (item) {
        return item;
      }
      aElement = aElement.parentNode;
    }
    return null;
  },

  







  indexOfItem: function(aItem) {
    return this._indexOfElement(aItem._target);
  },

  







  _indexOfElement: function(aElement) {
    let container = this._container;
    let itemCount = this._itemsByElement.size;

    for (let i = 0; i < itemCount; i++) {
      if (container.getItemAtIndex(i) == aElement) {
        return i;
      }
    }
    return -1;
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

  



  get itemCount() this._itemsByElement.size,

  



  get allItems() {
    let items = [];
    for (let i = 0; i < this.itemCount; i++) {
      items.push(this.getItemAtIndex(i));
    }
    return items;
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
    return aItem._prebuiltTarget || (this.isUnique(aItem) &&
           aItem._label != "undefined" && aItem._label != "null" &&
           aItem._value != "undefined" && aItem._value != "null");
  },

  








  _findExpectedIndex: function(aItem) {
    let container = this._container;
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

    
    this._entangleItem(aItem, this._container.insertItemAt(aIndex,
      aItem._prebuiltTarget || aItem._label, 
      aItem._value,
      aItem._description,
      aItem.attachment));

    
    if (!this._currentFilterPredicate(aItem)) {
      aItem._target.hidden = true;
    }
    if (aOptions.attributes) {
      aItem.setAttributes(aOptions.attributes, aItem._target);
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

  








  _currentFilterPredicate: function(aItem) {
    return true;
  },

  












  _currentSortPredicate: function(aFirst, aSecond) {
    return +(aFirst._label.toLowerCase() > aSecond._label.toLowerCase());
  },

  _container: null,
  _stagedItems: null,
  _itemsByLabel: null,
  _itemsByValue: null,
  _itemsByElement: null,
  _preferredValue: null
};




MenuItem.prototype.__iterator__ =
MenuContainer.prototype.__iterator__ = function() {
  if (!this._itemsByElement) {
    return;
  }
  for (let [, item] of this._itemsByElement) {
    yield item;
  }
};
