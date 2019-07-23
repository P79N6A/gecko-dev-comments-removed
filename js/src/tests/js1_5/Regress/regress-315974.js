




































var gTestfile = 'regress-315974.js';









var BUGNUMBER = 315974;
var summary = 'Test internal printf() for wide characters';

printBugNumber(BUGNUMBER);
printStatus (summary);

enterFunc (String (BUGNUMBER));

var goodSurrogatePair = '\uD841\uDC42';
var badSurrogatePair = '\uD841badbad';

var goodSurrogatePairQuotedUtf8 = '"\uD841\uDC42"';
var badSurrogatePairQuotedUtf8 = 'no error thrown!';
var goodSurrogatePairQuotedNoUtf8 = '"\\uD841\\uDC42"';
var badSurrogatePairQuotedNoUtf8 = '"\\uD841badbad"';

var status = summary + ': String.quote() should pay respect to surrogate pairs';
var actual = goodSurrogatePair.quote();

var expect = goodSurrogatePairQuotedUtf8;

if (actual != expect && actual == goodSurrogatePairQuotedNoUtf8)
  expect = actual;
reportCompare(expect, actual, status);





status = summary + ': String.quote() should throw error on bad surrogate pair';

actual = badSurrogatePair.quote();

reportCompare(badSurrogatePairQuotedNoUtf8, actual, status);

exitFunc (String (BUGNUMBER));
