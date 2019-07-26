










function testcase() {
  var o = {foo: 1};

  with (o) {
    foo = 42;
  }

  try {
    foo;
  }
  catch (e) {
     return true;
  }
 }
runTestCase(testcase);
