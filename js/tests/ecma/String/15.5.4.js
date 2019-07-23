





































gTestfile = '15.5.4.js';










var SECTION = "15.5.4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the String Prototype objecta";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,
	      "String.prototype.getClass = Object.prototype.toString; String.prototype.getClass()",
	      "[object String]",
	      eval("String.prototype.getClass = Object.prototype.toString; String.prototype.getClass()") );

delete String.prototype.getClass;

new TestCase( SECTION,
              "typeof String.prototype",  
              "object",  
              typeof String.prototype );

new TestCase( SECTION,
              "String.prototype.valueOf()",
              "",       
              String.prototype.valueOf() );

new TestCase( SECTION,
              "String.prototype +''",      
              "",       
              String.prototype + '' );

new TestCase( SECTION,
              "String.prototype.length",   
              0,        
              String.prototype.length );

var prop;
var value;

value = '';
for (prop in "")
{
  value += prop;
}
new TestCase( SECTION,
              'String "" has no enumerable properties',
              '',
              value );

value = '';
for (prop in String.prototype)
{
  value += prop;
}
new TestCase( SECTION,
              'String.prototype has no enumerable properties',
              '',
              value );

test();
