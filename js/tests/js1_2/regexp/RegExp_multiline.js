





































gTestfile = 'RegExp_multiline.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: multiline';

writeHeaderToLog('Executing script: RegExp_multiline.js');
writeHeaderToLog( SECTION + " "+ TITLE);




new TestCase ( SECTION, "RegExp.multiline",
	       false, RegExp.multiline);


new TestCase ( SECTION, "(multiline == false) '123\\n456'.match(/^4../)",
	       null, '123\n456'.match(/^4../));


new TestCase ( SECTION, "(multiline == false) 'a11\\na22\\na23\\na24'.match(/^a../g)",
	       String(['a11']), String('a11\na22\na23\na24'.match(/^a../g)));


new TestCase ( SECTION, "(multiline == false) 'a11\na22'.match(/^.+^./)",
	       null, 'a11\na22'.match(/^.+^./));


new TestCase ( SECTION, "(multiline == false) '123\\n456'.match(/.3$/)",
	       null, '123\n456'.match(/.3$/));


new TestCase ( SECTION, "(multiline == false) 'a11\\na22\\na23\\na24'.match(/a..$/g)",
	       String(['a24']), String('a11\na22\na23\na24'.match(/a..$/g)));


new TestCase ( SECTION, "(multiline == false) 'abc\ndef'.match(/c$...$/)",
	       null, 'abc\ndef'.match(/c$...$/));


new TestCase ( SECTION, "(multiline == false) 'a11\\na22\\na23\\na24'.match(new RegExp('a..$','g'))",
	       String(['a24']), String('a11\na22\na23\na24'.match(new RegExp('a..$','g'))));


new TestCase ( SECTION, "(multiline == false) 'abc\ndef'.match(new RegExp('c$...$'))",
	       null, 'abc\ndef'.match(new RegExp('c$...$')));



RegExp.multiline = true;
new TestCase ( SECTION, "RegExp.multiline = true; RegExp.multiline",
	       true, RegExp.multiline);


new TestCase ( SECTION, "(multiline == true) '123\\n456'.match(/^4../)",
	       String(['456']), String('123\n456'.match(/^4../)));


new TestCase ( SECTION, "(multiline == true) 'a11\\na22\\na23\\na24'.match(/^a../g)",
	       String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(/^a../g)));






new TestCase ( SECTION, "(multiline == true) '123\\n456'.match(/.3$/)",
	       String(['23']), String('123\n456'.match(/.3$/)));


new TestCase ( SECTION, "(multiline == true) 'a11\\na22\\na23\\na24'.match(/a..$/g)",
	       String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(/a..$/g)));


new TestCase ( SECTION, "(multiline == true) 'a11\\na22\\na23\\na24'.match(new RegExp('a..$','g'))",
	       String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(new RegExp('a..$','g'))));





RegExp.multiline = false;

test();
