var value = {};
var ws = new WeakSet();

assertEq(ws.has(value), false);

assertEq(ws.clear(), undefined);
assertEq(ws.has(value), false);


ws.add(value);
assertEq(ws.has(value), true);
ws.clear();
assertEq(ws.has(value), false);


ws.add(value);
for (var i = 0; i < 10; i++)
    ws.add({});
assertEq(ws.has(value), true);
ws.clear();
assertEq(ws.has(value), false);


ws.add(value);
for (var i = 0; i < 10; i++)
    ws.add({});
assertEq(ws.has(value), true);
gc();
assertEq(ws.has(value), true);
ws.clear();
assertEq(ws.has(value), false);


ws.add(value);
value = null;
ws.clear();
gc();
var value2 = {};
ws.add(value2);
value2 = null;
gc();
ws.clear();
