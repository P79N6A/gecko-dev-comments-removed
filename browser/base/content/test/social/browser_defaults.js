
let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;



function test() {
  let manifestPrefs = Services.prefs.getDefaultBranch("social.manifest.");
  let prefs = manifestPrefs.getChildList("", []);
  ok(prefs.length > 0, "we have builtin providers");
  for (let pref of prefs) {
    let manifest = JSON.parse(manifestPrefs.getComplexValue(pref, Ci.nsISupportsString).data);
    ok(manifest.builtin, "manifest is builtin " + manifest.origin);
  }
}
