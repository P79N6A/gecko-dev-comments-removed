










































let kURIs = [
  "http://page1",
  "http://page2",
  "http://page3",
  "http://page4",
];
let kTitles = [
  "tag1",
  "tag2",
  "tag3",
];


addPageBook(0, 0, 0, [0]);
addPageBook(1, 0, 0, [0,1]);
addPageBook(2, 0, 0, [0,2]);
addPageBook(3, 0, 0, [0,1,2]);



let gTests = [
  ["0: Make sure tags come back in the title when matching tags",
   "page1 tag", [0]],
  ["1: Check tags in title for page2",
   "page2 tag", [1]],
  ["2: Make sure tags appear even when not matching the tag",
   "page3", [2]],
  ["3: Multiple tags come in commas for page4",
   "page4", [3]],
  ["4: Extra test just to make sure we match the title",
   "tag2", [1,3]],
];
