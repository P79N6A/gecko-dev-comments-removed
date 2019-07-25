






let t = 0;
gczeal(2,1);
eval("\
let x = 3, y = 4;\
let (x = x+0, y = 12) { t += (x + y); }  \
let (x = x+1, y = 12) { t += (x + y); }  \
let (x = x+2, y = 12) { t += (x + y); }  \
let (x = x+3, y = 12) { t += (x + y); }  \
let (x = x+4, y = 12) { t += (x + y); }  \
let (x = x+5, y = 12) { t += (x + y); }  \
let (x = x+6, y = 12) { t += (x + y); }  \
let (x = x+7, y = 12) { t += (x + y); }  \
let (x = x+8, y = 12) { t += (x + y); }  \
let (x = x+9, y = 12) { t += (x + y); }  \
t += (x + y);\
assertEq(t, 202);\
");
