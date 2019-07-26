




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

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

  








  delegateWidgetAttributeMethods: function MC_delegateWidgetAttributeMethods(aWidget, aNode) {
    aWidget.getAttribute = aNode.getAttribute.bind(aNode);
    aWidget.setAttribute = aNode.setAttribute.bind(aNode);
    aWidget.removeAttribute = aNode.removeAttribute.bind(aNode);
  },

  








  delegateWidgetEventMethods: function MC_delegateWidgetEventMethods(aWidget, aNode) {
    aWidget.addEventListener = aNode.addEventListener.bind(aNode);
    aWidget.removeEventListener = aNode.removeEventListener.bind(aNode);
  }
};















this.MenuItem = function MenuItem(aAttachment, aLabel, aValue, aDescription) {
  this.attachment = aAttachment;
  this._label = aLabel + "";
  this._value = aValue + "";
  this._description = (aDescription || "") + "";
};

MenuItem.prototype = {
  



  get label() this._label,

  



  get value() this._value,

  



  get description() this._description,

  












  append: function MI_append(aElement, aOptions = {}) {
    let item = new MenuItem(aOptions.attachment);

    
    if (aOptions.attributes) {
      this.setAttributes(aOptions.attributes);
    }
    if (aOptions.finalize) {
      item.finalize = aOptions.finalize;
    }

    
    this._entangleItem(item, this.target.appendChild(aElement));

    
    return item;
  },

  





  remove: function MI_remove(aItem) {
    if (!aItem) {
      return;
    }
    this.target.removeChild(aItem.target);
    this._untangleItem(aItem);
  },

  


  markSelected: function MI_markSelected() {
    if (!this.target) {
      return;
    }
    this.target.classList.add("selected");
  },

  


  markDeselected: function MI_markDeselected() {
    if (!this.target) {
      return;
    }
    this.target.classList.remove("selected");
  },

  







  setAttributes: function MI_setAttributes(aAttributes, aElement = this.target) {
    for (let [name, value] of aAttributes) {
      aElement.setAttribute(name, value);
    }
  },

  







  _entangleItem: function MI__entangleItem(aItem, aElement) {
    if (!this._itemsByElement) {
      this._itemsByElement = new Map();
    }

    this._itemsByElement.set(aElement, aItem);
    aItem.target = aElement;
  },

  





  _untangleItem: function MI__untangleItem(aItem) {
    if (aItem.finalize) {
      aItem.finalize(aItem);
    }
    for (let childItem in aItem) {
      aItem.remove(childItem);
    }

    this._itemsByElement.delete(aItem.target);
    aItem.target = null;
  },

  



  toString: function MI_toString() {
    return this._label + " -> " + this._value;
  },

  _label: "",
  _value: "",
  _description: "",
  target: null,
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
    if (aContents instanceof Ci.nsIDOMNode ||
        aContents instanceof Ci.nsIDOMElement) {
      
      aOptions.node = aContents;
      aContents = [];
    }

    let [label, value, description] = aContents;
    let item = new MenuItem(aOptions.attachment, label, value, description);

    
    if (aOptions.staged) {
      return void this._stagedItems.push({ item: item, options: aOptions });
    }
    
    if (!("index" in aOptions)) {
      return this._insertItemAt(this._findExpectedIndex(label), item, aOptions);
    }
    
    
    return this._insertItemAt(aOptions.index, item, aOptions);
  },

  






  commit: function MC_commit(aOptions = {}) {
    let stagedItems = this._stagedItems;

    
    if (aOptions.sorted) {
      stagedItems.sort(function(a, b) a.item._label.toLowerCase() >
                                      b.item._label.toLowerCase());
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
    this._container.removeChild(aItem.target);
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

    this._itemsByLabel = new Map();
    this._itemsByValue = new Map();
    this._itemsByElement = new Map();
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
      item.target.hidden = !aVisibleFlag;
    }
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
    return -1;
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
    
    let targetNode = aItem ? aItem.target : null;

    
    if (this._container.selectedItem == targetNode) {
      return;
    }
    this._container.selectedItem = targetNode;
    ViewHelpers.dispatchEvent(targetNode, "select", aItem);
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
    return this._indexOfElement(aItem.target);
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
    return this.isUnique(aItem) &&
           aItem._label != "undefined" && aItem._label != "null" &&
           aItem._value != "undefined" && aItem._value != "null";
  },

  







  _findExpectedIndex: function MC__findExpectedIndex(aLabel) {
    let container = this._container;
    let itemCount = this.itemCount;

    for (let i = 0; i < itemCount; i++) {
      if (this.getItemAtIndex(i)._label > aLabel) {
        return i;
      }
    }
    return itemCount;
  },

  















  _insertItemAt: function MC__insertItemAt(aIndex, aItem, aOptions) {
    
    if (!aOptions.relaxed && !this.isEligible(aItem)) {
      return null;
    }

    
    this._entangleItem(aItem, this._container.insertItemAt(aIndex,
      aOptions.node || aItem._label,
      aItem._value,
      aItem._description,
      aOptions.attachment));

    
    if (aOptions.attributes) {
      aItem.setAttributes(aOptions.attributes, aItem.target);
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
    aItem.target = aElement;
  },

  





  _untangleItem: function MC__untangleItem(aItem) {
    if (aItem.finalize) {
      aItem.finalize(aItem);
    }
    for (let childItem in aItem) {
      aItem.remove(childItem);
    }

    this._itemsByLabel.delete(aItem._label);
    this._itemsByValue.delete(aItem._value);
    this._itemsByElement.delete(aItem.target);
    aItem.target = null;
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
