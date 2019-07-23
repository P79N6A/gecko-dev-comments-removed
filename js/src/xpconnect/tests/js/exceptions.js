






































 




  StartTest( "Exceptions" );

var CLASS = Components.classes["@mozilla.org/js/xpc/test/Echo;1"];
var IFACE = Components.interfaces.nsIEcho;
var nativeEcho = CLASS.createInstance(IFACE);

var localEcho =  {
    SendOneString : function() {throw this.result;},
    result : 0
};

nativeEcho.SetReceiver(localEcho);

PASS = "PASSED!";
FAIL = "FAILED!";

var result = PASS;
var error =  "";

try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    error = e;
	result = FAIL;
}

AddTestCase(
	"nativeEcho.SendOneString(\"foo\");" + error,
	PASS,
	result );



localEcho.result = new Components.Exception("hi", Components.results.NS_ERROR_ABORT);
result = FAIL;
error =  "";
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    result = PASS;

}

AddTestCase(
	"localEcho.result = new Components.Exception(\"hi\")," +
	"Components.results.NS_ERROR_ABORT);"+
	"nativeEcho.SendOneString(\"foo\");" + error,
	PASS,
	result );



localEcho.result = {message : "pretending to be NS_OK", result : 0};
result = PASS;
error =  "";

try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
	error = e;
    result = FAIL;
}
AddTestCase(
	"localEcho.result = {message : \"pretending to be NS_OK\", result : 0};"+
	"nativeEcho.SendOneString(\"foo\");" + error,
	PASS,
	result );


localEcho.result = {message : "pretending to be NS_ERROR_NO_INTERFACE", 
                    result : Components.results.NS_ERROR_NO_INTERFACE};
result = PASS;
error =  "";
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
	error = e;
    result = PASS;

}
AddTestCase(
	"localEcho.result = {message : \"pretending to be NS_ERROR_NO_INTERFACE\", "+
	"result : Components.results.NS_ERROR_NO_INTERFACE};"+
	"nativeEcho.SendOneString(\"foo\");" + error,
	PASS,
	result );


localEcho.result = "just a string";
result = FAIL;
error =  "";
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    result = PASS;
	error = e;
}
AddTestCase(
	"localEcho.result = localEcho.result = \"just a string\";"+
	"nativeEcho.SendOneString(\"foo\");" + error,
	PASS,
	result );


localEcho.result = Components.results.NS_ERROR_NO_INTERFACE;
result = FAIL;
error = "";
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    result = PASS;
	error = e;
}
AddTestCase(
	"localEcho.result = Components.results.NS_ERROR_NO_INTERFACE;" +
	"nativeEcho.SendOneString(\"foo\");" + error,
	PASS,
	result );

localEcho.result = "NS_OK";
result = FAIL;
error = "";
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    result = PASS;
	error = e;
}
AddTestCase(
	"localEcho.result = \"NS_OK\"" +
	"nativeEcho.SendOneString(\"foo\");" + error,
	PASS,
	result );


localEcho.result = Components;
result = FAIL;
error = "";
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    result = PASS;
	error = e;
}

AddTestCase( 
	"localEcho.result = Components;" +
	"nativeEcho.sendOneString(\"foo\") " + error,
	PASS,
	result);


localEcho.SendOneString = function(){return Components.foo.bar;};
result = FAIL;
error = "";
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    result = PASS;
	error = e;
}
AddTestCase( 
	"localEcho.SendOneString = function(){return Components.foo.bar;};" +
	"nativeEcho.sendOneString(\"foo\") " + error,
	PASS,
	result);


localEcho.SendOneString = function(){new Function("","foo ===== 1")};
result = FAIL;
error = ""
try {
    nativeEcho.SendOneString("foo");
} catch(e)  {
    result = PASS;
	error = e;
}

AddTestCase(
	"localEcho.SendOneString = function(){new Function(\"\",\"foo ===== 1\")};" +
	"nativeEcho.sendOneString(\"foo\") " + error,
	PASS,
	result);


StopTest();