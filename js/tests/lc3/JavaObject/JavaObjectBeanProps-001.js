






































var SECTION = "JavaObject Field or method access";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
              SECTION;
startTest();

var dt = new DT();

var a = [
  "boolean",
  "byte",
  "integer", 
  "double",
  "float",
  "short",
  "char"
  ];

var v = [
  true,
  1,
  2,
  3.0,
  4.0,
  5,
  'a'.charCodeAt(0)
  ];

for (var i=0; i < a.length; i++) {
  var name = a[i];
  var getterName = "get" + a[i].charAt(0).toUpperCase() + 
    a[i].substring(1);
  var setterName = "set" + a[i].charAt(0).toUpperCase() + 
    a[i].substring(1);
  new TestCase(
    "dt['" + name + "'] == dt." + getterName + "()",
    dt[name],
    dt[getterName]() );

  dt[name] = v[i];
  new TestCase(
    "dt['" + name + "'] = "+ v[i] +"; dt." + getterName + "() == " + v[i],
    dt[getterName](),
    v[i]);
}

test();

