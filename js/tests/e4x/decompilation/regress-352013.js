





































var bug = 352013;
var summary = 'Decompilation with new operator redeaux';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var l, m, r;
var nTests = 0;



l = function () { (new x(y))[z]; };
expect = 'function () { (new x(y))[z]; }';
actual = l + '';
compareSource(++nTests, expect, actual);

m = function () { new (x(y))[z]; };
expect = 'function () { new (x(y)[z]); }';
actual = m + '';
compareSource(++nTests, expect, actual);

r = function () { new (x(y)[z]); };
expect = 'function () { new (x(y)[z]); }';
actual = r + '';
compareSource(++nTests, expect, actual);



l = function () { (new x(y)).@a; };
expect = 'function () { (new x(y)).@a; }';
actual = l + '';
compareSource(++nTests, expect, actual);

m = function () { new (x(y)).@a; };
expect = 'function () { new (x(y).@a); }';
actual = m + '';
compareSource(++nTests, expect, actual);

r = function () { new (x(y).@a); };
expect = 'function () { new (x(y).@a); }';
actual = r + '';
compareSource(++nTests, expect, actual);



l = function () { (new x(y)).@n::a; };
expect = 'function () { (new x(y)).@[n::a]; }';
actual = l + '';
compareSource(++nTests, expect, actual);

m = function () { new (x(y)).@n::a; };
expect = 'function () { new (x(y).@[n::a]); }';
actual = m + '';
compareSource(++nTests, expect, actual);

r = function () { new (x(y).@n::a); };
expect = 'function () { new (x(y).@[n::a]); }';
actual = r + '';
compareSource(++nTests, expect, actual);



l = function () { (new x(y)).n::z; };
expect = 'function () { (new x(y)).n::z; }';
actual = l + '';
compareSource(++nTests, expect, actual);

m = function () { new (x(y)).n::z; };
expect = 'function () { new (x(y).n::z); }';
actual = m + '';
compareSource(++nTests, expect, actual);

r = function () { new (x(y).n::z); };
expect = 'function () { new (x(y).n::z); }';
actual = r + '';
compareSource(++nTests, expect, actual);



l = function () { (new x(y)).n::[z]; };
expect = 'function () { (new x(y)).n::[z]; }';
actual = l + '';
compareSource(++nTests, expect, actual);

m = function () { new (x(y)).n::[z]; };
expect = 'function () { new (x(y).n::[z]); }';
actual = m + '';
compareSource(++nTests, expect, actual);

r = function () { new (x(y).n::[z]); };
expect = 'function () { new (x(y).n::[z]); }';
actual = r + '';
compareSource(++nTests, expect, actual);

END();
