try {
    (function(b, foreign, p) {
         "use asm"
         var ff = foreign.ff
         function f() {
            ff() | 0
         }
         return f
    })(this, {
        ff: startTraceLogger
    }, ArrayBuffer)()
} catch(e) {
    
    
    
}

assertEq(true, true);
