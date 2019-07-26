





"use strict";

module.metadata = {
  "stability": "experimental"
};

exports.Collection = Collection;













exports.addCollectionProperty = function addCollProperty(obj, propName, array) {
  array = array || [];
  let publicIface = new Collection(array);

  Object.defineProperty(obj, propName, {
    configurable: true,
    enumerable: true,

    set: function set(itemOrItems) {
      array.splice(0, array.length);
      publicIface.add(itemOrItems);
    },

    get: function get() {
      return publicIface;
    }
  });
};










function Collection(array) {
  array = array || [];

  



  this.__iterator__ = function Collection___iterator__() {
    let items = array.slice();
    for (let i = 0; i < items.length; i++)
      yield items[i];
  };

  


  this.__defineGetter__("length", function Collection_get_length() {
    return array.length;
  });

  







  this.add = function Collection_add(itemOrItems) {
    let items = toArray(itemOrItems);
    for (let i = 0; i < items.length; i++) {
      let item = items[i];
      if (array.indexOf(item) < 0)
        array.push(item);
    }
    return this;
  };

  







  this.remove = function Collection_remove(itemOrItems) {
    let items = toArray(itemOrItems);
    for (let i = 0; i < items.length; i++) {
      let idx = array.indexOf(items[i]);
      if (idx >= 0)
        array.splice(idx, 1);
    }
    return this;
  };
};

function toArray(itemOrItems) {
  let isArr = itemOrItems &&
              itemOrItems.constructor &&
              itemOrItems.constructor.name === "Array";
  return isArr ? itemOrItems : [itemOrItems];
}
