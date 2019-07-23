





































gTestfile = 'Number.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
var TITLE = 'functions: Number';
var BUGNUMBER="123435";

startTest();
writeHeaderToLog('Executing script: Number.js');
writeHeaderToLog( SECTION + " "+ TITLE);

date = new Date(2200);

new TestCase( SECTION, "Number(new Date(2200))  ",
	      2200,  (Number(date)));
new TestCase( SECTION, "Number(true)            ",
	      1,  (Number(true)));
new TestCase( SECTION, "Number(false)           ",
	      0,  (Number(false)));
new TestCase( SECTION, "Number('124')           ",
	      124,  (Number('124')));
new TestCase( SECTION, "Number('1.23')          ",
	      1.23,  (Number('1.23')));
new TestCase( SECTION, "Number({p:1})           ",
	      NaN,  (Number({p:1})));
new TestCase( SECTION, "Number(null)            ",
	      0,  (Number(null)));
new TestCase( SECTION, "Number(-45)             ",
	      -45,  (Number(-45)));




new TestCase( SECTION, "Number([1,2,3])         ",
	      3,  (Number([1,2,3])));


test();

