



function printBugNumber (num) {
  BUGNUMBER = num;
}
gcslice(1)
schedulegc(this);
gcslice(2);
var BUGNUMBER = ("one");
printBugNumber();
