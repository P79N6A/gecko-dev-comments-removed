





var BUGNUMBER = 364023;
var summary = 'Do not crash in JS_GetPrivate';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function exploit() {
    var code = "";
    for(var i = 0; i < 0x10000; i++) {
      if(i == 125) {
        code += "void 0x10000050505050;\n";
      } else {
        code += "void " + (0x10000000000000 + i) + ";\n";
      }
    }
    code += "function foo() {}\n";
    eval(code);
  }
  exploit();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
