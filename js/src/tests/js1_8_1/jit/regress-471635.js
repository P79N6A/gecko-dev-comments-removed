





var BUGNUMBER = 471635;
var summary = 'TM: trace js shell print()';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  (function(){
    for (var i = 1; i < 20; ++i) {
      print("#");
    }
  })();

  var recorderStarted;
  var recorderAborted;
  var traceCompleted;
  var skip = true;

  if (this.tracemonkey && !this.tracemonkey.adaptive)
  {
    recorderStarted = this.tracemonkey.recorderStarted;
    recorderAborted = this.tracemonkey.recorderAborted;
    traceCompleted  = this.tracemonkey.traceCompleted;
    skip = false;
  }

  jit(false);

  if (!skip)
  {
    expect = 'recorderStarted=1, recorderAborted=0, traceCompleted=1';
    actual = 'recorderStarted=' + recorderStarted + ', recorderAborted=' + recorderAborted + ', traceCompleted=' + traceCompleted;
  }
  else
  {
    expect = actual = 'Test skipped due to lack of tracemonkey jitstats object.';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
