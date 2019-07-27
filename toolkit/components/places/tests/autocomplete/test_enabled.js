









let kURIs = [
  "http://url/0",
];
let kTitles = [
  "title",
];

addPageBook(0, 0); 



let gTests = [
  ["1: plain search",
   "url", [0]],
  ["2: search disabled",
   "url", [], function() setSearch(0)],
  ["3: resume normal search",
   "url", [0], function() setSearch(1)],
];

function setSearch(aSearch) {
  prefs.setBoolPref("browser.urlbar.autocomplete.enabled", !!aSearch);
}

add_task(function* test_sync_enabled() {
  
  Cc["@mozilla.org/autocomplete/search;1?name=history"]
    .getService(Ci.mozIPlacesAutoComplete);

  let types = [ "history", "bookmark", "openpage" ];

  
  
  for (let type of types) {
    Services.prefs.setBoolPref("browser.urlbar.suggest." + type, true);
  }
  Assert.equal(Services.prefs.getBoolPref("browser.urlbar.autocomplete.enabled"), true);

  
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", false);
  for (let type of types) {
    Assert.equal(Services.prefs.getBoolPref("browser.urlbar.suggest." + type), false);
  }

  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
  for (let type of types.filter(t => t != "history")) {
    Assert.equal(Services.prefs.getBoolPref("browser.urlbar.suggest." + type), false);
  }
  Assert.equal(Services.prefs.getBoolPref("browser.urlbar.autocomplete.enabled"), true);

  
  
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", false);
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", true);
  for (let type of types.filter(t => t != "history")) {
    Assert.equal(Services.prefs.getBoolPref("browser.urlbar.suggest." + type), true);
  }
});
