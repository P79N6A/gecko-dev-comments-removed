










function testcase() {
    var o = { }; 
    Object.defineProperty(o, "bar", {get: function()  {this.barGetter = true; return 42;}, 
                                     set: function(x) {this.barSetter = true; }});
    try {
        o.foo( o["bar"] );
        throw new Exception("o.foo does not exist!");
    } catch(e) {
        return (e instanceof TypeError) && (o.barGetter===true) && (o.barSetter===undefined);
    }
}
runTestCase(testcase);
