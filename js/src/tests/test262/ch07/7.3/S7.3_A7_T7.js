










var y=2;
var z=3;
var
x
=
y
<<
z
;
if (x !== 16) {
  $ERROR('#1: var\\nx\\n=\\ny\\n<<\\nz\\n; x === 16. Actual: ' + (x));
}
x=0;


var y=2;
var z=3;
var
x
=
y
<<
z
;
if (x !== 16) {
  $ERROR('#2: var\\nx\\n=\\ny\\n<<\\nz\\n; x ===16 ');
}
x=0;


var y=2;
var z=3;
eval("\u2028var\u2028x\u2028=\u2028y\u2028<<\u2028z\u2028");
if (x !== 16) {
  $ERROR('#3: eval("\\u2028var\\u2028x\\u2028=\\u2028y\\u2028<<\\u2028z\\u2028"); x === 16. Actual: ' + (x));
}
x=0;


var y=2;
var z=3;
eval("\u2029var\u2029x\u2029=\u2029y\u2029<<\u2029z\u2029");
if (x !== 16) {
  $ERROR('#4: eval("\\u2029var\\u2029x\\u2029=\\u2029y\\u2029<<\\u2029z\\u2029"); x === 16. Actual: ' + (x));
}

