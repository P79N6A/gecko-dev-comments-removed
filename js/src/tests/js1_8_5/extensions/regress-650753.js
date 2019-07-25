


var x = {}, h = new WeakMap;
h.set(x, null);
gc();

reportCompare(0, 0, 'ok');
