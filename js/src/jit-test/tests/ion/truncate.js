function f() {
    var x = Math.pow(2, 31); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 1); 
    x = x + x; assertEq((x + 1) | 0, 0); 
}

for (var i = 0; i <= 100000; i++)
    f();
