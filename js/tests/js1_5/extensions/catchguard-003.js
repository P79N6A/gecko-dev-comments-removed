







































test();

function test()
{
    enterFunc ("test");

    var EXCEPTION_DATA = "String exception";
    var e = "foo", x = "foo";
    var caught = false;

    printStatus ("Catchguard 'Common Scope' test.");
    
    try 
    {    
        throw EXCEPTION_DATA;   
    }
    catch (e if ((x = 1) && false))
    {
        reportFailure ("Catch block (e if ((x = 1) && false) should not " +
                       "have executed.");
    }
    catch (e if (x == 1))
    {   
        caught = true;
    }
    catch (e)
    {   
        reportFailure ("Same scope should be used across all catchguards.");
    }

    if (!caught)
        reportFailure ("Exception was never caught.");
    
    if (e != "foo")
        reportFailure ("Exception data modified inside catch() scope should " +
                       "not be visible in the function scope (e ='" +
                       e + "'.)");

    if (x != 1)
        reportFailure ("Data modified in 'catchguard expression' should " +
                       "be visible in the function scope (x = '" +
                       x + "'.)");

    reportCompare('PASS', 'PASS', '');

    exitFunc ("test");
}
