














































let kURIs = [
  "http://example.com/",
  "http://example.com/container",
  
  "http://example.com/unvisited-not-bookmarked-elsewhere/vy-__--D",
  "http://example.com/visited-not-bookmarked-elsewhere/_0YlX-9L",
  "http://example.com/unvisited-bookmarked-elsewhere/X132H20w",
  "http://example.com/visited-bookmarked-elsewhere/n_6D_Pw5",
  
  "http://example.com/unvisited-not-bookmarked-elsewhere",
  "http://example.com/visited-not-bookmarked-elsewhere",
  "http://example.com/unvisited-bookmarked-elsewhere",
  "http://example.com/visited-bookmarked-elsewhere",
];
let kTitles = [
  "container title",
  
  "unvisited not-bookmarked-elsewhere child title",
  "visited   not-bookmarked-elsewhere child title",
  "unvisited bookmarked-elsewhere     child title",
  "visited   bookmarked-elsewhere     child title",
  
  "unvisited not-bookmarked-elsewhere child title P-13U-z-",
  "visited   not-bookmarked-elsewhere child title _3-X4_Qd",
  "unvisited bookmarked-elsewhere     child title I4jOt6o4",
  "visited   bookmarked-elsewhere     child title 9-RVT4D5",
];


addLivemark(0, 1, 0, 2, 1, null, true);
addLivemark(0, 1, 0, 3, 2, null, false);
addLivemark(0, 1, 0, 4, 3, null, true);
addPageBook(4, 3, 3, null, null, null, true);
addLivemark(0, 1, 0, 5, 4, null, false);


addLivemark(0, 1, 0, 6, 5, null, true);
addLivemark(0, 1, 0, 7, 6, null, false);
addLivemark(0, 1, 0, 8, 7, null, true);
addPageBook(8, 7, 7, null, null, null, true);
addLivemark(0, 1, 0, 9, 8, null, false);

let gTests = [
  
  ["0: unvisited not-bookmarked-elsewhere livemark child (should be empty)",
   "vy-__--D", []],
  ["1: visited not-bookmarked-elsewhere livemark child (should not be empty)",
   "_0YlX-9L", [3]],
  ["2: unvisited bookmarked-elsewhere livemark child (should not be empty)",
   "X132H20w", [4]],
  ["3: visited bookmarked-elsewhere livemark child (should not be empty)",
   "n_6D_Pw5", [5]],
  
  ["4: unvisited not-bookmarked-elsewhere livemark child (should be empty)",
   "P-13U-z-", []],
  ["5: visited not-bookmarked-elsewhere livemark child (should not be empty)",
   "_3-X4_Qd", [7]],
  ["6: unvisited bookmarked-elsewhere livemark child (should not be empty)",
   "I4jOt6o4", [8]],
  ["7: visited bookmarked-elsewhere livemark child (should not be empty)",
   "9-RVT4D5", [9]],
];
