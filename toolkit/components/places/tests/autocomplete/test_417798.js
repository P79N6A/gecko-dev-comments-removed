









































let kURIs = [
  "http://abc/def",
  "javascript:5",
];
let kTitles = [
  "Title with javascript:",
];

addPageBook(0, 0); 

addPageBook(1, 0, 0, undefined, undefined, undefined, true);



let gTests = [
  ["0: Match non-javascript: with plain search",
   "a", [0]],
  ["1: Match non-javascript: with almost javascript:",
   "javascript", [0]],
  ["2: Match javascript:",
   "javascript:", [0,1]],
  ["3: Match nothing with non-first javascript:",
   "5 javascript:", []],
  ["4: Match javascript: with multi-word search",
   "javascript: 5", [1]],
];
