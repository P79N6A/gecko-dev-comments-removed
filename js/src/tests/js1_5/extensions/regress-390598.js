






































var BUGNUMBER = 390598;
var summary = 'array_length_setter is exploitable';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function exploit() {
    var fun = function () {};
    fun.__proto__ = [];
    fun.length = 0x50505050 >> 1;
    fun();
  }
  exploit();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
