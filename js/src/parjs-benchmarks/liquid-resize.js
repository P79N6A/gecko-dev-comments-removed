











  
var benchmarking = (typeof(MODE) != "undefined");


if (typeof(libdir) == "undefined") {
  print("Selecting default libdir of './';");
  print("you should override if you are not running from current directory.");
  var libdir = "./";
}

if (benchmarking) {
  
  load(libdir + "util.js");
}


load(libdir + "rectarray.js");



























var tinyImage =
  WrapArray.build(20, 5,
    function(x, y, k) {
      var ret;
      if (6 <= x && x < 8 && 0 <= y && y < 4)
        ret = ".";
      else if ((x-15)*(x-15)+(y-1)*(y-1) < 2)
        ret = "^";
      else if ((x-20)*(x-20)+(y-3)*(y-3) < 2)
        ret = "%";
      else if ((x-1)*(x-1)+(y-3)*(y-3) < 2)
        ret = "@";
      else
        ret = " ";
      return ret.charCodeAt(0) - 32;
    });

var smallImage =
  WrapArray.build(60, 15,
    function(x, y, k) {
      var ret;
      if (6 <= x && x < 8 && 0 <= y && y < 7)
        ret = ".";
      else if ((x-15)*(x-15)+(y-1)*(y-1) < 2)
        ret = "^";
      else if ((x-40)*(x-40)+(y-6)*(y-6) < 6)
        ret = "%";
      else if ((x-1)*(x-1)+(y-12)*(y-12) < 2)
        ret = "@";
      else
        ret = " ";
      return ret.charCodeAt(0) - 32;
    });

var bigImage =
  WrapArray.build(200, 70,
    function(x, y, k) {
      var ret;
      if (4 <= x && x < 7 && 10 <= y && y < 40)
        ret = ".";
      else if ((x-150)*(x-150)+(y-13)*(y-13) < 70)
        ret = "^";
      else if ((x-201)*(x-201)+(y-33)*(y-33) < 200)
        ret = "%";
      else if ((x-15)*(x-15)+(y-3)*(y-3) < 7)
        ret = "@";
      else
        ret = " ";
      return ret.charCodeAt(0) - 32;
    });


function randomImage(w, h, sparsity, variety) {
  return WrapArray.build(w, h, function (x,y) {
      if (Math.random() > 1/sparsity)
        return 0;
      else
      return 1+Math.random()*variety|0;
  });
}


function stripedImage(w, h) {
  return WrapArray.build(w, h,
                         function (x, y) (Math.abs(x%100-y%100) < 10) ? 32 : 0);
}

var massiveImage =
  WrapArray.build(70, 10000, function(x,y) (Math.abs(x%100-y%100) < 10) ? 32 : 0);


WrapArray.prototype.asciiart = function asciiart() {
  return this.map(function (x) String.fromCharCode(x+32));
};


WrapArray.prototype.row = function row(i) {
  return this.slice(i*this.width*this.payload, (i+1)*this.width*this.payload);
};


WrapArray.prototype.render = function render() {
  var art = this.asciiart();
  var a = new Array(art.height);
  for (var i=0; i < art.height; i++) {
    a[i] = art.row(i);
  }
  return a.map(function(r) r.join("")).join("\n");
};


WrapArray.prototype.print =
  (function locals() { var pr = print;
      return function print() pr(this.render()); })();


Array.prototype.toRectArray = function toRectArray(w,h) {
  var p = this;
  return WrapArray.build(w, h, function (i, j) p[i+j*w]);
};

WrapArray.prototype.toRectArray = function toRectArray(w,h) {
  var p = this;
  return WrapArray.build(w, h, function (i, j) p.get(i, j));
};


ParallelArray.prototype.toRectArray = function toRectArray(w, h) {
  var p = this;
  if (h == undefined) h = p.shape[1];
  if (w == undefined) w = p.shape[0];
  if (p.shape.length == 2)
    return WrapArray.build(w, h, function (i,j) p.get(i,j));
  if (p.shape.length == 1)
    return WrapArray.build(w, h, function (i,j) p.get(i + j*w));
};


