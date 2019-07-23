





































START("12.3 - The for-each-in Statement");

var bug = 292020;
var summary = 'for-each-in should not affect for-in';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);


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
