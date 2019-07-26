










function testcase() {
  var o = {foo: 42};

  with (o) {
    var foo = "set in with";
  }

  return o.foo === "set in with";
 }
runTestCase(testcase);
