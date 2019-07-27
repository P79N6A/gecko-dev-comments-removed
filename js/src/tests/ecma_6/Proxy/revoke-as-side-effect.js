function createProxy(proxyTarget) {
  var {proxy, revoke} = Proxy.revocable(proxyTarget, new Proxy({}, {
    get(target, propertyKey, receiver) {
      print("trap get:", propertyKey);
      revoke();
    }
  }));
  return proxy;
}

var obj;


assertEq(Object.getPrototypeOf(createProxy({})), Object.prototype);
assertEq(Object.getPrototypeOf(createProxy([])), Array.prototype);


obj = {};
Object.setPrototypeOf(createProxy(obj), Array.prototype);
assertEq(Object.getPrototypeOf(obj), Array.prototype);


assertEq(Object.isExtensible(createProxy({})), true);
assertEq(Object.isExtensible(createProxy(Object.preventExtensions({}))), false);


obj = {};
Object.preventExtensions(createProxy(obj));
assertEq(Object.isExtensible(obj), false);


assertEq(Object.getOwnPropertyDescriptor(createProxy({}), "a"), undefined);
assertEq(Object.getOwnPropertyDescriptor(createProxy({a: 5}), "a").value, 5);


obj = {};
Object.defineProperty(createProxy(obj), "a", {value: 5});
assertEq(obj.a, 5);


assertEq("a" in createProxy({}), false);
assertEq("a" in createProxy({a: 5}), true);


assertEq(createProxy({}).a, undefined);
assertEq(createProxy({a: 5}).a, 5);


assertThrowsInstanceOf(() => createProxy({}).a = 0, TypeError);
assertThrowsInstanceOf(() => createProxy({a: 5}).a = 0, TypeError);


assertEq(delete createProxy({}).a, true);
assertEq(delete createProxy(Object.defineProperty({}, "a", {configurable: false})).a, false);


for (var k in createProxy({})) {
    
    assertEq(true, false);
}
for (var k in createProxy({a: 5})) {
    
    assertEq(k, "a");
}


assertEq(Object.getOwnPropertyNames(createProxy({})).length, 0);
assertEq(Object.getOwnPropertyNames(createProxy({a: 5})).length, 1);


assertEq(createProxy(function() { return "ok" })(), "ok");



assertEq(new (createProxy(function(){ return obj; })), obj);

if (typeof reportCompare === "function")
  reportCompare(true, true);