WrapArray.prototype.toParallelArray = function toParallelArray(mode) {
  var r = this;
  var w = this.width;
  var h = this.height;

  if (false) {
    
    return new ParallelArray([w,h], function (i,j) r.get(i,j), mode);
  } else {
    
    
    var b = r.backingArray;
    return new ParallelArray([w,h], function (i,j) b[i+w*j], mode);
  }
};


WrapArray.prototype.transpose =
  function transpose() {
    var r = this;
    var b = r.backingArray;
    var w = r.width;
    return WrapArray.buildN(r.height, r.width, r.payload,
                            function(x, y, k)
                              
                              b[y+w*x]
                           );
  };


WrapArray.prototype.transposeParallelArray =
  function transpose(mode) {
    var r = this;
    var b = r.backingArray;
    var w = r.width;
    return new ParallelArray([r.height, r.width], function(x, y) b[y+w*x], mode);
  };

ParallelArray.prototype.transpose =
  function transpose(mode) {
    var p = this;
    var w = this.shape[0];
    var h = this.shape[1];
    return new ParallelArray([h,w], function (i,j) p.get(j,i), mode);
  };








function detectEdgesSeq_naive(ra) {
  var sobelX = [[-1.0,  0.0, 1.0],
                [-2.0, 0.0, 2.0],
                [-1.0, 0.0, 1.0]];
  var sobelY = [[1.0,  2.0, 1.0],
                [0.0, 0.0, 0.0],
                [-1.0, -2.0, -1.0]];

  var width = ra.width;
  var height = ra.height;

  var abs = function(x) (x < 0) ? -x : x;
  return WrapArray.build(width, height,
    
    function (x,y)
    {
      
      var totalX = 0;
      var totalY = 0;
      for (var offY = -1; offY <= 1; offY++) {
        var newY = y + offY;
        for (var offX = -1; offX <= 1; offX++) {
          var newX = x + offX;
          if ((newX >= 0) && (newX < width) && (newY >= 0) && (newY < height)) {
            var pointIndex = (x + offX + (y + offY) * width);
            var e = ra.get(x + offX, y + offY);
            totalX += e * sobelX[offY + 1][offX + 1];
            totalY += e * sobelY[offY + 1][offX + 1];
          }
        }
      }
      var total = (abs(totalX) + abs(totalY))/8.0 | 0;
      return total;
    });
}



function detectEdgesSeq_array_wh(data, width, height) {
    var data1 = new Array(width*height);
    var sobelX =  [[-1.0,  0.0, 1.0],
                    [-2.0, 0.0, 2.0],
                    [-1.0, 0.0, 1.0]];
    var sobelY = [[1.0,  2.0, 1.0],
                    [0.0, 0.0, 0.0],
                    [-1.0, -2.0, -1.0]];

    var abs = function(x) (x < 0) ? -x : x;
    for (var y = 0; y < height; y++) {
        for (var x = 0; x < width; x++) {
            
            var totalX = 0;
            var totalY = 0;
            for (var offY = -1; offY <= 1; offY++) {
                var newY = y + offY;
                for (var offX = -1; offX <= 1; offX++) {
                    var newX = x + offX;
                    if ((newX >= 0) && (newX < width) && (newY >= 0) && (newY < height)) {
                        var pointIndex = x + offX + (y + offY) * width;
                        var e = data[pointIndex];
                        totalX += e * sobelX[offY + 1][offX + 1];
                        totalY += e * sobelY[offY + 1][offX + 1];
                    }
                }
            }
            var total = ((abs(totalX) + abs(totalY))/8.0)|0;
            var index = y*width+x;
            data1[index] = total | 0;
        }
    }
    data1.width = width;
    data1.height = height;
    return data1;
}

function detectEdgesSeq_wraparray(data) {
    return detectEdgesSeq_array_wh(data.backingArray, data.width, data.height);
}


WrapArray.prototype.detectEdges2D =
  (function locals () { var detect = detectEdgesSeq_array_wh;
      return function detectEdges() {
        return detect(this.backingArray, this.width, this.height).toRectArray(this.width, this.height);
      };
  })();



