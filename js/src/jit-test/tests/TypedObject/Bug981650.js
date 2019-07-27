


if (typeof TypedObject === "undefined")
  quit();

var T = TypedObject;
var AT = new T.ArrayType(T.int32);
var v = new AT(10);
new AT(v);
