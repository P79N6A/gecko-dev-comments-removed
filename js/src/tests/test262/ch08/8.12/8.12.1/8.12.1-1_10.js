









function testcase() {

    var o = {};
    Object.defineProperty(o, "foo", {value: 42, writable:true, configurable:true});
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
