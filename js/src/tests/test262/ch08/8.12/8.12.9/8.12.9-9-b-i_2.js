










function testcase() {
    var o = {};
    Object.defineProperty(o, "foo", 
                          { value: "hello", 
                            configurable: true,
                            enumerable: true,
                            writable: true});
    Object.preventExtensions(o);
    Object.defineProperty(o, "foo", { get: function() { return 5;} });

    var fooDescrip = Object.getOwnPropertyDescriptor(o, "foo");
    return o.foo===5 && fooDescrip.get!==undefined && fooDescrip.set===undefined && fooDescrip.value===undefined && fooDescrip.configurable===true && fooDescrip.enumerable===true && fooDescrip.writable===undefined;
}
runTestCase(testcase);
