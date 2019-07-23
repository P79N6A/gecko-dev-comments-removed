







































DESCRIPTION = "var in catch clause should have caused an error.";
EXPECTED = "error";

test();

function test()
{
    enterFunc ("test");

    var EXCEPTION_DATA = "String exception";
    var e;

    printStatus ("Catchguard var declaration negative test.");
    
    try 
    {    
        throw EXCEPTION_DATA;   
    }
    catch (var e)
    {   

    }

    reportFailure ("var in catch clause should have caused an error.");

    exitFunc ("test");
}
