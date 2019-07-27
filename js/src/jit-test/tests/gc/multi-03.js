

var g1 = newGlobal();
var g2 = newGlobal();

schedulegc(g1);
schedulegc(g2);
gcslice(0);
schedulegc(g1);
gcslice(1);
gcslice();
