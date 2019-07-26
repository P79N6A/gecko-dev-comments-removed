


















function testcase() {
  var o = { foo : 42};

  try {
    throw o;
  }
  catch (e) {
    var foo;

    if (foo === undefined) {
      return true;
    }
  }
 }
runTestCase(testcase);
