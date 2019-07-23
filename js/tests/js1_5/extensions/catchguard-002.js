







































test();

function test()
{
    enterFunc ("test");

    var EXCEPTION_DATA = "String exception";
    var e;
    var caught = false;

    printStatus ("Basic catchguard test.");
    
    try 
    {    
        throw EXCEPTION_DATA;   
    }
    catch (e if true)
    {
        caught = true;
    }
    catch (e if true)
    {   
        reportFailure ("Second (e if true) catch block should not have " +
                       "executed.");
    }
    catch (e)
    {   
        reportFailure ("Catch block (e) should not have executed.");
    }

    if (!caught)
        reportFailure ("Exception was never caught.");
    
    reportCompare('PASS', 'PASS', '');

    exitFunc ("test");
}
