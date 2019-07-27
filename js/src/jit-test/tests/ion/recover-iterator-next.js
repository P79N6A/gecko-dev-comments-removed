








if (getJitCompilerOptions()["ion.warmup.trigger"] <= 90)
    setJitCompilerOption("ion.warmup.trigger", 90);




if (getJitCompilerOptions()["ion.forceinlineCaches"])
    setJitCompilerOption("ion.forceinlineCaches", 0);

var arr = new Array();
var max = 2000;
for (var i=0; i < max; i++)
    arr[i] = i;

function f() {
    var res = 0;
    var nextObj;
    var itr = arr[Symbol.iterator]();
    do {
        nextObj = itr.next();
        if (nextObj.done)
          break;
        res += nextObj.value;
        assertRecoveredOnBailout(nextObj, true);
    } while (true);
    return res;
}

for (var j = 0; j < 10; j++)
  assertEq(f(), max * (max - 1) / 2);
