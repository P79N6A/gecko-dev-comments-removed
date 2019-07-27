load(libdir + "asm.js");


if (!getBuildConfiguration()["arm-simulator"])
    quit();

enableSPSProfiling();
enableSingleStepProfiling();
var m = asmCompile(USE_ASM + 'function f() {} return f');
asmLink(m)();
asmLink(m)();
