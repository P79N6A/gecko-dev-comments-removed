











if (!getBuildConfiguration().parallelJS || !this.TypedObject)
  quit(0);

const benchmark = true;
const iterations = benchmark ? 100 : 1;

const T = TypedObject;
const IX = new T.ArrayType(T.uint32);

const { loc, bytes, height, width, maxval } = readPgm("cat.pgm");
if (maxval > 255)
    throw "Bad maxval: " + maxval;



var ROWS_PER_SLICE=20;
var indices = new IX(ROWS_PER_SLICE*(width-2));
for ( var i=0 ; i < ROWS_PER_SLICE ; i++ )
    for ( var j=0 ; j < width-2 ; j++ )
	indices[i*(width-2)+j] = (i << 20) | (j+1);


var warmup = edgeDetect1(bytes, indices, loc, height, width);

var r = time(
    function () {
	var r;
	for ( var i=0 ; i < iterations ; i++ ) {
	    r = null;
	    r = edgeDetect1(bytes, indices, loc, height, width);
	}
	return r;
    });

if (benchmark)
    print(r.time);
else {
    
    var result = [];
    for ( var y=0 ; y < r.result.length ; y++ )
	for ( var i=0 ; i < ROWS_PER_SLICE ; i++ )
	    result.push(subarray(r.result[y], i*(width-2), (i+1)*(width-2)));

    
    
    
    
    
    var out = copyAndZeroPgm(bytes, loc);
    for ( var h=1 ; h < height-1 ; h++ ) {
	if (h-1 >= result.length)	
	    continue;
	for ( var w=1 ; w < width-1 ; w++ )
	    out[loc+(h*width)+w] = result[h-1][w-1];
    }
    
    putstr(encode(out));
}

quit();




function edgeDetect1(input, indices, loc, height, width) {
    function c1(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp) {
	
	
	
	return 0 - xmm - xzm - xpm + xmp + xzp + xpp;
    }
    function c2(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp) {
	
	
	
	return xmm + xmz + xmp - xpm - xpz - xpp;
    }
    function c3(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp) {
	
	
	
	return 0 - xmm - xzm - xpm - xmz + 8*xzz - xpz - xmp - xzp - xpp;
    }
    function c4(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp) {
	
	
	
	return xmz - xzm + xzp - xpz;
    }
    function max2(a,b) { return a > b ? a : b }
    function max4(a,b,c,d) { return max2(max2(a,b),max2(c,d)); }
    function max5(a,b,c,d,e) { return max2(max4(a,b,c,d),e); }
    var result = [];
    
    for ( var _h=1 ; _h < height-1 ; _h+=ROWS_PER_SLICE ) {
	result.push(indices.mapPar(
	    function (x) {
		var w = (x & 0xFFFFF);
		var h = (x >> 20) + _h;
		var xmm=input[loc+(h-1)*width+(w-1)];
		var xzm=input[loc+h*width+(w-1)];
		var xpm=input[loc+(h+1)*width+(w-1)];
		var xmz=input[loc+(h-1)*width+w];
		var xzz=input[loc+h*width+w];
		var xpz=input[loc+(h+1)*width+w];
		var xmp=input[loc+(h-1)*width+(w+1)];
		var xzp=input[loc+h*width+(w+1)];
		var xpp=input[loc+(h+1)*width+(w+1)];
		
		var sum=max5(0,
			     c1(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp),
			     c2(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp),
			     c3(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp),
			     c4(xmm,xzm,xpm,xmz,xzz,xpz,xmp,xzp,xpp));
		return sum;
	    }));
    }
    return result;
}



function subarray(x, start, end) {
    var v = [];
    for ( var i=start ; i < end ; i++ )
	v.push(x[i]);
    return v;
}



function time(thunk) {
    var then = Date.now();
    var r = thunk();
    var now = Date.now();
    return { time:now - then, result:r};
}



function readPgm(filename) {
    var bytes = snarf(filename, "binary"); 
    var loc = 0;
    var { loc, word } = getAscii(bytes, loc, true);
    if (word != "P5")
	throw "Bad magic: " + word;
    var { loc, word } = getAscii(bytes, loc);
    var width = parseInt(word);
    var { loc, word } = getAscii(bytes, loc);
    var height = parseInt(word);
    var { loc, word } = getAscii(bytes, loc);
    var maxval = parseInt(word);
    loc++;
    return { bytes: bytes, loc: loc, width: width, height: height, maxval: maxval };
}

function copyAndZeroPgm(bytes, loc) {
    var out = new Uint8Array(bytes.length);
    for ( var i=0 ; i < loc ; i++ )
	out[i] = bytes[i];
    return out;
}

function getAscii(bytes, loc, here) {
    if (!here)
	loc = skipWhite(bytes, loc);
    var s = "";
    while (loc < bytes.length && bytes[loc] > 32)
	s += String.fromCharCode(bytes[loc++]);
    return { loc: loc, word: s };
}

function skipWhite(bytes, loc) {
    while (loc < bytes.length && bytes[loc] <= 32)
	loc++;
    return loc;
}

function encode(xs) {
    function hex(n) {
	return "0123456789abcdef".charAt(n);
    }
    var out = "";
    for ( var i=0, limit=xs.length ; i < limit ; i++ ) {
	var c = xs[i];
	out += hex((c >> 4) & 15);
	out += hex(c & 15);
    }
    return out;
}
