



















































var bug = '(none)';
var summary = '10.1.4.1 Entering An Execution Context';
var actual = '';
var expect = '';

test();

function test()
{
    enterFunc ("test");
    printBugNumber (bug);
    printStatus (summary);

    var y;
    eval("var x = 1");

    if (delete y)
        reportFailure ("Expected *NOT* to be able to delete y");

    if (typeof x == "undefined")
        reportFailure ("x did not remain defined after eval()");
    else if (x != 1)
        reportFailure ("x did not retain it's value after eval()");
    
    if (!delete x)
        reportFailure ("Expected to be able to delete x");

    reportCompare('PASS', 'PASS', '10.1.4.1 Entering An Execution Context');

    exitFunc("test");        
}
