










var x = "1";
if (--x !== 1 - 1) {
  $ERROR('#1: var x = "1"; --x === 1 - 1. Actual: ' + (--x));
}


var x = "x";
if (isNaN(--x) !== true) {
  $ERROR('#2: var x = "x"; --x === Not-a-Number. Actual: ' + (--x));
}


var x = new String("-1"); 
if (--x !== -1 - 1) {
  $ERROR('#3: var x = new String("-1"); --x === -1 - 1. Actual: ' + (--x));
}

