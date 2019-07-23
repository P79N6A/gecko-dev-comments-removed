

































































































var gTestfile = '15.10.6.2-2.js';
var i = 0;
var BUGNUMBER = 76717;
var summary = 'Testing re.exec(str) when re.lastIndex is < 0 or > str.length';
var status = '';
var statusmessages = new Array();
var pattern = '';
var patterns = new Array();
var string = '';
var strings = new Array();
var actualmatch = '';
var actualmatches = new Array();
var expectedmatch = '';
var expectedmatches = new Array();







pattern = /abc/gi;
string = 'AbcaBcabC';

status = inSection(1);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc');
addThis();

status = inSection(2);
actualmatch = pattern.exec(string);
expectedmatch = Array('aBc');
addThis();

status = inSection(3);
actualmatch = pattern.exec(string);
expectedmatch = Array('abC');
addThis();




status = inSection(4);
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();




status = inSection(5);
pattern.lastIndex = -1;
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();










status = inSection(6);
pattern.lastIndex = Math.pow(2,32);
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();
 
status = inSection(7);
pattern.lastIndex = -Math.pow(2,32);
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(8);
pattern.lastIndex = Math.pow(2,32) + 1;
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(9);
pattern.lastIndex = -(Math.pow(2,32) + 1);
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(10);
pattern.lastIndex = Math.pow(2,32) * 2;
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(11);
pattern.lastIndex = -Math.pow(2,32) * 2;
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(12);
pattern.lastIndex = Math.pow(2,40);
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(13);
pattern.lastIndex = -Math.pow(2,40);
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(14);
pattern.lastIndex = Number.MAX_VALUE;
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();

status = inSection(15);
pattern.lastIndex = -Number.MAX_VALUE;
actualmatch = pattern.exec(string);
expectedmatch = null;
addThis();
 












pattern = /abc/i;
string = 'AbcaBcabC';

status = inSection(16);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc');
addThis();

status = inSection(17);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc'); 
addThis();

status = inSection(18);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc'); 
addThis();




status = inSection(19);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();




status = inSection(20);
pattern.lastIndex = -1;
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();










status = inSection(21);
pattern.lastIndex = Math.pow(2,32);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();
 
status = inSection(22);
pattern.lastIndex = -Math.pow(2,32);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(23);
pattern.lastIndex = Math.pow(2,32) + 1;
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(24);
pattern.lastIndex = -(Math.pow(2,32) + 1);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(25);
pattern.lastIndex = Math.pow(2,32) * 2;
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(26);
pattern.lastIndex = -Math.pow(2,32) * 2;
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(27);
pattern.lastIndex = Math.pow(2,40);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(28);
pattern.lastIndex = -Math.pow(2,40);
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(29);
pattern.lastIndex = Number.MAX_VALUE;
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
  addThis();

status = inSection(30);
pattern.lastIndex = -Number.MAX_VALUE;
actualmatch = pattern.exec(string);
expectedmatch = Array('Abc') 
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
