





































var bug = 245795;
var summary = 'eval(uneval(function)) should be round-trippable';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

if (typeof uneval != 'undefined')
{
  function a()
    {
      b=function() {};
    }

  var r = /function a\(\) \{ b = \(?function \(\) \{\s*\}\)?; \}/;
  eval(uneval(a));

  var v = a.toString().replace(/[ \n]+/g, ' ');
  
  printStatus("[" + v + "]");

  expect = true;
  actual = r.test(v);

  reportCompare(expect, actual, summary);
}
