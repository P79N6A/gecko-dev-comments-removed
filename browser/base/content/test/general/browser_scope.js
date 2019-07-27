




thisTestLeaksUncaughtRejectionsAndShouldBeFixed("TypeError: this.docShell is null");

function test() {
  ok(!!gBrowser, "gBrowser exists");
  is(gBrowser, getBrowser(), "both ways of getting tabbrowser work");
}
