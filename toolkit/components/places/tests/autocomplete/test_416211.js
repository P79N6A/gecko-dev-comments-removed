








































let theTag = "superTag";


let kURIs = [
  "http://theuri/",
];
let kTitles = [
  "Page title",
  "Bookmark title",
  theTag,
];


addPageBook(0, 0, 1, [2]);




let gTests = [
  ["0: Make sure the tag match gives the bookmark title",
   theTag, [0]],
];
