










































































var gTestfile = 'regress-226507.js';
var UBound = 0;
var BUGNUMBER = 226507;
var summary = 'Testing for recursion check in js_EmitTree';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];






var N = 350;

var counter = 0;
function f()
{
  ++counter;
}













var source = "".concat(
  repeat_str("try { f(); } finally {\n", N),
  "f(",
  repeat_str("1,", N),
  "1);\n",
  repeat_str("}", N));


source += source;





if (typeof Script == 'undefined')
{
  print('Test skipped. Script not defined.');
  expect = actual = 0;
}
else
{
  try
  {
    var script = Script(source);
    script();


    status = inSection(1);
    actual = counter;
    expect = (N + 1) * 2;
  }
  catch(ex)
  {
    actual = ex + '';
  }
}
addThis();



test();




function repeat_str(str, repeat_count)
{
  var arr = new Array(--repeat_count);
  while (repeat_count != 0)
    arr[--repeat_count] = str;
  return str.concat.apply(str, arr);
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
