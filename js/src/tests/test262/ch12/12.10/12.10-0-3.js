










function testcase() {
  var o = {prop: "12.10-0-3 before"};
  var f;

  with (o) {
    f = function () { return prop; }
  }
  o.prop = "12.10-0-3 after";
  return f()==="12.10-0-3 after"
 }
runTestCase(testcase);
