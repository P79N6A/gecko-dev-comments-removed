









function testcase() {

    var o = {};
    Object.defineProperty(o, "foo", {value: 42, configurable:true});
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
