



"use strict";

module.metadata = {
  "stability": "experimental"
};










var has = exports.has = function has(array, element) {
  
  return !!~array.indexOf(element);
};
var hasAny = exports.hasAny = function hasAny(array, elements) {
  if (arguments.length < 2)
    return false;
  if (!Array.isArray(elements))
    elements = [ elements ];
  return array.some(function (element) {
      return has(elements, element);
  });
};










var add = exports.add = function add(array, element) {
  var result;
  if ((result = !has(array, element)))
    array.push(element);

  return result;
};











exports.remove = function remove(array, element) {
  var result;
  if ((result = has(array, element)))
    array.splice(array.indexOf(element), 1);

  return result;
};







exports.unique = function unique(array) {
  return array.reduce(function(values, element) {
    add(values, element);
    return values;
  }, []);
};

exports.flatten = function flatten(array){
   var flat = [];
   for (var i = 0, l = array.length; i < l; i++) {
    flat = flat.concat(Array.isArray(array[i]) ? flatten(array[i]) : array[i]);
   }
   return flat;
};

function fromIterator(iterator) {
  let array = [];
  if (iterator.__iterator__) {
    for each (let item in iterator)
      array.push(item);
  }
  else {
    for (let item of iterator)
      array.push(item);
  }
  return array;
}
exports.fromIterator = fromIterator;


function find(array, predicate) {
  var index = 0;
  var count = array.length;
  while (index < count) {
    var value = array[index];
    if (predicate(value)) return value;
    else index = index + 1;
  }
}
exports.find = find;
