






















































var BUGNUMBER = 57043;
var summary = 'Indexing object properties by signed numerical literals -'
  var statprefix = 'Adding a property to test object with an index of ';
var statsuffix =  ', testing it now -';
var propprefix = 'This is property ';
var obj = new Object();
var status = ''; var actual = ''; var expect = ''; var value = '';



var index =
  [-1073741825, -1073741824, -1073741823, -5000, -507, -3, -2, -1, -0, 0, 1, 2, 3, 1073741823, 1073741824, 1073741825];



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var j in index) {testProperty(index[j]);}

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

function positive(n) { return 1 / n > 0; }

function getStatus(i)
{
  return statprefix +
         (positive(i) ? i : "-" + -i) +
         statsuffix;
}
