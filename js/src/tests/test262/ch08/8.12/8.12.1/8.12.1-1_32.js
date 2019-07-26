









function testcase() {

    var o = {};
    Object.defineProperty(o, "foo", {set: function() {;}, configurable:true});
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
