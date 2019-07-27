


















var UBound = 0;
var BUGNUMBER = 127557;
var summary = 'Testing cloned function objects';
var cnCOMMA = ',';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

if (typeof clone == 'function')
{
  status = inSection(1);
  var f = evaluate("(function(x, y) {\n" +
                   "    function h() { return h_peer(); }\n" +
                   "    function h_peer() { return (x + cnCOMMA + y); }\n" +
                   "    return h;\n" +
                   "})");
  var g = clone(f);
  g.prototype = new Object;
  var h = g(5,6);
  actual = h();
  expect = '5,6';
  addThis();
}
else
{
  reportCompare('Test not run', 'Test not run', 'shell only test requires clone()');
}




test();




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

