









function testcase() {

    var base = {};
    Object.defineProperty(base, "foo", {get: function() {return 42;}, enumerable:true});
    var o = Object.create(base);
    return o.hasOwnProperty("foo")===false;

}
runTestCase(testcase);
