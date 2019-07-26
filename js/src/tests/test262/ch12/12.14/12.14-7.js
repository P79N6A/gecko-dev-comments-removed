


















function testcase() {
      var o = {foo: 1};
      var catchAccessed = false;
      
      try {
        throw o;
      }
      catch (expObj) {
        catchAccessed = (expObj.foo == 1);
      }

      try {
        expObj;
      }
      catch (e) {
        return catchAccessed && e instanceof ReferenceError
      }
      return false;
    }
runTestCase(testcase);
