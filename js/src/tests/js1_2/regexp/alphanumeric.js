





































gTestfile = 'alphanumeric.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: \\w and \\W';

writeHeaderToLog('Executing script: alphanumeric.js');
writeHeaderToLog( SECTION + " " + TITLE);

var non_alphanumeric = "~`!@#$%^&*()-+={[}]|\\:;'<,>./?\f\n\r\t\v " + '"';
var alphanumeric     = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";


new TestCase ( SECTION,
	       "'" + alphanumeric + "'.match(new RegExp('\\w+'))",
	       String([alphanumeric]), String(alphanumeric.match(new RegExp('\\w+'))));


new TestCase ( SECTION,
	       "'" + non_alphanumeric + "'.match(new RegExp('\\W+'))",
	       String([non_alphanumeric]), String(non_alphanumeric.match(new RegExp('\\W+'))));


new TestCase ( SECTION,
	       "'" + non_alphanumeric + "'.match(new RegExp('\\w'))",
	       null, non_alphanumeric.match(new RegExp('\\w')));


new TestCase ( SECTION,
	       "'" + alphanumeric + "'.match(new RegExp('\\W'))",
	       null, alphanumeric.match(new RegExp('\\W')));

var s = non_alphanumeric + alphanumeric;


new TestCase ( SECTION,
	       "'" + s + "'.match(new RegExp('\\w+'))",
	       String([alphanumeric]), String(s.match(new RegExp('\\w+'))));

s = alphanumeric + non_alphanumeric;


new TestCase ( SECTION,
	       "'" + s + "'.match(new RegExp('\\W+'))",
	       String([non_alphanumeric]), String(s.match(new RegExp('\\W+'))));


new TestCase ( SECTION,
	       "'" + s + "'.match(/\w+/)",
	       String([alphanumeric]), String(s.match(/\w+/)));

s = alphanumeric + non_alphanumeric;


new TestCase ( SECTION,
	       "'" + s + "'.match(/\W+/)",
	       String([non_alphanumeric]), String(s.match(/\W+/)));

s = 'abcd*&^%$$';

new TestCase ( SECTION,
	       "'" + s + "'.match(/(\w+)...(\W+)/)",
	       String([s , 'abcd' , '%$$']), String(s.match(/(\w+)...(\W+)/)));

var i;


for (i = 0; i < alphanumeric.length; ++i)
{
  s = '#$' + alphanumeric[i] + '%^';
  new TestCase ( SECTION,
		 "'" + s + "'.match(new RegExp('\\w'))",
		 String([alphanumeric[i]]), String(s.match(new RegExp('\\w'))));
}

for (i = 0; i < non_alphanumeric.length; ++i)
{
  s = 'sd' + non_alphanumeric[i] + String((i+10) * (i+10) - 2 * (i+10));
  new TestCase ( SECTION,
		 "'" + s + "'.match(new RegExp('\\W'))",
		 String([non_alphanumeric[i]]), String(s.match(new RegExp('\\W'))));
}

test();
