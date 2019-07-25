
setDebug(true);


function f1(){ "use strict"; options('strict'); }
trap(f1, 0, '')
f1()


function f2(){ with({a:0}){}; }
trap(f2, 0, '')
f2()

x = 0;


function f3(){ for(y in x); }
trap(f3, 5, '')
f3()


function f4(){ for(y in x); }
trap(f4, 10, '')
f4()


function f5() {
  for ([, x] in 0) {}
}
trap(f5, 9, '')
f5()


function f6() {
  "use strict";
  print(Math.min(0, 1));
}
trap(f6, 10, '')
f6()
