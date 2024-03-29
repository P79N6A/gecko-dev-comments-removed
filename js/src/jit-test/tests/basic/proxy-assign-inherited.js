



var hits;
var handlers = {
  getOwnPropertyDescriptor: function(name) {
    return undefined;
  },
  getPropertyDescriptor: function(name) {
    return { value:42, writable:true, enumerable:true, configurable:true };
  },
  defineProperty: function(name, descriptor) {
    hits++;
    assertEq(name, 'x');
    assertEq(descriptor.value, 43);
  }
};
hits = 0;
Proxy.create(handlers).x = 43;
assertEq(hits, 1);