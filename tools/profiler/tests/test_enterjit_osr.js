


function run_test() {
    let p = Cc["@mozilla.org/tools/profiler;1"];
    
    if (!p)
        return;
    p = p.getService(Ci.nsIProfiler);
    if (!p)
        return;

    
    
    
    do_check_true(!p.IsActive());

    const ms = 5;
    p.StartProfiler(100, ms, ["js"], 1);

    function arbitrary_name(){
        
        
        var delayMS = 5;
        while (1) {
            do_print("loop: ms = " + delayMS);
            let then = Date.now();
            do {
                let n = 10000;
                while (--n); 
                
            } while (Date.now() - then < delayMS);
            let pr = p.getProfileData().threads[0].samples;
            if (pr.length > 0 || delayMS > 30000)
                return pr;
            delayMS *= 2;
        }
    };

    var profile = arbitrary_name();

    do_check_neq(profile.length, 0);
    let stack = profile[profile.length - 1].frames.map(f => f.location);
    do_print(stack);

    
    
    var gotName = false;
    for (var i = 0; i < stack.length; i++) {
        if (stack[i].match(/arbitrary_name/)) {
            do_check_eq(gotName, false);
            gotName = true;
        }
    }
    do_check_eq(gotName, true);

    p.StopProfiler();
}
