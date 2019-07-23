









































let kURIs = [
  "http://unescapeduri/",
  "http://escapeduri/%40/",
];
let kTitles = [
  "title",
];


addPageBook(0, 0);
addPageBook(1, 0);



let gTests = [
  ["0: Unescaped location matches itself",
   kURIs[0], [0]],
  ["1: Escaped location matches itself",
   kURIs[1], [1]],
];
