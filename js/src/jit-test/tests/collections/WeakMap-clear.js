var key = {};
var wm = new WeakMap();

assertEq(wm.has(key), false);

wm.clear();
assertEq(wm.has(key), false);


wm.set(key, 42);
assertEq(wm.has(key), true);
wm.clear();
assertEq(wm.has(key), false);


wm.set(key, {});
for (var i = 0; i < 10; i++)
    wm.set({}, {});
assertEq(wm.has(key), true);
wm.clear();
assertEq(wm.has(key), false);


wm.set(key, {});
for (var i = 0; i < 10; i++)
    wm.set({}, {});
assertEq(wm.has(key), true);
gc();
assertEq(wm.has(key), true);
wm.clear();
assertEq(wm.has(key), false);


wm.set(key, {});
key = null;
wm.clear();
gc();
var key2 = {};
wm.set(key2, {});
key2 = null;
gc();
wm.clear();
