









function testcase() {

    var o = {};
    Object.defineProperty(o, "foo", {set: function() {;}});
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
