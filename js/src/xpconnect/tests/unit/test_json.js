



































function run_test() {
  
  function toJSONString(a) {
    var res = JSONModule.toString(a);
    if (!JSONModule.isMostlyHarmless(res))
      throw new SyntaxError("Invalid JSON string: " + res);
    return res;
  }
  
  
  function isInvalidType(a) {
    try {
      JSONModule.toString(a);
      return false;
    } catch (ex) {
      return ex.name == "TypeError";
    }
  }
  
  function isInvalidSyntax(a) {
    try {
      JSONModule.fromString(a);
      return false;
    } catch (ex) {
      return ex.name == "SyntaxError";
    }
  }
  
  Components.utils.import("resource://gre/modules/JSON.jsm");
  do_check_eq(typeof(JSONModule), "object");
  
  
  do_check_eq(toJSONString(true), "true");
  do_check_eq(toJSONString(false), "false");
  
  do_check_eq(toJSONString(1), "1");
  do_check_eq(toJSONString(1.23), "1.23");
  do_check_eq(toJSONString(1.23e-45), "1.23e-45");
  
  do_check_true(isInvalidType(Infinity));
  do_check_true(isInvalidType(NaN));
  
  
  do_check_eq(toJSONString("Foo-Bar \b\t\n\f\r\"\\ \x01\u20ac"),
              '"Foo-Bar \\b\\t\\n\\f\\r\\"\\\\ \\u0001\\u20ac"');
  
  do_check_eq(toJSONString(null), "null");
  do_check_true(isInvalidType(undefined));
  
  do_check_eq(toJSONString([1, "2", 3.3]), '[1,"2",3.3]');
  
  do_check_eq(toJSONString({ 0: 0, 1: "1", 2: -2.2, length: 3 }), '[0,"1",-2.2]');
  
  var obj = { a: 1, b: "2", c: [-3e+30] };
  do_check_eq(toJSONString(obj), '{"a":1,"b":"2","c":[-3e+30]}');
  do_check_eq(JSONModule.toString(obj, ["b", "c"] ), '{"a":1}');
  
  do_check_true(isInvalidType(function() { }));
  
  
  do_check_eq(toJSONString(obj), JSONModule.toString(obj));
  
  do_check_eq(JSONModule.fromString("true"), true);
  do_check_eq(JSONModule.fromString("false"), false);
  do_check_eq(JSONModule.fromString("1"), 1);
  do_check_eq(JSONModule.fromString('"2.2"'), "2.2");
  do_check_eq(JSONModule.fromString("1.23e-45"), 1.23e-45);
  do_check_true(isInvalidSyntax("NaN"));
  
  do_check_eq(JSONModule.fromString('"Foo-Bar \\b\\t\\n\\f\\r\\"\\\\ \\u0001\\u20ac"'),
                              "Foo-Bar \b\t\n\f\r\"\\ \x01\u20ac");
  do_check_true(isInvalidSyntax('"multi\nline"'));
  do_check_eq(JSONModule.fromString("null"), null);
  do_check_true(isInvalidSyntax("."));
  
  var res = JSONModule.fromString('[1,"2",3.3]');
  do_check_eq(res.length, 3);
  do_check_eq(res[2], 3.3);
  
  do_check_false(res instanceof Array);
  
  res = JSONModule.fromString(toJSONString(obj));
  do_check_eq(res.a, obj.a);
  do_check_eq(res.b, obj.b);
  do_check_eq(res.c.length, obj.c.length);
  do_check_eq(res.c[0], obj.c[0]);
  
  
  do_check_true(JSONModule.isMostlyHarmless("a"));
  do_check_true(JSONModule.isMostlyHarmless("a[0]"));
  do_check_true(JSONModule.isMostlyHarmless('a["alert(\\"P0wn3d!\\");"]'));
  
  do_check_false(JSONModule.isMostlyHarmless('(function() { alert("P0wn3d!"); })()'));
  do_check_false(JSONModule.isMostlyHarmless('{ get a() { return "P0wn3d!"; } }'));
  
  
  let bigString = " ";
  while (bigString.length < (1 << 22))
    bigString += bigString;
  
  do_check_eq(JSONModule.fromString(toJSONString(bigString)), bigString);
}
