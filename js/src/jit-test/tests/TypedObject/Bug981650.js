


if (typeof TypedObject === "undefined")
  quit();

var T = TypedObject;
var v = new T.ArrayType(T.int32, 10);
new v(v);