function detectEdgesPar_2d(pa, mode)
{
  var sobelX = [[-1.0,  0.0, 1.0],
                [-2.0, 0.0, 2.0],
                [-1.0, 0.0, 1.0]];
  var sobelY = [[1.0,  2.0, 1.0],
                [0.0, 0.0, 0.0],
                [-1.0, -2.0, -1.0]];

  var width = pa.shape[0];
  var height = pa.shape[1];

  var abs = function(x) (x < 0) ? -x : x;
  var ret=new ParallelArray([width, height],
    function (x,y)
    {
      
      var totalX = 0;
      var totalY = 0;
      for (var offY = -1; offY <= 1; offY++) {
        var newY = y + offY;
        for (var offX = -1; offX <= 1; offX++) {
          var newX = x + offX;
          if ((newX >= 0) && (newX < width) && (newY >= 0) && (newY < height)) {
            var pointIndex = (x + offX + (y + offY) * width);
            var e = pa.get(x + offX, y + offY);
            totalX += e * sobelX[offY + 1][offX + 1];
            totalY += e * sobelY[offY + 1][offX + 1];
          }
        }
      }
      var total = (abs(totalX) + abs(totalY))/8.0 | 0;
      return total;
    }, mode);
  ret.width = width;
  ret.height = height;
  return ret;
}

function detectEdgesPar_1d(pa, mode)
{
  var sobelX = [[-1.0,  0.0, 1.0],
                [-2.0, 0.0, 2.0],
                [-1.0, 0.0, 1.0]];
  var sobelY = [[1.0,  2.0, 1.0],
                [0.0, 0.0, 0.0],
                [-1.0, -2.0, -1.0]];

  var width = pa.shape[0];
  var height = pa.shape[1];

  var abs = function(x) (x < 0) ? -x : x;

  var ret=new ParallelArray(width*height,
    function (index)
    {
      var j = index | 0;
      var y = (j / width) | 0;
      var x = (j % width);
      
      var totalX = 0;
      var totalY = 0;
      for (var offY = -1; offY <= 1; offY++) {
        var newY = y + offY;
        for (var offX = -1; offX <= 1; offX++) {
          var newX = x + offX;
          if ((newX >= 0) && (newX < width) && (newY >= 0) && (newY < height)) {
            var pointIndex = (x + offX + (y + offY) * width);
            var e = pa.get(x + offX, y + offY);
            totalX += e * sobelX[offY + 1][offX + 1];
            totalY += e * sobelY[offY + 1][offX + 1];
          }
        }
      }
      var total = (abs(totalX) + abs(totalY))/8.0 | 0;
      return total;
    }, mode);

  ret.width = width;
  ret.height = height;
  return ret;
}


ParallelArray.prototype.detectEdges2D =
  (function locals () { var detect = detectEdgesPar_2d;
      return function detectEdges(mode) detect(this, mode); })();

ParallelArray.prototype.detectEdges1D =
  (function locals () { var detect = detectEdgesPar_1d;
      return function detectEdges(mode) detect(this, mode); })();







function computeEnergy_2d(source, width, height) {
  var energy = new WrapArray(width, height);
  energy.set(0, 0, 0);
  for (var y = 0; y < height; y++) {
    for (var x = 0; x < width; x++) {
      var e = source.get(x, y);
      if (y >= 1) {
        var p = energy.get(x, y-1);
        if (x > 0) {
          p = Math.min(p, energy.get(x-1, y-1));
        }
        if (x < (width - 1)) {
          p = Math.min(p, energy.get(x+1, y-1));
        }
        e += p;
      }
      energy.set(x, y, e);
    }
  }
  return energy;
}

function computeEnergy_1d(source, width, height) {
  var energy = new WrapArray(width, height);
  energy.set(0, 0, 0);
  for (var y = 0; y < height; y++) {
    for (var x = 0; x < width; x++) {
      var e = source.get(x + y*width);
      if (y >= 1) {
        var p = energy.get(x, (y-1));
        if (x > 0) {
          p = Math.min(p, energy.get(x-1, (y-1)));
        }
        if (x < (width - 1)) {
          p = Math.min(p, energy.get(x+1, (y-1)));
        }
        e += p;
      }
      energy.set(x, y, e);
    }
  }
  return energy;
}


WrapArray.prototype.computeEnergy =
  (function locals () { var energy = computeEnergy_2d;
      return function computeEnergy() energy(this, this.width, this.height); })();


