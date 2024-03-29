





var BUGNUMBER = 462459;
var summary = 'TM: trace new Array()';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

if (!this.tracemonkey || this.tracemonkey.adaptive)
{
  jit(false);
  expect = actual = 'Test skipped due to lack of tracemonkey jitstats';
  reportCompare(expect, actual, summary);
}
else
{
  jit(true);

  expect = 'recorder started, recorder not aborted, trace completed';
  actual = '';

  var recorderStartedStart = this.tracemonkey.recorderStarted;
  var recorderAbortedStart = this.tracemonkey.recorderAborted;
  var traceCompletedStart  = this.tracemonkey.traceCompleted;


  for (var i = 0; i < RUNLOOP; i++)
  {
    new Array();
  }

  jit(false);

  var recorderStartedEnd = this.tracemonkey.recorderStarted;
  var recorderAbortedEnd = this.tracemonkey.recorderAborted;
  var traceCompletedEnd  = this.tracemonkey.traceCompleted;

  if (recorderStartedEnd > recorderStartedStart)
  {
    actual = 'recorder started, ';
  }
  else
  {
    actual = 'recorder not started, ';
  }

  if (recorderAbortedEnd > recorderAbortedStart)
  {
    actual += 'recorder aborted, ';
  }
  else
  {
    actual += 'recorder not aborted, ';
  }

  if (traceCompletedEnd > traceCompletedStart)
  {
    actual += 'trace completed';
  }
  else
  {
    actual += 'trace not completed';
  }

  reportCompare(expect, actual, summary);
}

