








if (getJitCompilerOptions()["ion.warmup.trigger"] <= 30)
    setJitCompilerOption("ion.warmup.trigger", 30);




if (getJitCompilerOptions()["ion.forceinlineCaches"])
    setJitCompilerOption("ion.forceinlineCaches", 0);

var uceFault = function (j) {
    if (j >= 31)
        uceFault = function (j) { return true; };
    return false;
}

function f(j) {
    var i = Math.pow(2, j) | 0;
    var obj = {
      i: i,
      v: i + i
    };
    assertRecoveredOnBailout(obj, false); 
    assertRecoveredOnBailout(obj.v, false); 
    if (uceFault(j) || uceFault(j)) {
        
        
        assertEq(obj.v, 2 * i);
    }
    return 2 * obj.i;
}

var min = -100;
for (var j = min; j <= 31; ++j) {
    with({}){};
    f(j);
}
