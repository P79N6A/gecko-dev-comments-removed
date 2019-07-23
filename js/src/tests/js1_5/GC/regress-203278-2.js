





































var gTestfile = 'regress-203278-2.js';

var BUGNUMBER = 203278;
var summary = 'Don\'t crash in recursive js_MarkGCThing';
var actual = 'FAIL';
var expect = 'PASS';

printBugNumber(BUGNUMBER);
printStatus (summary);




var a = new Array(1000 * 1000);

var i = a.length;
while (i-- != 0) {
  switch (i % 11) {
  case 0:
    a[i] = { };
    break;
  case 1:
    a[i] = { a: true, b: false, c: 0 };
    break;
  case 2:
    a[i] = { 0: true, 1: {}, 2: false };
    break;
  case 3:
    a[i] = { a: 1.2, b: "", c: [] };
    break;
  case 4:
    a[i] = [ false ];
    break;
  case 6:
    a[i] = [];
    break;
  case 7:
    a[i] = false;
    break;
  case 8:
    a[i] = "x";
    break;
  case 9:
    a[i] = new String("x");
    break;
  case 10:
    a[i] = 1.1;
    break;
  case 10:
    a[i] = new Boolean();
    break;
  }	   
}

printStatus("DSF is prepared");






for (i = 0; i != 50*1000; ++i) {
  a = [a, a, {}];
  a = [a,  {}, a];

}

printStatus("Linked list is prepared");

gc();

actual = 'PASS';

reportCompare(expect, actual, summary);

