









function testcase() {   
    function foo() {};
    Object.defineProperty(foo.prototype, "bar", {value: "unwritable"}); 
    
    var o = new foo(); 
    o.bar = "overridden"; 
    return o.hasOwnProperty("bar")===false && o.bar==="unwritable";
}
runTestCase(testcase);
