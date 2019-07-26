




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const PANE_APPEARANCE_DELAY = 50;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["ViewHelpers", "MenuItem", "MenuContainer"];




this.ViewHelpers = {
  










  create: function VH_create({ constructor, proto }, aProperties = {}) {
    let descriptors = {
      constructor: { value: constructor }
    };
    for (let name in aProperties) {
      descriptors[name] = Object.getOwnPropertyDescriptor(aProperties, name);
    }
    constructor.prototype = Object.create(proto, descriptors);
  },

  












  dispatchEvent: function VH_dispatchEvent(aTarget, aType, aDetail) {
    if (!aTarget) {
      return true; 
    }
    let document = aTarget.ownerDocument || aTarget;
    let dispatcher = aTarget.ownerDocument ? aTarget : document.documentElement;

    let event = document.createEvent("CustomEvent");
    event.initCustomEvent(aType, true, true, aDetail);
    return dispatcher.dispatchEvent(event);
  },

  








  delegateWidgetAttributeMethods: function VH_delegateWidgetAttributeMethods(aWidget, aNode) {
    aWidget.getAttribute = aNode.getAttribute.bind(aNode);
    aWidget.setAttribute = aNode.setAttribute.bind(aNode);
    aWidget.removeAttribute = aNode.removeAttribute.bind(aNode);
  },

  








  delegateWidgetEventMethods: function VH_delegateWidgetEventMethods(aWidget, aNode) {
    aWidget.addEventListener = aNode.addEventListener.bind(aNode);
    aWidget.removeEventListener = aNode.removeEventListener.bind(aNode);
  },

  











  togglePane: function VH_togglePane(aFlags, aPane) {
    
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







ViewHelpers.L10N = function L10N(aStringBundleName) {
  XPCOMUtils.defineLazyGetter(this, "stringBundle", () =>
    Services.strings.createBundle(aStringBundleName));

  XPCOMUtils.defineLazyGetter(this, "ellipsis", () =>
    Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data);
};

ViewHelpers.L10N.prototype = {
  stringBundle: null,

  





  getStr: function L10N_getStr(aName) {
    return this.stringBundle.GetStringFromName(aName);
  },

  






  getFormatStr: function L10N_getFormatStr(aName, ...aArgs) {
    return this.stringBundle.formatStringFromName(aName, aArgs, aArgs.length);
  },

  










  numberWithDecimals: function L10N__numberWithDecimals(aNumber, aDecimals = 0) {
    
    if (aNumber == (aNumber | 0)) {
      return aNumber;
    }
    
    
    
    
    let localized = aNumber.toLocaleString(); 
    let padded = localized + new Array(aDecimals).join("0"); 
    let match = padded.match("([^]*?\\d{" + aDecimals + "})\\d*$");
    return match.pop();
  }
};


















ViewHelpers.Prefs = function Prefs(aPrefsRoot = "", aPrefsObject = {}) {
  this.root = aPrefsRoot;

  for (let accessorName in aPrefsObject) {
    let [prefType, prefName] = aPrefsObject[accessorName];
    this.map(accessorName, prefType, prefName);
  }
};

ViewHelpers.Prefs.prototype = {
  






  _get: function P__get(aType, aPrefName) {
    if (this[aPrefName] === undefined) {
      this[aPrefName] = Services.prefs["get" + aType + "Pref"](aPrefName);
    }
    return this[aPrefName];
  },

  






  _set: function P__set(aType, aPrefName, aValue) {
    Services.prefs["set" + aType + "Pref"](aPrefName, aValue);
    this[aPrefName] = aValue;
  },

  






  map: function P_map(aAccessorName, aType, aPrefName) {
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

  












  append: function MI_append(aElement, aOptions = {}) {
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

  





  remove: function MI_remove(aItem) {
    if (!aItem) {
      return;
    }
    this._target.removeChild(aItem._target);
    this._untangleItem(aItem);
  },

  


  markSelected: function MI_markSelected() {
    if (!this._target) {
      return;
    }
    this._target.classList.add("selected");
  },

  


  markDeselected: function MI_markDeselected() {
    if (!this._target) {
      return;
    }
    this._target.classList.remove("selected");
  },

  







  setAttributes: function MI_setAttributes(aAttributes, aElement = this._target) {
    for (let [name, value] of aAttributes) {
      aElement.setAttribute(name, value);
    }
  },

  







  _entangleItem: function MI__entangleItem(aItem, aElement) {
    if (!this._itemsByElement) {
      this._itemsByElement = new Map(); 
    }

    this._itemsByElement.set(aElement, aItem);
    aItem._target = aElement;
  },

  





  _untangleItem: function MI__untangleItem(aItem) {
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

  





  _unlinkItem: function MC__unlinkItem(aItem) {
    this._itemsByElement.delete(aItem._target);
  },

  



  toString: function MI_toString() {
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

  


































  push: function MC_push(aContents, aOptions = {}) {
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

  







  commit: function MC_commit(aOptions = {}) {
    let stagedItems = this._stagedItems;

    
    if (aOptions.sorted) {
      stagedItems.sort((a, b) => this._sortPredicate(a.item, b.item));
    }
    
    for (let { item, options } of stagedItems) {
      this._insertItemAt(-1, item, options);
    }
    
    this._stagedItems.length = 0;
  },

  






  refresh: function MC_refresh() {
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

  





  remove: function MC_remove(aItem) {
    if (!aItem) {
      return;
    }
    this._container.removeChild(aItem._target);
    this._untangleItem(aItem);
  },

  


  empty: function MC_empty() {
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

  



  setUnavailable: function MC_setUnavailable() {
    this._container.setAttribute("notice", this.unavailableText);
    this._container.setAttribute("label", this.unavailableText);
    this._container.removeAttribute("tooltiptext");
  },

  



  emptyText: "",

  


  unavailableText: "",

  





  toggleContents: function MC_toggleContents(aVisibleFlag) {
    for (let [, item] of this._itemsByElement) {
      item._target.hidden = !aVisibleFlag;
    }
  },

  







  sortContents: function MC_sortContents(aPredicate = this._sortPredicate) {
    let sortedItems = this.allItems.sort(this._sortPredicate = aPredicate);

    for (let i = 0, len = sortedItems.length; i < len; i++) {
      this.swapItems(this.getItemAtIndex(i), sortedItems[i]);
    }
  },

  







  swapItems: function MC_swapItems(aFirst, aSecond) {
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

  







  swapItemsAtIndices: function MC_swapItemsAtIndices(aFirst, aSecond) {
    this.swapItems(this.getItemAtIndex(aFirst), this.getItemAtIndex(aSecond));
  },

  








  containsLabel: function MC_containsLabel(aLabel) {
    return this._itemsByLabel.has(aLabel) ||
           this._stagedItems.some(function({item}) item._label == aLabel);
  },

  








  containsValue: function MC_containsValue(aValue) {
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

    
    if (this._container.selectedItem == targetElement) {
      return;
    }

    this._container.selectedItem = targetElement;
    ViewHelpers.dispatchEvent(targetElement, "select", aItem);
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

  







  getItemAtIndex: function MC_getItemAtIndex(aIndex) {
    return this.getItemForElement(this._container.getItemAtIndex(aIndex));
  },

  







  getItemByLabel: function MC_getItemByLabel(aLabel) {
    return this._itemsByLabel.get(aLabel);
  },

  







  getItemByValue: function MC_getItemByValue(aValue) {
    return this._itemsByValue.get(aValue);
  },

  







  getItemForElement: function MC_getItemForElement(aElement) {
    while (aElement) {
      let item = this._itemsByElement.get(aElement);
      if (item) {
        return item;
      }
      aElement = aElement.parentNode;
    }
    return null;
  },

  







  indexOfItem: function MC_indexOfItem(aItem) {
    return this._indexOfElement(aItem._target);
  },

  







  _indexOfElement: function MC__indexOfElement(aElement) {
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

  







  isUnique: function MC_isUnique(aItem) {
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

  







  isEligible: function MC_isEligible(aItem) {
    return aItem._prebuiltTarget || (this.isUnique(aItem) &&
           aItem._label != "undefined" && aItem._label != "null" &&
           aItem._value != "undefined" && aItem._value != "null");
  },

  








  _findExpectedIndex: function MC__findExpectedIndex(aItem) {
    let container = this._container;
    let itemCount = this.itemCount;

    for (let i = 0; i < itemCount; i++) {
      if (this._sortPredicate(this.getItemAtIndex(i), aItem) > 0) {
        return i;
      }
    }
    return itemCount;
  },

  















  _insertItemAt: function MC__insertItemAt(aIndex, aItem, aOptions = {}) {
    
    if (!aOptions.relaxed && !this.isEligible(aItem)) {
      return null;
    }

    
    this._entangleItem(aItem, this._container.insertItemAt(aIndex,
      aItem._prebuiltTarget || aItem._label, 
      aItem._value,
      aItem._description,
      aItem.attachment));

    
    if (aOptions.attributes) {
      aItem.setAttributes(aOptions.attributes, aItem._target);
    }
    if (aOptions.finalize) {
      aItem.finalize = aOptions.finalize;
    }

    
    return aItem;
  },

  







  _entangleItem: function MC__entangleItem(aItem, aElement) {
    this._itemsByLabel.set(aItem._label, aItem);
    this._itemsByValue.set(aItem._value, aItem);
    this._itemsByElement.set(aElement, aItem);
    aItem._target = aElement;
  },

  





  _untangleItem: function MC__untangleItem(aItem) {
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

  





  _unlinkItem: function MI__unlinkItem(aItem) {
    this._itemsByLabel.delete(aItem._label);
    this._itemsByValue.delete(aItem._value);
    this._itemsByElement.delete(aItem._target);
  },

  












  _sortPredicate: function MC__sortPredicate(aFirst, aSecond) {
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
MenuContainer.prototype.__iterator__ = function VH_iterator() {
  if (!this._itemsByElement) {
    return;
  }
  for (let [, item] of this._itemsByElement) {
    yield item;
  }
};
