




































START("13.4.4.3 - XML.appendChild should copy child");

var bug = 312692;
var summary = '13.4.4.3 - XML.appendChild should copy child';
var actual = '';
var expect = 'error';

printBugNumber (bug);
printStatus (summary);

try
{
    var node = <node/>;
    node.appendChild(node);

    var result = String(node);
    actual = 'no error';
}
catch(e)
{
    actual = 'error';
}
TEST(1, expect, actual);

END();
