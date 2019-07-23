





































var gTestfile = 'regress-83293.js';

































var BUGNUMBER = 103351; 
var summ_OLD = 'Testing str.replace(strA, strB) == str.replace(new RegExp(strA),strB)';
var summ_NEW = 'Testing String.prototype.replace(x,y) when x is a string';
var summary = summ_NEW;
var status = '';
var actual = '';
var expect= '';
var cnEmptyString = '';
var str = 'abc';
var strA = cnEmptyString;
var strB = 'Z';



test();











function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);


































































  status = 'Section 1 of test';
  actual = 'abc'.replace('a', 'Z');
  expect = 'Zbc';
  reportCompare(expect, actual, status);

  status = 'Section 2 of test';
  actual = 'abc'.replace('b', 'Z');
  expect = 'aZc';
  reportCompare(expect, actual, status);

  status = 'Section 3 of test';
  actual = 'abc'.replace(undefined, 'Z');
  expect = 'abc'; 
  reportCompare(expect, actual, status);

  status = 'Section 4 of test';
  actual = 'abc'.replace(null, 'Z');
  expect = 'abc'; 
  reportCompare(expect, actual, status);

  status = 'Section 5 of test';
  actual = 'abc'.replace(true, 'Z');
  expect = 'abc'; 
  reportCompare(expect, actual, status);

  status = 'Section 6 of test';
  actual = 'abc'.replace(false, 'Z');
  expect = 'abc'; 
  reportCompare(expect, actual, status);

  status = 'Section 7 of test';
  actual = 'aa$aa'.replace('$', 'Z');
  expect = 'aaZaa'; 
  reportCompare(expect, actual, status);

  status = 'Section 8 of test';
  actual = 'abc'.replace('.*', 'Z');
  expect = 'abc';  
  reportCompare(expect, actual, status);

  status = 'Section 9 of test';
  actual = 'abc'.replace('', 'Z');
  expect = 'Zabc';  
  reportCompare(expect, actual, status);

  exitFunc ('test');
}
