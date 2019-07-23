





































 
({
    start: function(pc) {
        print("Recording at @" + pc);
        return true;
    },
    stop: function(pc) {
        print("Recording ended at @" + pc);
    },
    
    track: function(from, to) {
        print("Mapped value @" + from + " to @" + to);
        return true;
    },
    
    setSP: function(sp) {
        print("SP = @" + sp);
        return true;
    },
    
    constant: function(v, c) {
        print("constant " + c + " -> @" + v); 
        return true;
    }   
});

