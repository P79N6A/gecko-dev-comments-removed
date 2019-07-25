


var g = newGlobal('new-compartment');
for (var j = 0; j < 4; j++) {
    var dbg = new Debug;
    dbg.addDebuggee(g);
    dbg.enabled = false;
    dbg = null;
    gc(); gc();
} 
