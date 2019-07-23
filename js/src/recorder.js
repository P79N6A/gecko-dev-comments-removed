





































 
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
        var x = this.code.push(x);
        if (x)
            this.map[to] = x;
    },
    
    setSP: function(sp) {
        this.SP = sp;
    },
    
    generate_constant: function(v, vv) {
        this.emit({ op: "generate_constant", value: vv }, v);
    },
    boolean_to_jsval: function(b, v, vv) {
        this.emit({ op: "boolean_to_jsval", value: vv, operand: this.map[b] }, v);
    },
    string_to_jsval: function(s, v, vv) {
        this.emit({ op: "string_to_jsval", value: vv, operand: this.map[s] }, v);
    },
    object_to_jsval: function(o, v, vv) {
        this.emit({ op: "object_to_jsval", value: vv, operand: this.map[o] }, v);
    },
    id_to_jsval: function(id, v, vv) {
        this.emit({ op: "id_to_jsval", value: vv, operand: this.map[id] }, v);
    },
    guard_jsdouble_is_int_and_int_fits_in_jsval: function(d, i, vv, g) {
        this.emit({ op: "guard_jsdouble_is_int_and_int_fits_in_jsval", value: vv, 
                    operand: this.map[d], state: g }, i);
    }
});
