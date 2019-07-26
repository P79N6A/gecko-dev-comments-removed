










function testcase() {
    if (this!==fnGlobalObject()) {
        return;
    }
    
    var fooCalled = false;
    function foo(){ fooCalled = true; } 
    
    try {
        this.bar( foo() );
        throw new Exception("this.bar does not exist!");
    } catch(e) {
        return (e instanceof TypeError) && (fooCalled===true);
    }
}
runTestCase(testcase);
