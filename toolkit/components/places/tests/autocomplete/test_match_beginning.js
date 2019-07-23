









































let kURIs = [
  "http://x.com/y",
  "https://y.com/x",
];
let kTitles = [
  "a b",
  "b a",
];

addPageBook(0, 0);
addPageBook(1, 1);



let gTests = [
  
  ["0: Match at the beginning of titles",
   "a", [0],
   function() setBehavior(3)],
  ["1: Match at the beginning of titles",
   "b", [1]],
  ["2: Match at the beginning of urls",
   "x", [0]],
  ["3: Match at the beginning of urls",
   "y", [1]],
  
  
  ["4: Sanity check that matching anywhere finds more",
   "a", [0,1],
   function() setBehavior(1)],
];

function setBehavior(aType) {
  prefs.setIntPref("browser.urlbar.matchBehavior", aType);
}
