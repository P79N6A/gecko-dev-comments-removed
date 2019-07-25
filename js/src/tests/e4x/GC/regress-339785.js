







var summary = "scanner: memory exposure to scripts";
var BUGNUMBER = 339785;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

function evalXML(N)
{
    var str = Array(N + 1).join('a'); 
    src = "var x = <xml>&"+str+";</xml>;";
    try {
        eval(src);
        return "Should have thrown unknown entity error";
    } catch (e) {
        return e.message;
    }
    return "Unexpected";
}

var N1 = 1;
var must_be_good = evalXML(N1);
expect = 'unknown XML entity a';
actual = must_be_good;
TEST(1, expect, actual);

function testScanner()
{
    for (var power = 2; power != 15; ++power) {
        var N2 = (1 << power) - 2;
        var can_be_bad  = evalXML(N2);
        var diff = can_be_bad.length - must_be_good.length;
        if (diff != 0 && diff != N2 - N1) {
            return "Detected memory exposure at entity length of "+(N2+2);
        }
    }
    return "Ok";
}

expect = "Ok";



for (var iTestScanner = 0; iTestScanner < 100; ++iTestScanner)
{
    actual = testScanner();
    TEST(iTestScanner+1, expect, actual);
}


END();
