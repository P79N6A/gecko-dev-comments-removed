









function testcase() {

    var o = {};
    Object.defineProperty(o, "foo", {value: 42});
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
