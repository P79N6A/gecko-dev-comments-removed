function run_test() {
    let p = Cc["@mozilla.org/tools/profiler;1"];
    
    if (!p)
	return;
    p = p.getService(Ci.nsIProfiler);
    if (!p)
	return;

    do_check_true(!p.IsActive());

    p.StartProfiler(100, 10, ["js"], 1);
    
    (function (){
	p.StopProfiler();
	let n = 10000;
	while (--n);  
	
	
    })();
}
