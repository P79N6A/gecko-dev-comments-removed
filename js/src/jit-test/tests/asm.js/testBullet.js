



if (!isAsmJSCompilationAvailable())
    quit();





var code = "setIonCheckGraphCoherency(false); load('" + libdir + "bullet.js'); runBullet()";
nestedShell("--js-cache", "--execute=" + code);
setIonCheckGraphCoherency(false);
load(libdir + 'bullet.js');
var results = runBullet();
assertEq(results.asmJSValidated, true);
assertEq(results.loadedFromCache, true);
