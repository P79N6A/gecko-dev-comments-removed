





































 
({
    start: function(pc, sp) {
        this.error = false;
        this.anchorPC = pc;
        this.anchorSP = this.SP = sp;
        this.code = [];
        this.map = {};
        print("Recording at @" + pc);
    },
    stop: function(pc) {
        print("Recording ended at @" + pc);
    },
    
    track: function(from, to) {
        this.map[to] = this.map[from];
    },
    
    emit: function(x, to) {
        this.map[to] = this.code.push(x);
    },
    
    setSP: function(sp) {
        this.SP = sp;
    },
    
    constant: function(v, c) {
        this.emit({ op: "constant", value: c });
    }	
});
