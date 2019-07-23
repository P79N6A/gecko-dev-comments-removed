





































gTestfile = '12.3-01.js';

var summary = '12.3 - for-each-in should not affect for-in';
var BUGNUMBER = 292020;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);


function foreachbug()
{
    var arryOuter = ["outervalue0", "outervalue1"];
    var arryInner = ["innervalue1","innervalue2"];

    for (var j in arryOuter)
    {
        var result = (j in arryOuter);
        if (!result)
        {
            return ("enumerated property not in object: (" +
                    j + " in  arryOuter) " + result);
            return result;
        }


        for each (k in arryInner)
        {
            
        }
    }
    return '';
}

TEST(1, '', foreachbug());

END();
