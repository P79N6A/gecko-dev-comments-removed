






































var bug = 277683;
var summary = 'processing instruction with target name XML';
var actual = '';
var expect = '';

START(summary);

printBugNumber (bug);
printStatus (summary);

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
