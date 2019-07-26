



var handler = { "\u0039" : function() {} };
var g = newGlobal('new-compartment');
if (typeof findReferences == 'function') {
  findReferences(g);
}
