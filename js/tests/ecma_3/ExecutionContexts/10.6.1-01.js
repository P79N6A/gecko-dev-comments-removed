




































var gTestfile = '10.6.1-01.js';

var BUGNUMBER = 290774;
var summary = 'activation object never delegates to Object.prototype';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var toStringResult;
var evalResult;
var watchResult;
var parseIntResult;

var eval = 'fooEval';
var watch = undefined;
var parseInt = 'fooParseInt';


function toString()
{
  return 'fooString';
}

function normal()
{
  toStringResult = toString;
  evalResult = eval;
  watchResult = watch;
  parseIntResult = parseInt;
}

function outerinnervar()
{
  toStringResult = toString;
  evalResult = eval;
  watchResult = watch;
  parseIntResult = parseInt;
  function inner()
  {
    
    
    
    printStatus(toString);
  }
}

expect = true;

printStatus('normal');
printStatus('======');
normal();

printStatus('toStringResult ' + toStringResult);
printStatus('toString ' + toString);
actual = ((toStringResult + '') == (toString + ''));
reportCompare(expect, actual, inSection(1));

printStatus('evalResult ' + evalResult);
printStatus('eval ' + eval);
actual = ((evalResult + '') == (eval + ''));
reportCompare(expect, actual, inSection(2));

printStatus('watchResult ' + watchResult);
printStatus('watch ' + watch);
actual = ((watchResult + '') == (watch + ''));
reportCompare(expect, actual, inSection(3));

printStatus('parseIntResult ' + parseIntResult);
printStatus('parseInt ' + parseInt);
actual = ((parseIntResult + '') == (parseInt + ''));
reportCompare(expect, actual, inSection(4));

printStatus('outerinner');
printStatus('==========');

outerinnervar();

printStatus('toStringResult ' + toStringResult);
printStatus('toString ' + toString);
actual = ((toStringResult + '') == (toString + ''));
reportCompare(expect, actual, inSection(5));


printStatus('evalResult ' + evalResult);
printStatus('eval ' + eval);
actual = ((evalResult + '') == (eval + ''));
reportCompare(expect, actual, inSection(6));

printStatus('watchResult ' + watchResult);
printStatus('watch ' + watch);
actual = ((watchResult + '') == (watch + ''));
reportCompare(expect, actual, inSection(7));

printStatus('parseIntResult ' + parseIntResult);
printStatus('parseInt ' + parseInt);
actual = ((parseIntResult + '') == (parseInt + ''));
reportCompare(expect, actual, inSection(8));
