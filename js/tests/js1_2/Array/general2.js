





































gTestfile = 'general2.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'String:push,splice,concat,unshift,sort';

writeHeaderToLog('Executing script: general2.js');
writeHeaderToLog( SECTION + " "+ TITLE);

array1 = new Array();
array2 = [];
size   = 10;




for (var i = 0; i < size; i++)
{
  array1.push(i);
  array2.push(size - 1 - i);
}



for (i = array1.length; i > 0; i--)
{
  array3 = array1.slice(1,i);
  array1.splice(1,i-1);
  array1 = array3.concat(array1);
}



for (i = 0; i < size; i++)
{
  array1.push(array1.shift());
  array2.unshift(array2.pop());
}

new TestCase( SECTION, "Array.push,pop,shift,unshift,slice,splice", true,String(array1) == String(array2));
array1.sort();
array2.sort();
new TestCase( SECTION, "Array.sort", true,String(array1) == String(array2));

test();

