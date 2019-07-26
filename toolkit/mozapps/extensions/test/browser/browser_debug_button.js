







let { Promise } = Components.utils.import("resource://gre/modules/Promise.jsm", {});
let { Task } = Components.utils.import("resource://gre/modules/Task.jsm", {});

const getDebugButton = node =>
    node.ownerDocument.getAnonymousElementByAttribute(node, "anonid", "debug-btn");
const addonDebuggingEnabled = bool =>
  Services.prefs.setBoolPref("devtools.chrome.enabled", !!bool);

Services.prefs.setBoolPref("devtools.debugger.show-server-notifications", false);


function test() {
  requestLongerTimeout(2);

  waitForExplicitFinish();


  var gProvider = new MockProvider();
  gProvider.createAddons([{
    id: "non-debuggable@tests.mozilla.org",
    name: "No debug",
    description: "foo"
  },
  {
    id: "debuggable@tests.mozilla.org",
    name: "Debuggable",
    description: "bar",
    isDebuggable: true
  }]);

  Task.spawn(function* () {
    addonDebuggingEnabled(false);

    yield testDOM((nondebug, debuggable) => {
      is(nondebug.disabled, true,
        "addon:disabled::remote:disabled button is disabled for legacy addons");
      is(nondebug.hidden, true,
        "addon:disabled::remote:disabled button is hidden for legacy addons");
      is(debuggable.disabled, true,
        "addon:disabled::remote:disabled button is disabled for debuggable addons");
      is(debuggable.hidden, true,
        "addon:disabled::remote:disabled button is hidden for debuggable addons");
    });
    
    addonDebuggingEnabled(true);

    yield testDOM((nondebug, debuggable) => {
      is(nondebug.disabled, true,
        "addon:enabled::remote:enabled button is disabled for legacy addons");
      is(nondebug.disabled, true,
        "addon:enabled::remote:enabled button is hidden for legacy addons");
      is(debuggable.disabled, false,
        "addon:enabled::remote:enabled button is enabled for debuggable addons");
      is(debuggable.hidden, false,
        "addon:enabled::remote:enabled button is visible for debuggable addons");
    });

    Services.prefs.clearUserPref("devtools.debugger.show-server-notifications");
    finish();
  });

  function testDOM (testCallback) {
    let deferred = Promise.defer();
    open_manager("addons://list/extension", function(aManager) {
      const {document} = aManager;
      const addonList = document.getElementById("addon-list");
      const nondebug = addonList.querySelector("[name='No debug']");
      const debuggable = addonList.querySelector("[name='Debuggable']");

      testCallback.apply(null, [nondebug, debuggable].map(getDebugButton));

      close_manager(aManager, deferred.resolve);
    });
    return deferred.promise;
  }
}
