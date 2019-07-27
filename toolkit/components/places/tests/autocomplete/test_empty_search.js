









let kURIs = [
  "http://foo/0",
  "http://foo/1",
  "http://foo/2",
  "http://foo/3",
  "http://foo/4",
  "http://foo/5",
];
let kTitles = [
  "title",
];


addPageBook(0, 0); 
addPageBook(1, 0, 0); 
addPageBook(2, 0); 
addPageBook(3, 0, 0); 


addPageBook(4, 0, 0); 
addPageBook(5, 0, 0); 


markTyped([2,3,5], 0);

removePages([4,5]);



let gTests = [
  ["0: Match everything",
   "foo", [0,1,2,3,4,5]],
  ["1: Match only typed history",
   "foo ^ ~", [2,3]],
  ["2: Drop-down empty search matches only typed history",
   "", [2,3]],
  ["3: Drop-down empty search matches only bookmarks",
   "", [2,3], matchBookmarks],
  ["4: Drop-down empty search matches only typed",
   "", [2,3], matchTyped],
];

function matchBookmarks() {
  prefs.setBoolPref("browser.urlbar.suggest.history", false);
  prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  clearPrefs();
}

function matchTyped() {
  prefs.setBoolPref("browser.urlbar.suggest.history", true);
  prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", true);
  clearPrefs();
}

function clearPrefs() {
  prefs.clearUserPref("browser.urlbar.suggest.history");
  prefs.clearUserPref("browser.urlbar.suggest.bookmark");
  prefs.clearUserPref("browser.urlbar.suggest.history.onlyTyped");
}
