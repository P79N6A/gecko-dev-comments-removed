







































var gTestfile = 'fe-001.js';

if (1) function f() {return 1;}
if (0) function f() {return 0;}

function test()
{
  enterFunc ("test");

  printStatus ("Function Expression Statements basic test.");
   
  reportCompare (1, f(), "Both functions were defined.");
   
  exitFunc ("test");
}

test();
