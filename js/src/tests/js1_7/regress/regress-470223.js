





var BUGNUMBER = 470223;
var summary = 'TM: Do not crash @ js_NewObjectWithGivenProto';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  function F(A) {
    for (R = [], s = 0; (m = A.indexOf("m", s++)) >= 0; )
      R.push([m]);
    for (i = R.length; i--; ) {
      let r = R[i];
      if (r[0]);
    }
  }
  F("m"); F("mm");

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
