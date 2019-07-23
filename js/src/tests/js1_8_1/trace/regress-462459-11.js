




































var gTestfile = 'regress-462459-11.js';

var BUGNUMBER = 462459;
var summary = 'TM: trace [1, 2]';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

if (!this.tracemonkey)
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


  for (var i = 0; i < 5; i++)
  {
    [1, 2];
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

