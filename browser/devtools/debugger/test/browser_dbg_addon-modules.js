




const ADDON_URL = EXAMPLE_URL + "addon4.xpi";

function test() {
  Task.spawn(function () {
    let addon = yield addAddon(ADDON_URL);
    let addonDebugger = yield initAddonDebugger(ADDON_URL);

    is(addonDebugger.title, "Debugger - Test add-on with JS Modules", "Saw the right toolbox title.");

    
    let groups = yield addonDebugger.getSourceGroups();
    is(groups[0].name, "browser_dbg_addon4@tests.mozilla.org", "Add-on code should be the first group");
    is(groups.length, 1, "Should be only one group.");

    let sources = groups[0].sources;
    is(sources.length, 2, "Should be two sources");
    ok(sources[0].url.endsWith("/browser_dbg_addon4@tests.mozilla.org.xpi!/bootstrap.js"), "correct url for bootstrap code")
    is(sources[0].label, "bootstrap.js", "correct label for bootstrap code")
    is(sources[1].url, "resource://browser_dbg_addon4/test.jsm", "correct url for addon code")
    is(sources[1].label, "test.jsm", "correct label for addon code")

    
    Cu.import("resource://browser_dbg_addon4/test2.jsm", {});

    groups = yield addonDebugger.getSourceGroups();
    is(groups[0].name, "browser_dbg_addon4@tests.mozilla.org", "Add-on code should be the first group");
    is(groups.length, 1, "Should be only one group.");

    sources = groups[0].sources;
    is(sources.length, 3, "Should be three sources");
    ok(sources[0].url.endsWith("/browser_dbg_addon4@tests.mozilla.org.xpi!/bootstrap.js"), "correct url for bootstrap code")
    is(sources[0].label, "bootstrap.js", "correct label for bootstrap code")
    is(sources[1].url, "resource://browser_dbg_addon4/test.jsm", "correct url for addon code")
    is(sources[1].label, "test.jsm", "correct label for addon code")
    is(sources[2].url, "resource://browser_dbg_addon4/test2.jsm", "correct url for addon code")
    is(sources[2].label, "test2.jsm", "correct label for addon code")

    Cu.unload("resource://browser_dbg_addon4/test2.jsm");
    yield addonDebugger.destroy();
    yield removeAddon(addon);
    finish();
  });
}
