


















function testcase() {
  var o = {foo: 42};

  try {
    throw o;
  }
  catch (e) {
    var foo = 1;
  }

  if (o.foo === 42) {
    return true;
  }
 }
runTestCase(testcase);
