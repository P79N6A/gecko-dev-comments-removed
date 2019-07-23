



































START("Function.prototype.toString should not quote XML literals");

var bug = 301692;
var summary = 'Function.prototype.toString should not quote XML literals';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

actual = ToString((function () { return <xml/>; }));
expect = 'function () { return <xml/>;}';
TEST(1, expect, actual);

actual = ToString((function () { return <xml></xml>; }));
expect = 'function () { return <xml></xml>;}';
TEST(2, expect, actual);

actual = ToString((function () { return <xml><morexml/></xml>; }));
expect = 'function () { return <xml><morexml/></xml>;}';
TEST(3, expect, actual);

actual = ToString((function (k) { return <xml>{k}</xml>; }));
expect = 'function (k) { return <xml>{k}</xml>;}';
TEST(4, expect, actual);

actual = ToString((function (k) { return <{k}/>; }));
expect = 'function (k) { return <{k}/>;}';
TEST(5, expect, actual);

actual = ToString((function (k) { return <{k}>{k}</{k}>; }));
expect = 'function (k) { return <{k}>{k}</{k}>;}';
TEST(6, expect, actual);

actual = ToString((function (k) { return <{k} 
                  {k}={k} {"k"}={k + "world"}><{k + "k"}/></{k}>; }));
expect = 'function (k) ' + 
         '{ return <{k} {k}={k} {"k"}={k + "world"}><{k + "k"}/></{k}>;}';
TEST(7, expect, actual);

END();

function ToString(f)
{
  return f.toString().replace(/\n/g, '').replace(/[ ]+/g, ' ');
}
