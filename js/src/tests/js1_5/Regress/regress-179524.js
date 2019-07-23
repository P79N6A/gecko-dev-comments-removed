




















































var gTestfile = 'regress-179524.js';
var UBound = 0;
var BUGNUMBER = 179524;
var summary = "Don't crash on extraneous arguments to str.match(), etc.";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


str = 'ABC abc';
var re = /z/ig;

status = inSection(1);
actual = str.match(re);
expect = null;
addThis();

status = inSection(2);
actual = str.match(re, 'i');
expect = null;
addThis();

status = inSection(3);
actual = str.match(re, 'g', '');
expect = null;
addThis();

status = inSection(4);
actual = str.match(re, 'z', new Object(), new Date());
expect = null;
addThis();





status = inSection(5);
actual = str.search(re);
expect = -1;
addThis();

status = inSection(6);
actual = str.search(re, 'i');
expect = -1;
addThis();

status = inSection(7);
actual = str.search(re, 'g', '');
expect = -1;
addThis();

status = inSection(8);
actual = str.search(re, 'z', new Object(), new Date());
expect = -1;
addThis();





status = inSection(9);
actual = str.replace(re, 'Z');
expect = str;
addThis();

status = inSection(10);
actual = str.replace(re, 'Z', 'i');
expect = str;
addThis();

status = inSection(11);
actual = str.replace(re, 'Z', 'g', '');
expect = str;
addThis();

status = inSection(12);
actual = str.replace(re, 'Z', 'z', new Object(), new Date());
expect = str;
addThis();











status = inSection(13);
actual = str.match('a').toString();
expect = str.match(/a/).toString();
addThis();

status = inSection(14);
actual = str.match('a', 'i').toString();
expect = str.match(/a/i).toString();
addThis();

status = inSection(15);
actual = str.match('a', 'ig').toString();
expect = str.match(/a/ig).toString();
addThis();

status = inSection(16);
actual = str.match('\\s', 'm').toString();
expect = str.match(/\s/m).toString();
addThis();





status = inSection(17);
actual = str.match('a', 'i', 'g').toString();
expect = str.match(/a/i).toString();
addThis();

status = inSection(18);
actual = str.match('a', 'ig', new Object()).toString();
expect = str.match(/a/ig).toString();
addThis();

status = inSection(19);
actual = str.match('\\s', 'm', 999).toString();
expect = str.match(/\s/m).toString();
addThis();





status = inSection(20);
try
{
  actual = str.match('a', 'z').toString();
  expect = 'SHOULD HAVE FALLEN INTO CATCH-BLOCK!';
  addThis();
}
catch (e)
{
  actual = e instanceof SyntaxError;
  expect = true;
  addThis();
}









status = inSection(21);
actual = str.search('a');
expect = str.search(/a/);
addThis();

status = inSection(22);
actual = str.search('a', 'i');
expect = str.search(/a/i);
addThis();

status = inSection(23);
actual = str.search('a', 'ig');
expect = str.search(/a/ig);
addThis();

status = inSection(24);
actual = str.search('\\s', 'm');
expect = str.search(/\s/m);
addThis();





status = inSection(25);
actual = str.search('a', 'i', 'g');
expect = str.search(/a/i);
addThis();

status = inSection(26);
actual = str.search('a', 'ig', new Object());
expect = str.search(/a/ig);
addThis();

status = inSection(27);
actual = str.search('\\s', 'm', 999);
expect = str.search(/\s/m);
addThis();





status = inSection(28);
try
{
  actual = str.search('a', 'z');
  expect = 'SHOULD HAVE FALLEN INTO CATCH-BLOCK!';
  addThis();
}
catch (e)
{
  actual = e instanceof SyntaxError;
  expect = true;
  addThis();
}











status = inSection(29);
actual = str.replace('a', 'Z');
expect = str.replace(/a/, 'Z');
addThis();

status = inSection(30);
actual = str.replace('a', 'Z', 'i');
expect = str.replace(/a/i, 'Z');
addThis();

status = inSection(31);
actual = str.replace('a', 'Z', 'ig');
expect = str.replace(/a/ig, 'Z');
addThis();

status = inSection(32);
actual = str.replace('\\s', 'Z', 'm'); 
actual = str.replace(' ', 'Z', 'm');   
expect = str.replace(/\s/m, 'Z');
addThis();





status = inSection(33);
actual = str.replace('a', 'Z', 'i', 'g');
expect = str.replace(/a/i, 'Z');
addThis();

status = inSection(34);
actual = str.replace('a', 'Z', 'ig', new Object());
expect = str.replace(/a/ig, 'Z');
addThis();

status = inSection(35);
actual = str.replace('\\s', 'Z', 'm', 999); 
actual = str.replace(' ', 'Z', 'm', 999);   
expect = str.replace(/\s/m, 'Z');
addThis();





status = inSection(36);
try
{
  actual = str.replace('a', 'Z', 'z');
  expect = 'SHOULD HAVE FALLEN INTO CATCH-BLOCK!';
  addThis();
}
catch (e)
{
  actual = e instanceof SyntaxError;
  expect = true;
  addThis();
}





test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
