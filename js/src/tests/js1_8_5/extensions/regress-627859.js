



var x = 42;
function a() { 
    var x;
    function b() {
        x = 43;
        
        
        
        
        
        
        (x for (x in []));
    }
    b();
}
a();
assertEq(x, 42);

reportCompare(true, true);
