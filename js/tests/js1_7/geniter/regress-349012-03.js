




































var bug = 349012;
var summary = 'generator recursively calling itself via send is a TypeError';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  function gen() {
    var iter = yield;
    try {
      iter.send(1);
    } catch (e) {
      yield e;
    }
  }

  expect = true;
  var iter = gen();
  iter.next();
  var ex = iter.send(iter);
  print(ex + '');
  actual = (ex instanceof TypeError);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
