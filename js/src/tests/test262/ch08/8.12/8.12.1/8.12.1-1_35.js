









function testcase() {

    var o = {};
    Object.defineProperty(o, "foo", {get: function() {return 42;}, set: function() {;}, enumerable:true});
    return o.hasOwnProperty("foo");

}
runTestCase(testcase);
