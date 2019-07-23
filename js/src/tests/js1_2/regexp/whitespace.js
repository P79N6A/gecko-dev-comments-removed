





































gTestfile = 'whitespace.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: \\f\\n\\r\\t\\v\\s\\S ';

writeHeaderToLog('Executing script: whitespace.js');
writeHeaderToLog( SECTION + " "+ TITLE);


var non_whitespace = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~`!@#$%^&*()-+={[}]|\\:;'<,>./?1234567890" + '"';
var whitespace     = "\f\n\r\t\v ";


new TestCase ( SECTION,
	       "'" + whitespace + "'.match(new RegExp('\\s+'))",
	       String([whitespace]), String(whitespace.match(new RegExp('\\s+'))));


new TestCase ( SECTION,
	       "'" + non_whitespace + "'.match(new RegExp('\\S+'))",
	       String([non_whitespace]), String(non_whitespace.match(new RegExp('\\S+'))));


new TestCase ( SECTION,
	       "'" + non_whitespace + "'.match(new RegExp('\\s'))",
	       null, non_whitespace.match(new RegExp('\\s')));


new TestCase ( SECTION,
	       "'" + whitespace + "'.match(new RegExp('\\S'))",
	       null, whitespace.match(new RegExp('\\S')));

var s = non_whitespace + whitespace;


new TestCase ( SECTION,
	       "'" + s + "'.match(new RegExp('\\s+'))",
	       String([whitespace]), String(s.match(new RegExp('\\s+'))));

s = whitespace + non_whitespace;


new TestCase ( SECTION,
	       "'" + s + "'.match(new RegExp('\\S+'))",
	       String([non_whitespace]), String(s.match(new RegExp('\\S+'))));


new TestCase ( SECTION, "'1233345find me345'.match(new RegExp('[a-z\\s][a-z\\s]+'))",
	       String(["find me"]), String('1233345find me345'.match(new RegExp('[a-z\\s][a-z\\s]+'))));

var i;


for (i = 0; i < whitespace.length; ++i)
{
  s = 'ab' + whitespace[i] + 'cd';
  new TestCase ( SECTION,
		 "'" + s + "'.match(new RegExp('\\\\s'))",
		 String([whitespace[i]]), String(s.match(new RegExp('\\s'))));
  new TestCase ( SECTION,
		 "'" + s + "'.match(/\s/)",
		 String([whitespace[i]]), String(s.match(/\s/)));
}

for (i = 0; i < non_whitespace.length; ++i)
{
  s = '  ' + non_whitespace[i] + '  ';
  new TestCase ( SECTION,
		 "'" + s + "'.match(new RegExp('\\\\S'))",
		 String([non_whitespace[i]]), String(s.match(new RegExp('\\S'))));
  new TestCase ( SECTION,
		 "'" + s + "'.match(/\S/)",
		 String([non_whitespace[i]]), String(s.match(/\S/)));
}


test();
