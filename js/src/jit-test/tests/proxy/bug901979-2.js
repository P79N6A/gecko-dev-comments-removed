


var global = this;
var status = "pass";



var metaHandler = {
  get: _ => { status = "SMASH"; },
  has: _ => { status = "SMASH"; },
  invoke: _ => { status = "SMASH"; }
};
var handler = new Proxy({}, metaHandler);



var angryProxy = new Proxy(Object.create(null), handler);
this.__proto__ = angryProxy;
Object.prototype.__proto__ = angryProxy;


this.nonExistingProperty;
assertEq(status, "SMASH");


status = "pass";
Map;
ArrayBuffer;
Date;
assertEq(status, "pass");
