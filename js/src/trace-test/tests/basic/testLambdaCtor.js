function testLambdaCtor() {
    var a = [];
    for (var x = 0; x < RUNLOOP; ++x) {
        var f = function(){};
        a[a.length] = new f;
    }

    
    

    
    return a[RUNLOOP-1].__proto__ === f.prototype;
}
assertEq(testLambdaCtor(), true);
