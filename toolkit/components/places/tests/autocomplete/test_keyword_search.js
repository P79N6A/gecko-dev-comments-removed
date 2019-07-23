











































let keyBase = "http://abc/?search=";
let keyKey = "key";

let unescaped = "ユニコード";
let pageInHistory = "ThisPageIsInHistory";


let kURIs = [
  keyBase + "%s",
  keyBase + "term",
  keyBase + "multi+word",
  keyBase + "blocking%2B",
  keyBase + unescaped,
  keyBase + pageInHistory,
];
let kTitles = [
  "Generic page title",
  "Keyword title",
];


addPageBook(0, 0, 1, [], keyKey);

gPages[1] = [1,1];
gPages[2] = [2,1];
gPages[3] = [3,1];
gPages[4] = [4,1];

addPageBook(5, 0);



let gTests = [
  ["0: Plain keyword query",
   keyKey + " term", [1]],
  ["1: Multi-word keyword query",
   keyKey + " multi word", [2]],
  ["2: Keyword query with +",
   keyKey + " blocking+", [3]],
  ["3: Unescaped term in query",
   keyKey + " " + unescaped, [4]],
  ["4: Keyword that happens to match a page",
   keyKey + " " + pageInHistory, [5]],
];
