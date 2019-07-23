




































var gTestfile = 'regress-432075.js';

var BUGNUMBER = 432075;
var summary = 'A function decompiles as [object Function] after export/import';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var a = Function;
var t = this;
export *;
import t.*;
Function = a;

expect = 'function anonymous() {}';
actual = (new Function("")) + '';

compareSource(expect, actual, summary);
