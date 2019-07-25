



var o = Array;
o.p = function() {};
o.watch('p', function() { }); 
for(var x in o) { 
    o[x]; 
}
delete o.p;
o.p = function() {};
assertEq(o.p, void 0);

reportCompare(0, 0, 'ok');
