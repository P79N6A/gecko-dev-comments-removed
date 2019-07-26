









function testcase() {

    var o = { get foo() { return 42;}, set foo(x) {;} };
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
