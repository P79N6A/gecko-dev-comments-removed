












































let kURIs = [
  "http://download/bookmarked",
  "http://embed/bookmarked",
  "http://framed/bookmarked",
  "http://download",
  "http://embed",
  "http://framed",
];
let kTitles = [
  "download-bookmark",
  "embed-bookmark",
  "framed-bookmark",
  "download2",
  "embed2",
  "framed2",
];


addPageBook(0, 0, 0, undefined, undefined, TRANSITION_DOWNLOAD);
addPageBook(1, 1, 1, undefined, undefined, TRANSITION_EMBED);
addPageBook(2, 2, 2, undefined, undefined, TRANSITION_FRAMED_LINK);
addPageBook(3, 3, undefined, undefined, undefined, TRANSITION_DOWNLOAD);
addPageBook(4, 4, undefined, undefined, undefined, TRANSITION_EMBED);
addPageBook(5, 5, undefined, undefined, undefined, TRANSITION_FRAMED_LINK);



let gTests = [
  ["0: Searching for bookmarked download uri matches",
   kTitles[0], [0]],
  ["1: Searching for bookmarked embed uri matches",
   kTitles[1], [1]],
  ["2: Searching for bookmarked framed uri matches",
   kTitles[2], [2]],
  ["3: Searching for download uri does not match",
   kTitles[3], []],
  ["4: Searching for embed uri does not match",
   kTitles[4], []],
  ["5: Searching for framed uri does not match",
   kTitles[5], []],
];
