










































let kURIs = [
  "http://www.site/",
  "http://site/",
  "ftp://ftp.site/",
  "ftp://site/",
  "https://www.site/",
  "https://site/",
];
let kTitles = [
  "title",
];


addPageBook(0, 0);
addPageBook(1, 0);
addPageBook(2, 0);
addPageBook(3, 0);
addPageBook(4, 0);
addPageBook(5, 0);

let allSite = [0,1,2,3,4,5];
let allWWW = [0,4];



let gTests = [
  ["0: http://www. matches all www.site",
   kURIs[0], allWWW],
  ["1: http:// matches all site",
   kURIs[1], allSite],
  ["2: ftp://ftp. matches itself",
   kURIs[2], [2]],
  ["3: ftp:// matches all site",
   kURIs[3], allSite],
  ["4: https://www. matches all www.site",
   kURIs[4], allWWW],
  ["5: https:// matches all site",
   kURIs[5], allSite],

  ["6: www.site matches all www.site",
   "www.site", allWWW],
];
