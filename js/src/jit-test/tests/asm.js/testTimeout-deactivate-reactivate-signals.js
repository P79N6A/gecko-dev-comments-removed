

load(libdir + "asm.js");

var jco = getJitCompilerOptions();
if (jco["signals.enable"] === 0 || !isCachingEnabled() || !isAsmJSCompilationAvailable())
    quit(6);



setJitCompilerOption("signals.enable", 0);

var code = USE_ASM + "/* deactivate-reactivate-signals */ function f() {} function g() { while(1) { f() } } return g";

var m = asmCompile(code);
assertEq(isAsmJSModule(m), true);

setJitCompilerOption("signals.enable", 1);

var m = asmCompile(code);
assertEq(isAsmJSModule(m), true);
assertEq(isAsmJSModuleLoadedFromCache(m), true);

var g = asmLink(m);
timeout(1);
g();
assertEq(true, false);
