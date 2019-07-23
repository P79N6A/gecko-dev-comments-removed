













































var gTestfile = 'regress-169497.js';
var i = 0;
var BUGNUMBER = 169497;
var summary = 'RegExp conformance test';
var status = '';
var statusmessages = new Array();
var pattern = '';
var patterns = new Array();
var sBody = '';
var sHTML = '';
var string = '';
var strings = new Array();
var actualmatch = '';
var actualmatches = new Array();
var expectedmatch = '';
var expectedmatches = new Array();

sBody += '<body onXXX="alert(event.type);">\n';
sBody += '<p>Kibology for all<\/p>\n';
sBody += '<p>All for Kibology<\/p>\n';
sBody += '<\/body>';

sHTML += '<html>\n';
sHTML += sBody;
sHTML += '\n<\/html>';

status = inSection(1);
string = sHTML;
pattern = /<body.*>((.*\n?)*?)<\/body>/i;
actualmatch = string.match(pattern);
expectedmatch = Array(sBody, '\n<p>Kibology for all</p>\n<p>All for Kibology</p>\n', '<p>All for Kibology</p>\n');
addThis();




test();



function addThis()
{
  statusmessages[i] = status;
  patterns[i] = pattern;
  strings[i] = string;
  actualmatches[i] = actualmatch;
  expectedmatches[i] = expectedmatch;
  i++;
}


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
