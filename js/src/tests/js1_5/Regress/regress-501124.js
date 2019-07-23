




































var gTestfile = 'regress-501124.js';

var BUGNUMBER = 501124;
var summary = 'Crypotographic login routines';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var hexVal = "00000000000000000000000000000000DEADBABE";
  var nblk   = (((hexVal.length/2) + 8) >> 6) + 1;

  var blks = new Array(nblk * 16);

  for(var i = 0; i < nblk * 16; i++)
    blks[i] = 0;

  for(i = 0; i < hexVal.length; i++) {
    blks[i >> 3] |= ("0x"+hexVal.charAt(i)) << (28 - (i % 8) * 4);
  }

  expect = '0,0,0,0,-559039810,0,0,0,0,0,0,0,0,0,0,0';
  actual   = blks + '';

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
