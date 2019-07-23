







































DESCRIPTION = "Illegally constructed catchguard should have thrown an exception.";
EXPECTED = "error";

test();

function test()
{
    enterFunc ("test");

    var EXCEPTION_DATA = "String exception";
    var e;

    printStatus ("Catchguard syntax negative test #2.");
    
    try 
    {    
        throw EXCEPTION_DATA;   
    }
    catch (e) 
    {   

    }
    catch (e) 
    {

    }

    reportFailure ("Illegally constructed catchguard should have thrown " +
                   "an exception.");

    exitFunc ("test");
}
