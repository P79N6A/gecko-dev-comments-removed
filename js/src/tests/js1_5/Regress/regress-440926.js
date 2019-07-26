





var BUGNUMBER = 440926;
var summary = 'Correctly match regexps with special "i" characters';
var actual = '';
var expect0 = '#I#,#I#;#I#,#I#';
var expect1 = 'iI#,iI#;iI#,iI#';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  actual += 'iI\u0130'.replace(/[\u0130]/gi, '#');
  actual += ',' + 'iI\u0130'.replace(/\u0130/gi, '#');

  jit(true);
  actual += ';' + 'iI\u0130'.replace(/[\u0130]/gi, '#');
  actual += ',' + 'iI\u0130'.replace(/\u0130/gi, '#');
  jit(false);

  
  if (actual == expect0)
    reportCompare(expect0, actual, summary);
  else
    reportCompare(expect1, actual, summary);

  exitFunc ('test');
}
