





































gTestfile = 'regress-277683.js';



var summary = 'processing instruction with target name XML';
var BUGNUMBER = 277683;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = 'error';
try
{
    XML.ignoreProcessingInstructions = false;
    var xml = <root><child>Kibo</child><?xml version="1.0"?></root>;
    actual = 'no error';
}
catch(e)
{
    actual = 'error';
}

TEST(1, expect, actual);

END();
