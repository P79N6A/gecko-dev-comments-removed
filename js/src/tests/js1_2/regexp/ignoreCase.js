





































gTestfile = 'ignoreCase.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: ignoreCase';

writeHeaderToLog('Executing script: ignoreCase.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "/xyz/i.ignoreCase",
	       true, /xyz/i.ignoreCase);


new TestCase ( SECTION, "/xyz/.ignoreCase",
	       false, /xyz/.ignoreCase);


new TestCase ( SECTION, "'ABC def ghi'.match(/[a-z]+/ig)",
	       String(["ABC","def","ghi"]), String('ABC def ghi'.match(/[a-z]+/ig)));


new TestCase ( SECTION, "'ABC def ghi'.match(/[a-z]+/i)",
	       String(["ABC"]), String('ABC def ghi'.match(/[a-z]+/i)));


new TestCase ( SECTION, "'ABC def ghi'.match(/([a-z]+)/ig)",
	       String(["ABC","def","ghi"]), String('ABC def ghi'.match(/([a-z]+)/ig)));


new TestCase ( SECTION, "'ABC def ghi'.match(/([a-z]+)/i)",
	       String(["ABC","ABC"]), String('ABC def ghi'.match(/([a-z]+)/i)));


new TestCase ( SECTION, "'ABC def ghi'.match(/[a-z]+/)",
	       String(["def"]), String('ABC def ghi'.match(/[a-z]+/)));


new TestCase ( SECTION, "(new RegExp('xyz','i')).ignoreCase",
	       true, (new RegExp('xyz','i')).ignoreCase);


new TestCase ( SECTION, "(new RegExp('xyz')).ignoreCase",
	       false, (new RegExp('xyz')).ignoreCase);


new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('[a-z]+','ig'))",
	       String(["ABC","def","ghi"]), String('ABC def ghi'.match(new RegExp('[a-z]+','ig'))));


new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('[a-z]+','i'))",
	       String(["ABC"]), String('ABC def ghi'.match(new RegExp('[a-z]+','i'))));


new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('([a-z]+)','ig'))",
	       String(["ABC","def","ghi"]), String('ABC def ghi'.match(new RegExp('([a-z]+)','ig'))));


new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('([a-z]+)','i'))",
	       String(["ABC","ABC"]), String('ABC def ghi'.match(new RegExp('([a-z]+)','i'))));


new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('[a-z]+'))",
	       String(["def"]), String('ABC def ghi'.match(new RegExp('[a-z]+'))));

test();
