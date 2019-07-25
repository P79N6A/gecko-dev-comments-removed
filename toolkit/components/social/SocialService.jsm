



const EXPORTED_SYMBOLS = ["SocialService"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/SocialProvider.jsm");

const MANIFEST_PREFS = Services.prefs.getBranch("social.manifest.");

let SocialServiceInternal = {};

XPCOMUtils.defineLazyGetter(SocialServiceInternal, "providers", function () {
  let providers = {};
  let prefs = MANIFEST_PREFS.getChildList("", {});
  prefs.forEach(function (pref) {
    try {
      var manifest = JSON.parse(MANIFEST_PREFS.getCharPref(pref));
      if (manifest && typeof(manifest) == "object") {
        let provider = new SocialProvider(manifest);
        providers[provider.origin] = provider;
      }
    } catch (err) {
      let msg = "SocialService: failed to load provider: " + pref +
                ", exception: " + err;
      Cu.reportError(msg);
      dump(msg);
    }
  }, this);

  return providers;
});

const SocialService = {
  getProvider: function getProvider(origin, onDone) {
    schedule((function () {
      onDone(SocialServiceInternal.providers[origin] || null);
    }).bind(this));
  },

  
  getProviderList: function getProviderList(onDone) {
    let providers = [p for each (p in SocialServiceInternal.providers)];
    schedule((function () {
      onDone(providers);
    }).bind(this));
  }
};

function schedule(callback) {
  Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
}
