







"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>browser console filters";
const WEB_CONSOLE_PREFIX = "devtools.webconsole.filter.";
const BROWSER_CONSOLE_PREFIX = "devtools.browserconsole.filter.";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);

  info("open the web console");
  let hud = yield openConsole();
  ok(hud, "web console opened");

  is(Services.prefs.getBoolPref(BROWSER_CONSOLE_PREFIX + "exception"), true,
     "'exception' filter is enabled (browser console)");
  is(Services.prefs.getBoolPref(WEB_CONSOLE_PREFIX + "exception"), true,
     "'exception' filter is enabled (web console)");

  info("toggle 'exception' filter");
  hud.setFilterState("exception", false);

  is(Services.prefs.getBoolPref(BROWSER_CONSOLE_PREFIX + "exception"), true,
     "'exception' filter is enabled (browser console)");
  is(Services.prefs.getBoolPref(WEB_CONSOLE_PREFIX + "exception"), false,
     "'exception' filter is disabled (web console)");

  hud.setFilterState("exception", true);

  
  let deferred = promise.defer();
  executeSoon(() => closeConsole().then(() => deferred.resolve(null)));
  yield deferred.promise;

  info("web console closed");
  hud = yield HUDService.toggleBrowserConsole();
  ok(hud, "browser console opened");

  is(Services.prefs.getBoolPref(BROWSER_CONSOLE_PREFIX + "exception"), true,
     "'exception' filter is enabled (browser console)");
  is(Services.prefs.getBoolPref(WEB_CONSOLE_PREFIX + "exception"), true,
     "'exception' filter is enabled (web console)");

  info("toggle 'exception' filter");
  hud.setFilterState("exception", false);

  is(Services.prefs.getBoolPref(BROWSER_CONSOLE_PREFIX + "exception"), false,
     "'exception' filter is disabled (browser console)");
  is(Services.prefs.getBoolPref(WEB_CONSOLE_PREFIX + "exception"), true,
     "'exception' filter is enabled (web console)");

  hud.setFilterState("exception", true);
});
