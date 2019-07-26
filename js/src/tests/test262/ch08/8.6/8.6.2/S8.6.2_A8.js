








var x = Object.preventExtensions({});
var y = {};
try {
  x.__proto__ = y;
} catch (err) {
  
  
  
}
if (Object.getPrototypeOf(x) !== Object.prototype) {
  $ERROR("Prototype of non-extensible object mutated");
}

