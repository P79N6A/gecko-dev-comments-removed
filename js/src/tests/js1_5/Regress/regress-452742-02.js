





var BUGNUMBER = 452742;
var summary = 'Do not do overzealous eval inside function optimization in BindNameToSlot';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '';

  var obj = { arguments: [-100] };

  function a()
  {
    with (obj) { return eval("arguments[0]"); }
  }

  function b()
  {
    var result;
    eval('with (obj) { result = eval("arguments[0]"); };');
    return result;
  }

  try
  {
    var result = a();
    if (result !== -100)
      throw "Bad result " + result;

    var result = b();
    if (result !== -100)
      throw "Bad result " + result;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
