





































var BUGNUMBER = 476871;
var summary = 'Do not crash @ js_StepXMLListFilter';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

try
{
  this.watch("NaN", /a/g)
    let(x)
    ((function(){
        for each (NaN in [null, '', '', '', '']) true;
      }
      )());

  NaN.( /x/ );
}
catch(ex)
{
  print(ex + '');
}

jit(false);

reportCompare(expect, actual, summary);
