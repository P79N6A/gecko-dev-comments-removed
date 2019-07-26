





function test() {
  waitForExplicitFinish();

  Services.prefs.setBoolPref(PREF_STRICT_COMPAT, true);
  ok(AddonManager.strictCompatibility, "Strict compatibility should be enabled");

  AddonManager.getAllAddons(function gAACallback(aAddons) {
    
    aAddons.sort(function compareTypeName(a, b) {
      return a.type.localeCompare(b.type) || a.name.localeCompare(b.name);
    });

    let allCompatible = true;
    for (let a of aAddons) {
      
      if (a.type == "plugin")
        continue;

      ok(a.isCompatible, a.type + " " + a.name + " " + a.version + " should be compatible");
      allCompatible = allCompatible && a.isCompatible;
    }
    
    if (!allCompatible)
      ok(false, "As this test failed, test browser_bug557956.js should have failed, too.");

    finish();
  });
}
