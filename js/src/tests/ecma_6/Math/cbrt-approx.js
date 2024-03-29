assertEq(Math.cbrt(1), 1);
assertEq(Math.cbrt(-1), -1);

var sloppy_tolerance = 200;  

assertNear(Math.cbrt(1e-300), 1e-100, sloppy_tolerance);
assertNear(Math.cbrt(-1e-300), -1e-100, sloppy_tolerance);

var cbrt_data = [
    [ Math.E, 1.3956124250860895 ], 
    [ Math.PI, 1.4645918875615231 ], 
    [ Math.LN2, 0.8849970445005177 ], 
    [ Math.SQRT2, 1.1224620483093728 ]
];

for (var [x, y] of cbrt_data)
    assertNear(Math.cbrt(x), y, sloppy_tolerance);

reportCompare(0, 0, "ok");
