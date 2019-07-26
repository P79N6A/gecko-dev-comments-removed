function run_test() {
    let p = Cc["@mozilla.org/tools/profiler;1"];
    
    if (!p)
	return;
    p = p.getService(Ci.nsIProfiler);
    if (!p)
	return;

    do_check_true(!p.IsActive());

    
    (function (){
	p.StartProfiler(100, 10, ["js"], 1);
	let n = 10000;
	while (--n); 
	
	
    })();
    p.StopProfiler();
}
