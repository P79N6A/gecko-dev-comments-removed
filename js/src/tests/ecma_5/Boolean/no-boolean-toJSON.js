





JSON.stringify(new Boolean(false), function(k, v) { 
    assertEq(typeof v, "object"); 
});

assertEq(Boolean.prototype.hasOwnProperty('toJSON'), false);

Object.prototype.toJSON = function() { return 2; };
assertEq(JSON.stringify(new Boolean(true)), "2");
