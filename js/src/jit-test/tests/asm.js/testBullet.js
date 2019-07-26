



if (!isAsmJSCompilationAvailable())
    quit();





var code = "setIonAssertGraphCoherency(false); load('" + libdir + "bullet.js'); runBullet()";
nestedShell("--js-cache", "--execute=" + code);
setIonAssertGraphCoherency(false);
load(libdir + 'bullet.js');
var results = runBullet();
assertEq(results.asmJSValidated, true);
assertEq(results.loadedFromCache, true);
