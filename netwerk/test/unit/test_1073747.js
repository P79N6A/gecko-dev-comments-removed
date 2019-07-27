

var test = function(s, funcName){
    function Arg(){};
    Arg.prototype.toString = function(){
	do_print("Testing " + funcName + " with null args");
	return this.value;
    };
    
    var args = [s, -1];
    for (var i = 0; i < 10; ++i) {
	args.push(new Arg());
    }
    var up = Components.classes["@mozilla.org/network/url-parser;1?auth=maybe"].getService(Components.interfaces.nsIURLParser);
    try {
	up[funcName].apply(up, args);
	return args;
	} catch (x) {
	    do_check_true(true); 
	    return x;
	}
    
    do_check_true(false);
};
var s = null;
var funcs = ["parseAuthority", "parseFileName", "parseFilePath", "parsePath", "parseServerInfo", "parseURL", "parseUserInfo"];

function run_test() {
    funcs.forEach(function(f){test(s, f);});
}
