














































var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: simple form';

writeHeaderToLog('Executing script: simple_form.js');
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase ( SECTION,
	       "/[0-9]{3}/.exec('23 2 34 678 9 09')",
	       String(["678"]), String(/[0-9]{3}/.exec('23 2 34 678 9 09')));

new TestCase ( SECTION,
	       "/3.{4}8/.exec('23 2 34 678 9 09')",
	       String(["34 678"]), String(/3.{4}8/.exec('23 2 34 678 9 09')));

new TestCase ( SECTION,
	       "(/3.{4}8/.exec('23 2 34 678 9 09').length",
	       1, (/3.{4}8/.exec('23 2 34 678 9 09')).length);

var re = /[0-9]{3}/;
new TestCase ( SECTION,
	       "re.exec('23 2 34 678 9 09')",
	       String(["678"]), String(re.exec('23 2 34 678 9 09')));

re = /3.{4}8/;
new TestCase ( SECTION,
	       "re.exec('23 2 34 678 9 09')",
	       String(["34 678"]), String(re.exec('23 2 34 678 9 09')));

new TestCase ( SECTION,
	       "/3.{4}8/.exec('23 2 34 678 9 09')",
	       String(["34 678"]), String(/3.{4}8/.exec('23 2 34 678 9 09')));

re =/3.{4}8/;
new TestCase ( SECTION,
	       "(re.exec('23 2 34 678 9 09').length",
	       1, (re.exec('23 2 34 678 9 09')).length);

new TestCase ( SECTION,
	       "(/3.{4}8/.exec('23 2 34 678 9 09').length",
	       1, (/3.{4}8/.exec('23 2 34 678 9 09')).length);

test();
