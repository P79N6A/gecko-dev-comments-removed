







var nc = 30, maxCol = nc*3, cr,cg,cb;

load(libdir + "util.js");




function computeSetByRow(x, y) {
  var Cr = (x - 256) / scale + 0.407476;
  var Ci = (y - 256) / scale + 0.234204;
  var I = 0, R = 0, I2 = 0, R2 = 0;
  var n = 0;
  while ((R2+I2 < 2.0) && (n < 512)) {
    I = (R+R)*I+Ci;
    R = R2-I2+Cr;
    R2 = R*R;
    I2 = I*I;
    n++;
  }
  return n;
}

function computeSequentially() {
  result = [];
  for (var r = 0; r < rows; r++) {
    for (var c = 0; c < cols; c++) {
      result.push(computeSetByRow(c, r));
    }
  }
  return result;
}

function computeParallel() {
  return new ParallelArray([rows, cols], function(r, c) {
    return computeSetByRow(c, r);
  }).flatten();
}

function compare(arrs, pas) {
  for (var r = 0; r < rows; r++) {
    for (var c = 0; c < cols; c++) {
      assertEq(seq[c + r * cols], par.get(r, c));
    }
  }
}

var scale = 10000*300;
var rows = 1024;
var cols = 1024;


benchmark("MANDELBROT", 1, DEFAULT_MEASURE,
          computeSequentially, computeParallel);
