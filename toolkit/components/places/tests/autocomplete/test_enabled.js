









































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
