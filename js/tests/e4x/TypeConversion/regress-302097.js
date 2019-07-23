





































START("section - description");

var bug = 302097;
var summary = 'E4X - Function.prototype.toString should not quote {} attribute values';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

function f(k) {
  return <xml k={k}/>;
}

actual = f.toString().replace(/</g, '&lt;');
expect = 'function f(k) {\n    return &lt;xml k={k}/>;\n}';

TEST(1, expect, actual);

END();
