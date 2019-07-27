





var BUGNUMBER = 402386;
var summary = 'Automatic Semicolon insertion in restricted statements';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var code;

  code = '(function() { label1: for (;;) { continue \n label1; }})';
  actual = uneval(eval(code));
  compareSource(code, actual, summary + ': ' + code);

  code = '(function() { label2: for (;;) { break \n label2; }})';
  actual = uneval(eval(code));
  compareSource(code, actual, summary + ': ' + code);

  code = '(function() { return \n x++; })';
  actual = uneval(eval(code));
  compareSource(code, actual, summary + ': ' + code);

  print('see bug 256617');
  code = '(function() { throw \n x++; })';

  expect = 'SyntaxError: no line break is allowed between \'throw\' and its expression';
  try { uneval(eval(code)); } catch(ex) { actual = ex + ''; };

  reportCompare(expect, actual, summary + ': ' + code);

  exitFunc ('test');
}

