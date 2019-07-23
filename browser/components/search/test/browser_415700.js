


































var gSS = Cc["@mozilla.org/browser/search-service;1"].
           getService(Ci.nsIBrowserSearchService);
var gObs = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);

function observers(aSubject, aTopic, aData) {
  switch (aData) {
    case "engine-added":
      test2();
      break;
    case "engine-current":
      test3();
      break;
    case "engine-removed":
      test4();
      break;
  }
}

function test() {
  waitForExplicitFinish();
  gObs.addObserver(observers, "browser-search-engine-modified", false);

  gSS.addEngine("http://localhost:8888/browser/browser/components/search/test/testEngine.xml",
                Ci.nsISearchEngine.DATA_XML, "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAABGklEQVQoz2NgGB6AnZ1dUlJSXl4eSDIyMhLW4Ovr%2B%2Fr168uXL69Zs4YoG%2BLi4i5dusTExMTGxsbNzd3f37937976%2BnpmZmagbHR09J49e5YvX66kpATVEBYW9ubNm2nTphkbG7e2tp44cQLIuHfvXm5urpaWFlDKysqqu7v73LlzECMYIiIiHj58mJCQoKKicvXq1bS0NKBgW1vbjh074uPjgeqAXE1NzSdPnvDz84M0AEUvXLgAsW379u1z5swBen3jxo2zZ892cHB4%2BvQp0KlAfwI1cHJyghQFBwfv2rULokFXV%2FfixYu7d%2B8GGqGgoMDKyrpu3br9%2B%2FcDuXl5eVA%2FAEWBfoWHAdAYoNuAYQ0XAeoUERFhGDYAAPoUaT2dfWJuAAAAAElFTkSuQmCC",
                false);
}

function test2() {
  var engine = gSS.getEngineByName("Foo");
  ok(engine, "Engine was added.");

  var aEngine = gSS.getEngineByAlias("fooalias");
  ok(!aEngine, "Alias was not parsed from engine description");

  gSS.currentEngine = engine;
}

function test3() {
  var engine = gSS.currentEngine;
  is(engine.name, "Foo", "Current engine was changed successfully");

  gSS.removeEngine(engine);
}

function test4() {
  var engine = gSS.currentEngine;
  ok(engine, "An engine is present.");
  isnot(engine.name, "Foo", "Current engine reset after removal");

  gObs.removeObserver(observers, "browser-search-engine-modified");
  finish();
}
