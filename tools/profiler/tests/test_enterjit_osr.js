


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
    let profile = (function arbitrary_name(){
	
	let then = Date.now();
	do {
	    let n = 10000;
	    while (--n); 
	    
	} while (Date.now() - then < ms * 2.5);
	return p.getProfileData().threads[0].samples;
    })();
    do_check_neq(profile.length, 0);
    let stack = profile[profile.length - 1].frames.map(f => f.location);
    stack = stack.slice(stack.lastIndexOf("js::RunScript") + 1);

    do_print(stack);
    
    
    if (stack.length < 2 || stack[1] != "EnterJIT") {
	do_print("No JIT?");
	
	do_check_eq(Math.min(stack.length, 1), 1);
	let thisInterp = stack[0];
	do_check_eq(thisInterp.split(" ")[0], "arbitrary_name");
	if (stack.length >= 2) {
	    let nextFrame = stack[1];
	    do_check_neq(nextFrame.split(" ")[0], "arbitrary_name");
	}
    } else {
	do_check_eq(Math.min(stack.length, 3), 3);
	let thisInterp = stack[0];
	let enterJit = stack[1];
	let thisBC = stack[2];
	do_check_eq(thisInterp.split(" ")[0], "arbitrary_name");
	do_check_eq(enterJit, "EnterJIT");
	do_check_eq(thisBC.split(" ")[0], "arbitrary_name");
    }

    p.StopProfiler();
}
