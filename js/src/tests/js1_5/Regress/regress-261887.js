





































var gTestfile = 'regress-261887.js';


var BUGNUMBER = 261887;
var summary = 'deleted properties should not be visited by for in';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var count = 0;
var result = "";
var value = "";

var t = new Object();
t.one = "one";
t.two = "two";
t.three = "three";
t.four = "four";
   
for (var prop in t) {
  if (count==1) delete(t.three);
  count++;
  value = value + t[prop];
  result = result + prop;
}
 
expect = 'onetwofour:onetwofour';
actual = value + ':' + result;

reportCompare(expect, actual, summary);
