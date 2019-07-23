function loop(f, expected) {
   
   
   
   
   for (var i = 0; i < 9; i++)
       assertEq(f(), expected);
}

function C(bad) {
   var x = bad;
   function f() {
       return x;  
                  
                  
   }
   this.m = f;
   return f;
}

var obj = {
   set m(f) {
       if (f())  
                 
                 
           loop(f, true);
   }
};

loop(C.call(obj, false), false);
C.call(obj, true);

checkStats({
    recorderStarted: 1,
    recorderAborted: 0,
    traceCompleted: 2,
    traceTriggered: 4
});
