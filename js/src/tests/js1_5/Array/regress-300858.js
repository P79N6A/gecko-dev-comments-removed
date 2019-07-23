




































var gTestfile = 'regress-300858.js';

var BUGNUMBER = 300858;
var summary = 'Do not crash when sorting array with holes';
var actual = 'No Crash';
var expect = 'No Crash';

var arry     = [];
arry[6]  = 'six';
arry[8]  = 'eight';
arry[9]  = 'nine';
arry[13] = 'thirteen';
arry[14] = 'fourteen';
arry[21] = 'twentyone';
arry.sort();

reportCompare(expect, actual, summary);
