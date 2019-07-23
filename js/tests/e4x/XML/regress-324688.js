





































gTestfile = 'regress-324688.js';

var summary = 'No Syntax Error when trailing space and XML.ignoreWhitespace ' +
    'true';
var BUGNUMBER = 324688;
var actual = 'No Error';
var expect = 'No Error';
START(summary);

function init()
{
    if (typeof Components != 'undefined')
    {
        try
        {
            netscape.security.PrivilegeManager.
                enablePrivilege('UniversalXPConnect');

            var TestObject = {
            observe: function () {
                    try
                    {
                        printBugNumber(BUGNUMBER);
                        printStatus (summary);
                        printStatus('Browser only: requires UniversalXPConnect');

                        printStatus("XML.ignoreWhitespace=" +
                                    XML.ignoreWhitespace);
                        var x = new XML("<a></a> ");
                    }
                    catch(ex2)
                    {
                        actual = ex2 + '';
                    }
                    print('expect = ' + expect);
                    print('actual = ' + actual);
                    TEST(1, expect, actual);
                    END();
                    gDelayTestDriverEnd = false;
                    jsTestDriverEnd();
                }
            };

            var t = Components.classes["@mozilla.org/timer;1"].
                createInstance(Components.interfaces.nsITimer);
            t.init(TestObject, 100, t.TYPE_ONE_SHOT);
        }
        catch(ex)
        {
            printStatus('Requires UniversalXPConnect');
        }
    }
}

if (typeof window != 'undefined')
{
    
    gDelayTestDriverEnd = true;

    window.addEventListener("load", init, false);
}
else
{
    TEST(1, expect, actual);
    END();
}

