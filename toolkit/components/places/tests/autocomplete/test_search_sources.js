









































let kURIs = [
  "http://url/0",
  "http://url/1",
  "http://url/2",
];
let kTitles = [
  "title",
];

addPageBook(0, 0); 
addPageBook(1, 0, 0); 
addPageBook(2, 0, 0); 


histsvc.removePage(toURI(kURIs[2]));



let gTests = [
  
  ["1: search none",
   "url", [], function() setSearch(0)],
  ["2: search history",
   "url", [0,1], function() setSearch(1)],
  ["3: search bookmark",
   "url", [1,2], function() setSearch(2)],
  ["4: search both",
   "url", [0,1,2], function() setSearch(3)],
];

function setSearch(aSearch) {
  prefs.setIntPref("browser.urlbar.search.sources", aSearch);
}
