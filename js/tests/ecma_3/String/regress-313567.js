




































var bug = 313567;
var summary = 'String.prototype.length should not be generic';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var s = new String("1");
s.toString = function() {
    return "22";
}
var expect = 1;
var actual = s.length;
printStatus("expect="+expect+" actual="+actual);
  
reportCompare(expect, actual, summary);
