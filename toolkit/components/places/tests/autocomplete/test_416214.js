
















































let theTag = "superTag";


let kURIs = [
  "http://escaped/ユニコード",
  "http://asciiescaped/blocking-firefox3%2B",
];
let kTitles = [
  "title",
  theTag,
];


addPageBook(0, 0, 0, [1]);
addPageBook(1, 0, 0, [1]);




let gTests = [
  ["0: Make sure tag matches return the right url as well as '+' remain escaped",
   theTag, [0,1]],
];
