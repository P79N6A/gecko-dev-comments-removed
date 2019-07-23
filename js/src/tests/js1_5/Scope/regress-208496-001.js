














































var gTestfile = 'regress-208496-001.js';
var UBound = 0;
var BUGNUMBER = 208496;
var summary = 'Testing |with (f)| inside the definition of |function f()|';
var status = '';
var statusitems = [];
var actual = '(TEST FAILURE)';
var actualvalues = [];
var expect= '';
var expectedvalues = [];





function f(par)
{
  var a = par;

  with(f)
  {
    var b = par;
    actual = b;
  }
}

status = inSection(1);
f('abc'); 
expect = 'abc';
addThis();

status = inSection(2);
f(111 + 222); 
expect = 333;
addThis();





var s = '';
s += 'function F(par)';
s += '{';
s += '  var a = par;';

s += '  with(F)';
s += '  {';
s += '    var b = par;';
s += '    actual = b;';
s += '  }';
s += '}';

s += 'status = inSection(3);';
s += 'F("abc");'; 
s += 'expect = "abc";';
s += 'addThis();';

s += 'status = inSection(4);';
s += 'F(111 + 222);'; 
s += 'expect = 333;';
s += 'addThis();';
eval(s);





function g(par)
{
  
  var a = '(TEST FAILURE)';
  var b = '(TEST FAILURE)';
  h(par);

  function h(par)
  {
    var a = par;

    with(h)
    {
      var b = par;
      actual = b;
    }
  }
}

status = inSection(5);
g('abc'); 
expect = 'abc';
addThis();

status = inSection(6);
g(111 + 222); 
expect = 333;
addThis();





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
