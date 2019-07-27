



setCachingEnabled(true);
if (!isAsmJSCompilationAvailable())
    quit();





var code = "setIonCheckGraphCoherency(false); setCachingEnabled(true); load('" + libdir + "bullet.js'); runBullet()";
nestedShell("--js-cache", "--no-js-cache-per-process", "--execute=" + code);
setIonCheckGraphCoherency(false);
load(libdir + 'bullet.js');
var results = runBullet();
assertEq(results.asmJSValidated, true);
assertEq(results.loadedFromCache, true);
