





































gTestfile = 'flags.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'regular expression flags with flags "i" and "g"';

writeHeaderToLog('Executing script: flags.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'aBCdEfGHijKLmno'.match(/fghijk/i)",
	       String(["fGHijK"]), String('aBCdEfGHijKLmno'.match(/fghijk/i)));

new TestCase ( SECTION, "'aBCdEfGHijKLmno'.match(new RegExp('fghijk','i'))",
	       String(["fGHijK"]), String('aBCdEfGHijKLmno'.match(new RegExp("fghijk","i"))));


new TestCase ( SECTION, "'xa xb xc xd xe xf'.match(/x./g)",
	       String(["xa","xb","xc","xd","xe","xf"]), String('xa xb xc xd xe xf'.match(/x./g)));

new TestCase ( SECTION, "'xa xb xc xd xe xf'.match(new RegExp('x.','g'))",
	       String(["xa","xb","xc","xd","xe","xf"]), String('xa xb xc xd xe xf'.match(new RegExp('x.','g'))));


new TestCase ( SECTION, "'xa Xb xc xd Xe xf'.match(/x./gi)",
	       String(["xa","Xb","xc","xd","Xe","xf"]), String('xa Xb xc xd Xe xf'.match(/x./gi)));

new TestCase ( SECTION, "'xa Xb xc xd Xe xf'.match(new RegExp('x.','gi'))",
	       String(["xa","Xb","xc","xd","Xe","xf"]), String('xa Xb xc xd Xe xf'.match(new RegExp('x.','gi'))));

new TestCase ( SECTION, "'xa Xb xc xd Xe xf'.match(/x./ig)",
	       String(["xa","Xb","xc","xd","Xe","xf"]), String('xa Xb xc xd Xe xf'.match(/x./ig)));

new TestCase ( SECTION, "'xa Xb xc xd Xe xf'.match(new RegExp('x.','ig'))",
	       String(["xa","Xb","xc","xd","Xe","xf"]), String('xa Xb xc xd Xe xf'.match(new RegExp('x.','ig'))));


test();

