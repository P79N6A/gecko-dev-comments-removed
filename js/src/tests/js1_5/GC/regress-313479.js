




































var gTestfile = 'regress-313479.js';

var BUGNUMBER = 313479;
var summary = 'Root access in jsnum.c';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var prepared_string = String(1);
String(2); 

var likeString = {
  toString: function() {
    var tmp = prepared_string;
    prepared_string = null;
    return tmp;
  }
};

var likeNumber = {
  valueOf: function() {
    gc();
    return 10;
  }
}

  var expect = 1;
var actual = parseInt(likeString, likeNumber);
printStatus("expect="+expect+" actual="+actual);
 
reportCompare(expect, actual, summary);
