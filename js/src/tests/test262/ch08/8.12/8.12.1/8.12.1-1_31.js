









function testcase() {

    var o = {};
    Object.defineProperty(o, "foo", {set: function() {;}, enumerable:true});
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
