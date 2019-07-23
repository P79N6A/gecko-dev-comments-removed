














































let kURIs = [
  "http://a.b.c/d-e_f/h/t/p",
  "http://d.e.f/g-h_i/h/t/p",
  "http://g.h.i/j-k_l/h/t/p",
  "http://j.k.l/m-n_o/h/t/p",
];
let kTitles = [
  "f(o)o b<a>r",
  "b(a)r b<a>z",
];


addPageBook(0, 0);
addPageBook(1, 1);

addPageBook(2, 0, 0);
addPageBook(3, 0, 1);




let gTests = [
  ["0: Match 2 terms all in url",
   "c d", [0]],
  ["1: Match 1 term in url and 1 term in title",
   "b e", [0,1]],
  ["2: Match 3 terms all in title; display bookmark title if matched",
   "b a z", [1,3]],
  ["3: Match 2 terms in url and 1 in title; make sure bookmark title is used for search",
   "k f t", [2]],
  ["4: Match 3 terms in url and 1 in title",
   "d i g z", [1]],
  ["5: Match nothing",
   "m o z i", []],
];
