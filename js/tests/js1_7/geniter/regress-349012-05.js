




































var bug = 349012;
var summary = 'generator recursively calling itself via close is an Error';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var iter;
  function gen() {
    iter.close();
    yield 1;
  }

  expect = true;
  try
  {
    iter = gen();
    var i = iter.next();
    print("i="+i);
  }
  catch(ex)
  {
    print(ex + '');
    actual = (ex instanceof TypeError) && (ex + '').indexOf(' already executing generator') != -1;
  }
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
