



function printBugNumber (num) {
  BUGNUMBER = num;
}
gcslice(0)
schedulegc(this);
gcslice(1);
var BUGNUMBER = ("one");
printBugNumber();
