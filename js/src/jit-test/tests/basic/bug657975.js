
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
trap(f4, 8, '')
f4()


function f5() {
  for ([, x] in 0) {}
}
trap(f5, 7, '')
f5()


function f6() {
  "use strict";
  print(Math.min(0, 1));
}
trap(f6, 10, '')
f6()


function f7() {
  try { y = w; } catch(y) {}
}
trap(f7, 14, '')
f7()


f8 = (function() {
  let x;
  yield
})
trap(f8, 6, undefined);
for (a in f8())
  (function() {})()


f9 = (function() {
  for (let a = 0; a < 0; ++a) {
    for each(let w in []) {}
  }
})
trap(f9, 22, undefined);
for (b in f9())
  (function() {})()


f10 = (function() {
    while (h) {
        continue
    }
})
trap(f10, 0, '');
try { f10() } catch (e) {}


f11 = Function("for (x = 0; x < 6; x++) { gc() }");
trap(f11, 23, '');
f11()
