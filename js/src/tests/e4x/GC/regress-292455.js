




































gTestfile = 'regress-292455.js';

var summary = "Regress - Do not crash on gc";
var BUGNUMBER = 292455;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

function output (text)
{
    if (typeof alert != 'undefined')
    {
        alert(text);
    }
    else if (typeof print != 'undefined')
    {
        print(text);
    }
}

function doTest ()
{
    var html = <div xml:lang="en">
        <h1>Kibology for all</h1>
        <p>Kibology for all. All for Kibology. </p>
        </div>;
    
    html.* += <h1>All for Kibology</h1>;
    gc();
    output(html);
    html.* += <p>All for Kibology. Kibology for all.</p>;
    gc();
    output(html);
}

doTest();

TEST(1, expect, actual);

END();
