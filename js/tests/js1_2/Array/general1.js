





































gTestfile = 'general1.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'String:push,unshift,shift';

writeHeaderToLog('Executing script: general1.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var array1 = [];

array1.push(123);            
array1.push("dog");          
array1.push(-99);            
array1.push("cat");          
new TestCase( SECTION, "array1.pop()", array1.pop(),'cat');

array1.push("mouse");        
new TestCase( SECTION, "array1.shift()", array1.shift(),123);

array1.unshift(96);          
new TestCase( SECTION, "state of array", String([96,"dog",-99,"mouse"]), String(array1));
new TestCase( SECTION, "array1.length", array1.length,4);
array1.shift();              
array1.shift();              
array1.shift();              
new TestCase( SECTION, "array1.shift()", array1.shift(),"mouse");
new TestCase( SECTION, "array1.shift()", "undefined", String(array1.shift()));

test();

