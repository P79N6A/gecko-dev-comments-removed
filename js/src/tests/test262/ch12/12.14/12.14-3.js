


















function testcase() {
  try {
    throw new Error();
  }
  catch (e) {
    var foo = "declaration in catch";
  }
  
  return foo === "declaration in catch";
 }
runTestCase(testcase);
