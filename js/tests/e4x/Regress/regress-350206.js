





































gTestfile = 'regress-350206.js';

var BUGNUMBER = 350206;
var summary = 'Do not assert: serial <= n in jsxml.c';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var pa = <state1 xmlns="http://aaa" id="D1"><tag1/></state1>
var ch = <state2 id="D2"><tag2/></state2>
pa.appendChild(ch);
pa.@msg = "Before assertion failure";
pa.toXMLString();

TEST(1, expect, actual);

END();
