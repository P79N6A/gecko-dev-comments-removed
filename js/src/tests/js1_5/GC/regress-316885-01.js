




































var gTestfile = 'regress-316885-01.js';

var BUGNUMBER = 316885;
var summary = 'Unrooted access in jsinterp.c';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var str_with_num = "0.1";

var obj = {
  elem getter: function() {
    return str_with_num;
  },
  elem setter: function(value) {
    gc();
  }

};

expect = Number(str_with_num);
actual = obj.elem++;

gc();

 
reportCompare(expect, actual, summary);
