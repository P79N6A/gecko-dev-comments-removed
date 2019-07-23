















































let katakana = ["\u30a8", "\u30c9"]; 
let ideograph = ["\u4efb", "\u5929", "\u5802"]; 


let kURIs = [
  "http://matchme/",
  "http://dontmatchme/",
  "http://title/1",
  "http://title/2",
  "http://tag/1",
  "http://tag/2",
  "http://crazytitle/",
  "http://katakana/",
  "http://ideograph/",
  "http://camel/pleaseMatchMe/",
];
let kTitles = [
  "title1",
  "matchme2",
  "dontmatchme3",
  "!@#$%^&*()_+{}|:<>?word",
  katakana.join(""),
  ideograph.join(""),
];


addPageBook(0, 0);
addPageBook(1, 0);

addPageBook(2, 1);
addPageBook(3, 2);

addPageBook(4, 0, 0, [1]);
addPageBook(5, 0, 0, [2]);

addPageBook(6, 3);

addPageBook(7, 4);

addPageBook(8, 5);

addPageBook(9, 0);



let gTests = [
  
  ["0: Match 'match' at the beginning or after / or on a CamelCase",
   "match", [0,2,4,9],
   function() setBehavior(2)],
  ["1: Match 'dont' at the beginning or after /",
   "dont", [1,3,5]],
  ["2: Match '2' after the slash and after a word (in tags too)",
   "2", [2,3,4,5]],
  ["3: Match 't' at the beginning or after /",
   "t", [0,1,2,3,4,5,9]],
  ["4: Match 'word' after many consecutive word boundaries",
   "word", [6]],
  ["5: Match a word boundary '/' for everything",
   "/", [0,1,2,3,4,5,6,7,8,9]],
  ["6: Match word boundaries '()_+' that are among word boundaries",
   "()_+", [6]],

  ["7: Katakana characters form a string, so match the beginning",
   katakana[0], [7]],
  


  ["9: Ideographs are treated as words so 'nin' is one word",
   ideograph[0], [8]],
  ["10: Ideographs are treated as words so 'ten' is another word",
   ideograph[1], [8]],
  ["11: Ideographs are treated as words so 'do' is yet another",
   ideograph[2], [8]],

  ["12: Extra negative assert that we don't match in the middle",
   "ch", []],
  ["13: Don't match one character after a camel-case word boundary (bug 429498)",
   "atch", []],

  
  ["14: Match on word boundaries as well as anywhere (bug 429531)",
   "tch", [0,1,2,3,4,5,9],
   function() setBehavior(1)],
];

function setBehavior(aType) {
  prefs.setIntPref("browser.urlbar.matchBehavior", aType);
}
