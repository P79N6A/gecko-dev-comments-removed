


"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Trait } = require('../deprecated/traits');




const Iterable = Trait.compose({
  




  _keyValueMap: Trait.required,
  



  __iterator__: function __iterator__(onKeys, onKeyValue) {
    let map = this._keyValueMap;
    for (let key in map)
      yield onKeyValue ? [key, map[key]] : onKeys ? key : map[key];
  }
});
exports.Iterable = Iterable;








const List = Trait.resolve({ toString: null }).compose({
  _keyValueMap: null,
  





  constructor: function List() {
    this._keyValueMap = [];
    for (let i = 0, ii = arguments.length; i < ii; i++)
      this._add(arguments[i]);
  },
  



  get length() this._keyValueMap.length,
   



  toString: function toString() 'List(' + this._keyValueMap + ')',
  




  _has: function _has(element) 0 <= this._keyValueMap.indexOf(element),
  




  _add: function _add(element) {
    let list = this._keyValueMap,
        index = list.indexOf(element);
    if (0 > index)
      list.push(this._public[list.length] = element);
  },
  




  _remove: function _remove(element) {
    let list = this._keyValueMap,
        index = list.indexOf(element);
    if (0 <= index) {
      delete this._public[list.length - 1];
      list.splice(index, 1);
      for (let length = list.length; index < length; index++)
        this._public[index] = list[index];
    }
  },
  


  _clear: function _clear() {
    for (let i = 0, ii = this._keyValueMap.length; i < ii; i ++)
      delete this._public[i];
    this._keyValueMap.splice(0);
  },
  






  __iterator__: function __iterator__(onKeys, onKeyValue) {
    let array = this._keyValueMap.slice(0),
        i = -1;
    for (let element of array)
      yield onKeyValue ? [++i, element] : onKeys ? ++i : element;
  },
  iterator: function iterator() {
    let array = this._keyValueMap.slice(0);

    for (let element of array)
      yield element;
  }

});
exports.List = List;
