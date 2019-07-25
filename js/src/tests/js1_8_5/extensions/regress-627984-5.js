



var o = ({});
o.p = function() {};
o.watch('p', function() { });
o.q = function() {}
delete o.p;
o.p = function() {};
assertEq(o.p, void 0);

reportCompare(0, 0, 'ok');
