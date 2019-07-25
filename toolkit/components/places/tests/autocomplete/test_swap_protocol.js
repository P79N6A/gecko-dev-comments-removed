













































let kURIs = [
  "http://www.site/",
  "http://site/",
  "ftp://ftp.site/",
  "ftp://site/",
  "https://www.site/",
  "https://site/",
  "http://woohoo/",
  "http://wwwwwwacko/",
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
addPageBook(6, 0);
addPageBook(7, 0);

let allSite = [0,1,2,3,4,5];



let gTests = [
  ["0: http://www.site matches all site", "http://www.site", allSite],
  ["1: http://site matches all site", "http://site", allSite],
  ["2: ftp://ftp.site matches itself", "ftp://ftp.site", [2]],
  ["3: ftp://site matches all site", "ftp://site", allSite],
  ["4: https://www.site matches all site", "https://www.site", allSite],
  ["5: https://site matches all site", "https://site", allSite],
  ["6: www.site matches all site", "www.site", allSite],

  ["7: w matches none of www.", "w", [6,7]],
  ["8: http://w matches none of www.", "w", [6,7]],
  ["9: http://www.w matches none of www.", "w", [6,7]],

  ["10: ww matches none of www.", "ww", [7]],
  ["11: http://ww matches none of www.", "http://ww", [7]],
  ["12: http://www.ww matches none of www.", "http://www.ww", [7]],

  ["13: www matches none of www.", "www", [7]],
  ["14: http://www matches none of www.", "http://www", [7]],
  ["15: http://www.www matches none of www.", "http://www.www", [7]],
];
