
(function(){
    
    (function(){
        var g = {};
        x = Float32Array()
        Function('g', "g.o = x[1]")(g);
    })();
    
    (function() {
        var g = new Float32Array(16);
        var h = new Float64Array(16);
        var farrays = [ g, h ];
        for (aridx = 0; aridx < farrays.length; ++aridx) {
            ar  = farrays[aridx];
            !(ar[ar.length-2] == (NaN / Infinity)[ar.length-2])
        }
    })();
    
    (function () {
        var v = new Float32Array(32);
        for (var i = 0; i < v.length; ++i)
        v[i] = i;
    var t = (false  );
    for (var i = 0; i < i .length; ++i)
        t += v[i];
    })();
    
    (function() {
        if (typeof ParallelArray !== "undefined")
        ParallelArray([1606], Math.fround)
    })();
    
    (function() {
        x = y = {};
        z = Float32Array(6)
        for (c in this) {
            Array.prototype.unshift.call(x, ArrayBuffer())
        }
        Array.prototype.sort.call(x, (function (j) {
            y.s = z[2]
        }))
    })();
    
})();









setJitCompilerOption("ion.usecount.trigger", 50);

function test(f) {
    f32[0] = 1;
    for(var n = 110; n; n--)
        f();
}

var f32 = new Float32Array(2);
var f64 = new Float64Array(2);

function acceptAdd() {
    var use = f32[0] + 1;
    assertFloat32(use, true);
    f32[0] = use;
}
test(acceptAdd);

function acceptAddSeveral() {
    var sum1 = f32[0] + 0.5;
    var sum2 = f32[0] + 0.5;
    f32[0] = sum1;
    f32[0] = sum2;
    assertFloat32(sum1, true);
    assertFloat32(sum2, true);
}
test(acceptAddSeveral);

function acceptAddVar() {
    var x = f32[0] + 1;
    f32[0] = x;
    f32[1] = x;
    assertFloat32(x, true);
}
test(acceptAddVar);

function refuseAddCst() {
    var x = f32[0] + 1234567890; 
    f32[0] = x;
    assertFloat32(x, false);
}
test(refuseAddCst);

function refuseAddVar() {
    var x = f32[0] + 1;
    f32[0] = x;
    f32[1] = x;
    f64[1] = x; 
    assertFloat32(x, false);
}
test(refuseAddVar);

function refuseAddStore64() {
    var x = f32[0] + 1;
    f64[0] = x; 
    f32[0] = f64[0];
    assertFloat32(x, false);
}
test(refuseAddStore64);

function refuseAddStoreObj() {
    var o = {}
    var x = f32[0] + 1;
    o.x = x; 
    f32[0] = o['x'];
    assertFloat32(x, false);
}
test(refuseAddStoreObj);

function refuseAddSeveral() {
    var sum = (f32[0] + 2) - 1; 
    f32[0] = sum;
    assertFloat32(sum, false);
}
test(refuseAddSeveral);

function refuseAddFunctionCall() {
    function plusOne(x) { return Math.cos(x+1)*13.37; }
    var res = plusOne(f32[0]); 
    f32[0] = res;
    assertFloat32(res, false);
}
test(refuseAddFunctionCall);

function acceptTrigo() {
    var res = Math.cos(f32[0]);
    f32[0] = res;
    assertFloat32(res, true);

    var res = Math.sin(f32[0]);
    f32[0] = res;
    assertFloat32(res, true);

    var res = Math.tan(f32[0]);
    f32[0] = res;
    assertFloat32(res, true);

    var res = Math.acos(f32[0]);
    f32[0] = res;
    assertFloat32(res, true);

    var res = Math.asin(f32[0]);
    f32[0] = res;
    assertFloat32(res, true);

    res = Math.atan(f32[0]);
    f32[0] = res;
    assertFloat32(res, true);
}
test(acceptTrigo);

function refuseMath() {
    var res = Math.log10(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.log2(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.log1p(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.expm1(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.cosh(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.sinh(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.tanh(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.acosh(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.asinh(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.atanh(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.cbrt(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.sign(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);

    res = Math.trunc(f32[0]);
    f32[0] = res;
    assertFloat32(res, false);
}
test(refuseMath);

function refuseLoop() {
    var res = f32[0],
        n = 10;
    while (n--) {
        res = res + 1; 
        assertFloat32(res, false);
    }
    assertFloat32(res, false);
    f32[0] = res;
}
test(refuseLoop);

function acceptLoop() {
    var res = f32[0],
        n = 10;
    while (n--) {
        var sum = res + 1;
        res = Math.fround(sum);
        assertFloat32(sum, true);
    }
    assertFloat32(res, true);
    f32[0] = res;
}
test(acceptLoop);

function alternateCond(n) {
    var x = f32[0];
    if (n > 0) {
        var s1 = x + 1;
        f32[0] = s1;
        assertFloat32(s1, true);
    } else {
        var s2 = x + 1;
        f64[0] = s2; 
        assertFloat32(s2, false);
    }
}
(function() {
    f32[0] = 0;
    for (var n = 110; n; n--) {
        alternateCond(n % 2);
    }
})();

function phiTest(n) {
    var x = (f32[0]);
    var y = n;
    if (n > 0) {
        x = x + 2;
        assertFloat32(x, true);
    } else {
        if (n < -10) {
            x = Math.fround(Math.cos(y));
            assertFloat32(x, true);
        } else {
            x = x - 1;
            assertFloat32(x, true);
        }
    }
    assertFloat32(x, true);
    f32[0] = x;
}
(function() {
    f32[0] = 0;
    for (var n = 100; n; n--) {
        phiTest( ((n % 3) - 1) * 15 );
    }
})();

function mixedPhiTest(n) {
    var x = (f32[0]);
    var y = n;
    if (n > 0) {
        x = x + 2; 
        assertFloat32(x, false);
    } else {
        if (n < -10) {
            x = Math.fround(Math.cos(y)); 
            assertFloat32(x, true);
        } else {
            x = x - 1; 
            assertFloat32(x, false);
        }
    }
    assertFloat32(x, false);
    x = x + 1; 
    f32[0] = x;
}
(function() {
    f32[0] = 0;
    for (var n = 100; n; n--) {
        mixedPhiTest( ((n % 3) - 1) * 15 );
    }
})();

function phiTest2(n) {
    var x = f32[0];
    while (n >= 0) {
        x = Math.fround(Math.fround(x) + 1);
        assertFloat32(x, true);
        if (n < 10) {
            x = f32[0] + 1;
            assertFloat32(x, true);
        }
        n = n - 1;
    }
}
(function(){
    f32[0] = 0;
    for (var n = 100; n > 10; n--) {
        phiTest2(n);
    }
})();
