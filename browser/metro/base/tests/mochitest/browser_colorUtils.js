




"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "iconColorCache purges stale entries",
  setUp: function() {
    Components.utils.import("resource:///modules/colorUtils.jsm", this);
    let dayMs = 86400000;
    let ColorUtils = this.ColorUtils;
    this.ColorUtils.init();

    
    ColorUtils.iconColorCache.set('http://test.old/favicon.ico', {
      "foreground":"rgb(255,255,255)",
      "background":"rgb(78,78,84)",
      "timestamp":Date.now() - (2*dayMs)
    });
    
    ColorUtils.iconColorCache.set('http://test.new/favicon.ico', {
      "foreground":"rgb(255,255,255)",
      "background":"rgb(78,78,84)",
      "timestamp":Date.now()
    });
  },
  run: function testIconColorCachePurge() {
    let dayMs = 86400000;
    let ColorUtils = this.ColorUtils;
    let cachePurgeSpy = spyOnMethod(ColorUtils.iconColorCache, 'purge');

    
    Services.obs.notifyObservers(null, "idle-daily", dayMs);
    is(cachePurgeSpy.callCount, 1);
    cachePurgeSpy.restore();

    ok(ColorUtils.iconColorCache.has('http://test.new/favicon.ico'),
       "fresh cache entry was not removed in the purge");

    ok(!ColorUtils.iconColorCache.has('http://test.old/favicon.ico'),
       "stale cache entry was removed in the purge");
  }
});
