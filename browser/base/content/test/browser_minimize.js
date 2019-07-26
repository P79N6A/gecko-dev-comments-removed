

function waitForActive() {
    if (!gBrowser.docShell.isActive) {
        executeSoon(waitForActive);
        return;
    }
    is(gBrowser.docShell.isActive, true, "Docshell should be active again");
    finish();
}

function waitForInactive() {
    if (gBrowser.docShell.isActive) {
        executeSoon(waitForInactive);
        return;
    }
    is(gBrowser.docShell.isActive, false, "Docshell should be inactive");
    window.restore();
    waitForActive();
}

function test() {
    registerCleanupFunction(function() {
      window.restore();
    });

    waitForExplicitFinish();
    is(gBrowser.docShell.isActive, true, "Docshell should be active");
    window.minimize();
    
    
    
    
    
    
    waitForInactive();
}
