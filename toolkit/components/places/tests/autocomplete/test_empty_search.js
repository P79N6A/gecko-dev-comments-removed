









































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


markTyped([2,3,5]);

removePages([4,5]);



let gTests = [
  ["0: Match everything",
   "foo", [0,1,2,3,4,5]],
  ["1: Match only typed history",
   "foo ^ ~", [2,3]],
  ["2: Drop-down empty search matches only typed history",
   "", [2,3]],
  ["3: Drop-down empty search matches everything",
   "", [0,1,2,3,4,5], function () setEmptyPref(0)],
  ["4: Drop-down empty search matches only typed",
   "", [2,3,5], function () setEmptyPref(32)],
  ["5: Drop-down empty search matches only typed history",
   "", [2,3], clearEmptyPref],
];

function setEmptyPref(aValue)
  prefs.setIntPref("browser.urlbar.default.behavior.emptyRestriction", aValue);

function clearEmptyPref()
  prefs.clearUserPref("browser.urlbar.default.behavior.emptyRestriction");
