

function test() {
    is(gBrowser.docShell.isActive, true, "Docshell should be active");
    window.minimize();
    is(gBrowser.docShell.isActive, false, "Docshell should be inactive");
    window.restore();
    is(gBrowser.docShell.isActive, true, "Docshell should be active again");
}
