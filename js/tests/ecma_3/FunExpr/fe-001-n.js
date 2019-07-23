







































DESCRIPTION = "Previous statement should have thrown a ReferenceError";
EXPECTED = "error";

test();

function test()
{
    enterFunc ("test"); 
    printStatus ("Function Expression test.");

    var x = function f(){return "inner";}();
    var y = f();    
    reportFailure ("Previous statement should have thrown a ReferenceError");

    exitFunc ("test");
}
