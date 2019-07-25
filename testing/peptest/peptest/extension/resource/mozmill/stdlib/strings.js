




































var EXPORTED_SYMBOLS = ['trim', 'vslice'];

var arrays = {}; Components.utils.import('resource://mozmill/stdlib/arrays.js', arrays);

var trim = function (str) {
  return (str.replace(/^[\s\xA0]+/, "").replace(/[\s\xA0]+$/, ""));
}

var vslice = function (str, svalue, evalue) {
  var sindex = arrays.indexOf(str, svalue);
  var eindex = arrays.rindexOf(str, evalue);
  return str.slice(sindex + 1, eindex);
}