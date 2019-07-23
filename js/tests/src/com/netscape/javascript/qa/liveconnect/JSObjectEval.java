package com.netscape.javascript.qa.liveconnect;

import netscape.javascript.JSObject;









public class JSObjectEval {
    




    public static Object eval(JSObject obj, String code) {
    	obj.eval(code);
	    return null;
    }
}
