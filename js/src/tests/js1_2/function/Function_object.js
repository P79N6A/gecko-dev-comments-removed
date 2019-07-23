





































gTestfile = 'Function_object.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'functions: Function_object';

writeHeaderToLog('Executing script: Function_object.js');
writeHeaderToLog( SECTION + " "+ TITLE);


function a_test_function(a,b,c)
{
  return a + b + c;
}

f = a_test_function;


new TestCase( SECTION, "f.name",
	      'a_test_function',  f.name);

new TestCase( SECTION, "f.length",
	      3,  f.length);

new TestCase( SECTION, "f.arity",
	      3,  f.arity);

new TestCase( SECTION, "f(2,3,4)",
	      9,  f(2,3,4));

var fnName = (version() == 120) ? '' : 'anonymous';

new TestCase( SECTION, "(new Function()).name",
	      fnName, (new Function()).name);

new TestCase( SECTION, "(new Function()).toString()",
	      'function ' + fnName + '() {\n}',  (new Function()).toString());

test();

