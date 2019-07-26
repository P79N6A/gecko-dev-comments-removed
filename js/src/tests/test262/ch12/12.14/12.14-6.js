


















function testcase() {
  var o = {foo : function () { return 42;}};

  try {
    throw o;
  }
  catch (e) {
    var foo = function () {};
    if (foo() === undefined) {
      return true;
    }
  }
 }
runTestCase(testcase);
