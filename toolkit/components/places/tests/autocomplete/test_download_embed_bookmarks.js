












































let kURIs = [
  "http://download/bookmarked",
  "http://embed/bookmarked",
  "http://download",
  "http://embed",
];
let kTitles = [
  "download-bookmark",
  "embed-bookmark",
  "download2",
  "embed2",
];


addPageBook(0, 0, 0, undefined, undefined, TRANSITION_DOWNLOAD);
addPageBook(1, 1, 1, undefined, undefined, TRANSITION_EMBED);
addPageBook(2, 2, undefined, undefined, undefined, TRANSITION_DOWNLOAD);
addPageBook(3, 3, undefined, undefined, undefined, TRANSITION_EMBED);



let gTests = [
  ["0: Searching for bookmarked download uri matches",
   kTitles[0], [0]],
  ["1: Searching for bookmarked embed uri matches",
   kTitles[1], [1]],
  ["2: Searching for download uri does not match",
   kTitles[2], []],
  ["3: Searching for embed uri does not match",
   kTitles[3], []],
];
