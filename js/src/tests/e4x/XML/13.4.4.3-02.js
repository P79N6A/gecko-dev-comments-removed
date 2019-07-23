




































gTestfile = '13.4.4.3-02.js';

var summary = "13.4.4.3 - XML.appendChild should copy child";
var BUGNUMBER = 312692;
var actual = '';
var expect = 'error';

printBugNumber(BUGNUMBER);
START(summary);

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
