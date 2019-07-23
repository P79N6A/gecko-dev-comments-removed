




































var bug = 311157;
var summary = 'Comment-hiding compromise left E4X parsing/scanning inconsistent';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

try
{
    eval('var x = <hi> <!-- duh -->\n     there </hi>');
}
catch(e)
{
}

reportCompare(expect, actual, summary);

