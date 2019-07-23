




































var gTestfile = 'regress-309242.js';

var BUGNUMBER = 309242;
var summary = 'E4X should be on by default while preserving comment hack';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);







expect = true;
actual = true;

<!-- comment -->; actual = false;

reportCompare(expect, actual, summary + ': &lt;!-- is comment to end of line');

expect = true;
actual = false;

<!--
 actual = true;


reportCompare(expect, actual, summary + ': comment hack works inside script');



var x = <foo/>;

expect = 'element';
actual = x.nodeKind();

reportCompare(expect, actual, summary + ': E4X is available');
