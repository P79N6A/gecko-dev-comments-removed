















function add(ta) {
    return Atomics.add(ta, 86, 6);
}

if (!this.SharedArrayBuffer || !this.Atomics || !this.SharedUint32Array)
    quit(0);

var sab = new SharedArrayBuffer(4096);
var ia = new SharedUint32Array(sab);
for ( var i=0, limit=ia.length ; i < limit ; i++ )
    ia[i] = 0xdeadbeef;		
var v = 0;
for ( var i=0 ; i < 1000 ; i++ )
    v += add(ia);

