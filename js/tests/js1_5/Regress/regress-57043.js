






















































var gTestfile = 'regress-57043.js';
var BUGNUMBER = 57043;
var summary = 'Indexing object properties by signed numerical literals -'
  var statprefix = 'Adding a property to test object with an index of ';
var statsuffix =  ', testing it now -';
var propprefix = 'This is property ';
var obj = new Object();
var status = ''; var actual = ''; var expect = ''; var value = '';



var index = Array(-5000, -507, -3, -2, -1, 0, 1, 2, 3); 



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (j in index) {testProperty(index[j]);}

  exitFunc ('test');
}


function testProperty(i)
{
  status = getStatus(i);

  
  obj[i] = value = (propprefix  +  i);  
 
  
  expect = value;
  actual = obj[i];
  reportCompare(expect, actual, status); 

  
  expect = value;
  actual = obj[String(i)];
  reportCompare(expect, actual, status);
}


function getStatus(i)
{
  return (statprefix  +  i  +  statsuffix);
}
