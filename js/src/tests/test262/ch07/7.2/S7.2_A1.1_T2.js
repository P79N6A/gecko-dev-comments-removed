










	var  x	=	1	;
if (x !== 1) {
  $ERROR('#1: 	var	x	=	1	; x === 1. Actual: ' + (x));
}


eval("	var\tx	=\t2	");
if (x !== 2) {
  $ERROR('#2: 	var\\tx	=\\t1	; x === 2. Actual: ' + (x));
}