ParallelArray.prototype.computeEnergy =
  (function locals () {
     var energy1d = computeEnergy_1d;
     var energy2d = computeEnergy_2d;
     return function computeEnergy()
         (this.shape.length == 2)
          ? energy2d(this, this.width, this.height)
          : energy1d(this, this.width, this.height); })();



function findPath(energy)
{
  var height = energy.height;
  var width  = energy.width;
  var path = new Array(height);
  var y = height - 1;
  var minPos = 0;
  var minEnergy = energy.get(minPos, y);

  for (var x = 1; x < width; x++) {
    if (energy.get(x,y) < minEnergy) {
      minEnergy = energy.get(x,y);
      minPos = x;
    }
  }

  path[y] = minPos;
  for (y = height - 2; y >= 0; y--) {
    minEnergy = energy.get(minPos, y);
    
    var p = minPos;
    if (p >= 1 && energy.get(p-1, y) < minEnergy) {
      minPos = p-1; minEnergy = energy.get(p-1, y);
    }
    if (p < width - 1 && energy.get(p+1, y) < minEnergy) {
      minPos = p+1; minEnergy = energy.get(p+1, y);
    }
    path[y] = minPos;
  }
  return path;
}


WrapArray.prototype.findPath =
  (function locals() { var path = findPath;
      return function findPath() path(this); })();


function cutPathHorizontallyBW(ra, path) {
  return WrapArray.build(ra.width-1, ra.height,
                         function (x, y) {
                             if (x < path[y]-1)
                               return ra.get(x, y);
                             if (x == path[y]-1)
                               return (ra.get(x,y)+ra.get(x+1,y))/2|0;
                             else
                               return ra.get(x+1,y);
                         });
}


WrapArray.prototype.cutPathHorizontallyBW =
  (function locals() { var cut = cutPathHorizontallyBW;
      return function cutPathHorizontallyBW(path) cut(this, path);  })();


function cutPathVerticallyBW(ra, path) {
  return WrapArray.build(ra.width, ra.height-1,
                         function (x, y) {
                             if (y < path[x]-1)
                               return ra.get(x, y);
                             if (y == path[x]-1)
                               return (ra.get(x,y)+ra.get(x,y+1))/2|0;
                             else
                               return ra.get(x,y+1);
                         });
}


WrapArray.prototype.cutPathVerticallyBW =
  (function locals() { var cut = cutPathVerticallyBW;
      return function cutPathVerticallyBW(path) cut(this, path); })();



function cutHorizontalSeamBW_seq(r)
{
  var e = r.detectEdges2D().computeEnergy();
  var p = e.findPath(); e = null;
  return r.cutPathHorizontallyBW(p);
}

function cutHorizontalSeamBW_par(r, mode)
{
  var e = r.toParallelArray(mode).detectEdges1D(mode).computeEnergy(mode);
  var p = e.findPath(mode); e = null;
  return r.cutPathHorizontallyBW(p, mode);
}


WrapArray.prototype.cutHorizontalSeamBW =
  (function locals() {
      var cut_seq = cutHorizontalSeamBW_seq;
      var cut_par = cutHorizontalSeamBW_par;
      return function cutHorizontalSeamBW(mode) {
        return (mode ? cut_par(this, mode) : cut_seq(this));
      };})();


function cutVerticalSeamBW_seq(r)
{
  var e = r.transpose().detectEdges2D().computeEnergy();
  return r.cutPathVerticallyBW(e.findPath());
}

function cutVerticalSeamBW_par(r, mode)
{
  var e = r.transposeParallelArray(mode).detectEdges1D(mode).computeEnergy(mode);
  return r.cutPathVerticallyBW(e.findPath());
}


WrapArray.prototype.cutVerticalSeamBW =
  (function locals() {
      var cut_seq = cutVerticalSeamBW_seq;
      var cut_par = cutVerticalSeamBW_par;
      return function cutVerticalSeamBW(mode) {
        return (mode ? cut_par(this, mode) : cut_seq(this));
      };})();


WrapArray.prototype.shrinkBW = function shrinkBW(w, h, mode) {
  if (w == undefined)
    w = this.width / 2 | 0;
  if (h == undefined)
    h = this.height / 2 | 0;
  var r = this;
  var i=0;
  while (r.height > h || r.width > w) {
    if (i > 0 && i%50 == 0) { print("shrinkBW iteration "+i); } i++;
    if (r.width > w) 
      r = r.cutHorizontalSeamBW(mode);
    if (r.height > h)
      r = r.cutVerticalSeamBW(mode);
  }
  return r;
};


