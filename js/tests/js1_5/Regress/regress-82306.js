















































var gTestfile = 'regress-82306.js';
var BUGNUMBER = 82306;
var summary = "Testing we don't crash on encodeURI()";
var URI = '';



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  URI += '<?xml version="1.0"?>';
  URI += '<zcti application="xxxx_demo">';
  URI += '<pstn_data>';
  URI += '<ani>650-930-xxxx</ani>';
  URI += '<dnis>877-485-xxxx</dnis>';
  URI += '</pstn_data>';
  URI += '<keyvalue key="name" value="xxx"/>';
  URI += '<keyvalue key="phone" value="6509309000"/>';
  URI += '</zcti>';

  
  encodeURI(URI);

  reportCompare('No Crash', 'No Crash', '');

  exitFunc ('test');
}
