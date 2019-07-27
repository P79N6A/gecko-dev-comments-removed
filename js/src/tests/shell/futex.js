






if ((this.SharedArrayBuffer && this.AtomicsObject && this.getSharedArrayBuffer && this.setSharedArrayBuffer))
    quit(0);

var DEBUG = false;

function dprint(s) {
    if (DEBUG) print(s);
}




var mem = new SharedInt32Array(3);



assertEq(getSharedArrayBuffer(), null); 

assertEq(setSharedArrayBuffer(mem.buffer), undefined); 
assertEq(getSharedArrayBuffer() == null, false);       

var v = getSharedArrayBuffer();
assertEq(v.byteLength, mem.buffer.byteLength); 
var w = new SharedInt32Array(v);
mem[0] = 314159;
assertEq(w[0], 314159);		
mem[0] = 0;

setSharedArrayBuffer(null);	
assertEq(getSharedArrayBuffer(), null);

setSharedArrayBuffer(mem.buffer);
setSharedArrayBuffer(undefined); 
assertEq(getSharedArrayBuffer(), null);

setSharedArrayBuffer(mem.buffer);
setSharedArrayBuffer();		
assertEq(getSharedArrayBuffer(), null);



assertThrowsInstanceOf(() => setSharedArrayBuffer({x:10, y:20}), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer([1,2]), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer(new ArrayBuffer(10)), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer(new Int32Array(10)), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer(new SharedInt32Array(10)), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer(false), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer(3.14), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer(mem), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer("abracadabra"), Error);
assertThrowsInstanceOf(() => setSharedArrayBuffer(() => 37), Error);






mem[0] = 42;
mem[1] = 37;
mem[2] = DEBUG;
setSharedArrayBuffer(mem.buffer);

if (helperThreadCount() === 0) {
  
  reportCompare(true,true);
  quit();
}

evalInWorker(`
var mem = new SharedInt32Array(getSharedArrayBuffer());
function dprint(s) {
    if (mem[2]) print(s);
}
assertEq(mem[0], 42);		// what was written in the main thread
assertEq(mem[1], 37);		//   is read in the worker
mem[1] = 1337;
dprint("Sleeping for 3 seconds");
sleep(3);
dprint("Waking the main thread now");
setSharedArrayBuffer(null);
Atomics.futexWake(mem, 0, 1);
`);

var then = Date.now();
assertEq(Atomics.futexWait(mem, 0, 42), Atomics.OK);
dprint("Woke up as I should have in " + (Date.now() - then)/1000 + "s");
assertEq(mem[1], 1337); 
assertEq(getSharedArrayBuffer(), null); 





timeout(2, function () {
    dprint("In the interrupt, starting inner wait");
    Atomics.futexWait(mem, 0, 42); 
});
var exn = false;
try {
    dprint("Starting outer wait");
    assertEq(Atomics.futexWait(mem, 0, 42, 5000), Atomics.OK);
}
catch (e) {
    dprint("Got the exception!");
    exn = true;
}
finally {
    timeout(-1);
}
assertEq(exn, true);
dprint("Done");

reportCompare(true,true);
