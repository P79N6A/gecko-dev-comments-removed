





































gTestfile = 'word_boundary.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: \\b and \\B';

writeHeaderToLog('Executing script: word_boundary.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'cowboy boyish boy'.match(new RegExp('\\bboy\\b'))",
	       String(["boy"]), String('cowboy boyish boy'.match(new RegExp('\\bboy\\b'))));

var boundary_characters = "\f\n\r\t\v~`!@#$%^&*()-+={[}]|\\:;'<,>./? " + '"';
var non_boundary_characters = '1234567890_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
var s     = '';
var i;


for (i = 0; i < boundary_characters.length; ++i)
{
  s = '123ab' + boundary_characters.charAt(i) + '123c' + boundary_characters.charAt(i);

  new TestCase ( SECTION,
		 "'" + s + "'.match(new RegExp('\\b123[a-z]\\b'))",
		 String(["123c"]), String(s.match(new RegExp('\\b123[a-z]\\b'))));
}


for (i = 0; i < non_boundary_characters.length; ++i)
{
  s = '123ab' + non_boundary_characters.charAt(i) + '123c' + non_boundary_characters.charAt(i);

  new TestCase ( SECTION,
		 "'" + s + "'.match(new RegExp('\\B123[a-z]\\B'))",
		 String(["123c"]), String(s.match(new RegExp('\\B123[a-z]\\B'))));
}

s = '';


for (i = 0; i < boundary_characters.length; ++i)
{
  s += boundary_characters[i] + "a" + i + "b";
}
s += "xa1111bx";

new TestCase ( SECTION,
	       "'" + s + "'.match(new RegExp('\\Ba\\d+b\\B'))",
	       String(["a1111b"]), String(s.match(new RegExp('\\Ba\\d+b\\B'))));

new TestCase ( SECTION,
	       "'" + s + "'.match(/\\Ba\\d+b\\B/)",
	       String(["a1111b"]), String(s.match(/\Ba\d+b\B/)));

s = '';


for (i = 0; i < non_boundary_characters.length; ++i)
{
  s += non_boundary_characters[i] + "a" + i + "b";
}
s += "(a1111b)";

new TestCase ( SECTION,
	       "'" + s + "'.match(new RegExp('\\ba\\d+b\\b'))",
	       String(["a1111b"]), String(s.match(new RegExp('\\ba\\d+b\\b'))));

new TestCase ( SECTION,
	       "'" + s + "'.match(/\\ba\\d+b\\b/)",
	       String(["a1111b"]), String(s.match(/\ba\d+b\b/)));

test();
