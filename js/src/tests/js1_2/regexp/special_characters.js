













var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: special_charaters';

writeHeaderToLog('Executing script: special_characters.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'^abcdefghi'.match(/\^abc/)", String(["^abc"]), String('^abcdefghi'.match(/\^abc/)));


new TestCase ( SECTION, "'abcdefghi'.match(/^abc/)", String(["abc"]), String('abcdefghi'.match(/^abc/)));


new TestCase ( SECTION, "'abcdefghi'.match(/fghi$/)", String(["ghi"]), String('abcdefghi'.match(/ghi$/)));


new TestCase ( SECTION, "'eeeefghi'.match(/e*/)", String(["eeee"]), String('eeeefghi'.match(/e*/)));


new TestCase ( SECTION, "'abcdeeeefghi'.match(/e+/)", String(["eeee"]), String('abcdeeeefghi'.match(/e+/)));


new TestCase ( SECTION, "'abcdefghi'.match(/abc?de/)", String(["abcde"]), String('abcdefghi'.match(/abc?de/)));


new TestCase ( SECTION, "'abcdefghi'.match(/c.e/)", String(["cde"]), String('abcdefghi'.match(/c.e/)));


new TestCase ( SECTION, "'abcewirjskjdabciewjsdf'.match(/(abc).+\\1'/)",
	       String(["abcewirjskjdabc","abc"]), String('abcewirjskjdabciewjsdf'.match(/(abc).+\1/)));


new TestCase ( SECTION, "'abcdefghi'.match(/xyz|def/)", String(["def"]), String('abcdefghi'.match(/xyz|def/)));


new TestCase ( SECTION, "'abcdeeeefghi'.match(/e{3}/)", String(["eee"]), String('abcdeeeefghi'.match(/e{3}/)));


new TestCase ( SECTION, "'abcdeeeefghi'.match(/e{3,}/)", String(["eeee"]), String('abcdeeeefghi'.match(/e{3,}/)));


new TestCase ( SECTION, "'abcdeeeefghi'.match(/e{2,8}/)", String(["eeee"]), String('abcdeeeefghi'.match(/e{2,8}/)));


new TestCase ( SECTION, "'abcdefghi'.match(/cd[xey]fgh/)", String(["cdefgh"]), String('abcdefghi'.match(/cd[xey]fgh/)));


new TestCase ( SECTION, "'netscape inc'.match(/t[r-v]ca/)", String(["tsca"]), String('netscape inc'.match(/t[r-v]ca/)));


new TestCase ( SECTION, "'abcdefghi'.match(/cd[^xy]fgh/)", String(["cdefgh"]), String('abcdefghi'.match(/cd[^xy]fgh/)));


new TestCase ( SECTION, "'netscape inc'.match(/t[^a-c]ca/)", String(["tsca"]), String('netscape inc'.match(/t[^a-c]ca/)));


new TestCase ( SECTION, "'this is b\ba test'.match(/is b[\b]a test/)",
	       String(["is b\ba test"]), String('this is b\ba test'.match(/is b[\b]a test/)));


new TestCase ( SECTION, "'today is now - day is not now'.match(/\bday.*now/)",
	       String(["day is not now"]), String('today is now - day is not now'.match(/\bday.*now/)));




new TestCase ( SECTION, "'a dog - 1 dog'.match(/\d dog/)", String(["1 dog"]), String('a dog - 1 dog'.match(/\d dog/)));


new TestCase ( SECTION, "'a dog - 1 dog'.match(/\D dog/)", String(["a dog"]), String('a dog - 1 dog'.match(/\D dog/)));


new TestCase ( SECTION, "'a b a\fb'.match(/a\fb/)", String(["a\fb"]), String('a b a\fb'.match(/a\fb/)));


new TestCase ( SECTION, "'a b a\nb'.match(/a\nb/)", String(["a\nb"]), String('a b a\nb'.match(/a\nb/)));


new TestCase ( SECTION, "'a b a\rb'.match(/a\rb/)", String(["a\rb"]), String('a b a\rb'.match(/a\rb/)));


new TestCase ( SECTION, "'xa\f\n\r\t\vbz'.match(/a\s+b/)", String(["a\f\n\r\t\vb"]), String('xa\f\n\r\t\vbz'.match(/a\s+b/)));


new TestCase ( SECTION, "'a\tb a b a-b'.match(/a\Sb/)", String(["a-b"]), String('a\tb a b a-b'.match(/a\Sb/)));


new TestCase ( SECTION, "'a\t\tb a  b'.match(/a\t{2}/)", String(["a\t\t"]), String('a\t\tb a  b'.match(/a\t{2}/)));


new TestCase ( SECTION, "'a\v\vb a  b'.match(/a\v{2}/)", String(["a\v\v"]), String('a\v\vb a  b'.match(/a\v{2}/)));


new TestCase ( SECTION, "'%AZaz09_$'.match(/\w+/)", String(["AZaz09_"]), String('%AZaz09_$'.match(/\w+/)));


new TestCase ( SECTION, "'azx$%#@*4534'.match(/\W+/)", String(["$%#@*"]), String('azx$%#@*4534'.match(/\W+/)));


new TestCase ( SECTION, "'test'.match(/(t)es\\1/)", String(["test","t"]), String('test'.match(/(t)es\1/)));


new TestCase ( SECTION, "'abcdef'.match(/\x63\x64/)", String(["cd"]), String('abcdef'.match(/\x63\x64/)));


new TestCase ( SECTION, "'abcdef'.match(/\\143\\144/)", String(["cd"]), String('abcdef'.match(/\143\144/)));

test();