WrapArray.prototype.timedShrinkBW = function timedShrinkBW(w, h, mode) {
  var times = {
    "topar": 0, "trans": 0, "edges": 0, "energ": 0, "fpath": 0, "cpath": 0
  };
  var r = this;
  var lasttime = new Date();
  function elapsed() {
    var d = new Date(); var e = d - lasttime; lasttime = d; return e;
  }
  if (mode) {
    while (r.height > h || r.width > w) {
      if (r.width > w) {
        elapsed();
        var e = r.toParallelArray(mode);
        times.topar += elapsed();
        e = e.detectEdges1D(mode);
        times.edges += elapsed();
        e = e.computeEnergy(mode);
        times.energ += elapsed();
        e = e.findPath(mode);
        times.fpath += elapsed();
        r = r.cutPathHorizontallyBW(e, mode);
        times.cpath += elapsed();
        e = null;
      }
      if (r.height > h) {
        elapsed();
        var e = r.transposeParallelArray(mode);
        times.trans += elapsed();
        e = e.detectEdges1D(mode);
        times.edges += elapsed();
        e = e.computeEnergy(mode);
        times.energ += elapsed();
        e = e.findPath(mode);
        times.fpath += elapsed();
        r = r.cutPathVerticallyBW(e, mode);
        times.cpath += elapsed();
        e = null;
      }
    }
  } else {
    while (r.height > h || r.width > w) {
      if (r.width > w) {
        elapsed();
        var e = r.detectEdges2D();
        times.edges += elapsed();
        e = e.computeEnergy();
        times.energ += elapsed();
        var p = e.findPath(); e = null;
        times.fpath += elapsed();
        r = r.cutPathHorizontallyBW(p);
        times.cpath += elapsed();
        e = null; p = null;
      }
      if (r.height > h) {
        elapsed();
        var e = r.transpose();
        times.trans += elapsed();
        e = e.detectEdges2D();
        times.edges += elapsed();
        e = e.computeEnergy();
        times.energ += elapsed();
        var p = e.findPath(); e = null;
        times.fpath += elapsed();
        r = r.cutPathVerticallyBW(p);
        times.cpath += elapsed();
      }
    }
  }
  return times;
};

function timedDetectEdges2D(mode) {
    var d = new Date(); this.detectEdges2D(mode); var e = new Date(); return e - d;
};

function timedDetectEdges1D(mode) {
    var d = new Date(); this.detectEdges1D(mode); var e = new Date(); return e - d;
};

WrapArray.prototype.timedDetectEdges2D = timedDetectEdges2D;
ParallelArray.prototype.timedDetectEdges2D = timedDetectEdges2D;
ParallelArray.prototype.timedDetectEdges1D = timedDetectEdges1D;

if (benchmarking) {
  

  
  
  
  
  
  
  
  
  
  var seqInput = stripedImage(800/4|0, 542/4|0, 10, 10);
  var parInput = seqInput.toParallelArray();

  function buildSequentially() {
    return seqInput.toParallelArray({mode:"seq"});
  }
  function buildParallel() {
    return seqInput.toParallelArray({mode:"par"});
  }

  function edgesSequentially() {
    return detectEdgesSeq_wraparray(seqInput);
  }
  function edgesParallel() {
    return detectEdgesPar_1d(parInput);
  }

  function resizSequentially() {
    var input = seqInput;
    return input.shrinkBW(input.width/2|0, input.height/2|0);
  }
  function resizParallel() {
    var input = seqInput; 
                          
                          
                          
                          
    return input.shrinkBW(input.width/2|0, input.height/2|0, {mode:"par"});
  }

  if (benchmarking) {
    benchmark("BUILD", 1, DEFAULT_MEASURE,
              buildSequentially, buildParallel);

    benchmark("EDGES", 1, DEFAULT_MEASURE,
              edgesSequentially, edgesParallel);

    benchmark("RESIZ", 1, DEFAULT_MEASURE,
              resizSequentially, resizParallel);
    }
}
