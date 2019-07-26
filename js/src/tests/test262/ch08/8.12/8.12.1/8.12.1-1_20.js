









function testcase() {

    var o = { get foo() { return 42;} };
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
