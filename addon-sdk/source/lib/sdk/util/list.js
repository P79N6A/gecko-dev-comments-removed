


'use strict';

module.metadata = {
  "stability": "experimental"
};

const { Class } = require('../core/heritage');
const listNS = require('../core/namespace').ns();

const List = Class({
  





  initialize: function List() {
    listNS(this).keyValueMap = [];

    for (let i = 0, ii = arguments.length; i < ii; i++)
      addListItem(this, arguments[i]);
  },
  



  get length() listNS(this).keyValueMap.length,
   



  toString: function toString() 'List(' + listNS(this).keyValueMap + ')',
  






  __iterator__: function __iterator__(onKeys, onKeyValue) {
    let array = listNS(this).keyValueMap.slice(0),
                i = -1;
    for each(let element in array)
      yield onKeyValue ? [++i, element] : onKeys ? ++i : element;
  }
});
exports.List = List;

function addListItem(that, value) {
  let list = listNS(that).keyValueMap,
      index = list.indexOf(value);

  if (-1 === index) {
    try {
      that[that.length] = value;
    }
    catch (e) {}
    list.push(value);
  }
}
exports.addListItem = addListItem;

function removeListItem(that, element) {
  let list = listNS(that).keyValueMap,
      index = list.indexOf(element);

  if (0 <= index) {
    list.splice(index, 1);
    try {
      for (let length = list.length; index < length; index++)
        that[index] = list[index];
      that[list.length] = undefined;
    }
    catch(e){}
  }
}
exports.removeListItem = removeListItem;

exports.listNS = listNS;
