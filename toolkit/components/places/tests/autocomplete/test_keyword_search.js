














































let keyBase = "http://abc/?search=";
let keyKey = "key";


let otherBase = "http://xyz/?foo=";

let unescaped = "ユニコード";
let pageInHistory = "ThisPageIsInHistory";


let kURIs = [
  keyBase + "%s",
  keyBase + "term",
  keyBase + "multi+word",
  keyBase + "blocking%2B",
  keyBase + unescaped,
  keyBase + pageInHistory,
  keyBase,
  otherBase + "%s",
  keyBase + "twoKey",
  otherBase + "twoKey"
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
gPages[6] = [6,1];



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
  ["5: Keyword without query (without space)",
   keyKey, [6]],
  ["6: Keyword without query (with space)",
   keyKey + " ", [6]],

  
  ["7: Two keywords matched",
   keyKey + " twoKey", [8,9],
   function() {
     
     addPageBook(7, 0, 1, [], keyKey);
     gPages[8] = [8,1];
     gPages[9] = [9,1];
   }]
];
