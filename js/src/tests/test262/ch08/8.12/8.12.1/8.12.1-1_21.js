









function testcase() {

    var o = { set foo(x) {;} };
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
