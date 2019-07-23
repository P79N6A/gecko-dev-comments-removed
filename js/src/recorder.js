





































 
({
    start: function(pc, sp) {
	this.anchorPC = pc;
	this.anchorSP = this.SP = sp;
	this.code = [];
	this.map = {};
	print("Recording at @" + pc);
        return true;
    },
    stop: function(pc) {
        print("Recording ended at @" + pc);
    },
    
    track: function(from, to) {
	this.map[to] = this.map[from];
	return true;
    },
    
    emit: function(x, to) {
	this.map[to] = this.code.push(x);
	return true;
    },
    
    setSP: function(sp) {
	this.SP = sp;
	return true;
    },
    
    constant: function(v, c) {
	this.emit({ op: "constant", value: c });
	return true;
    }	
});

