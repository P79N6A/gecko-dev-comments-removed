












































StartTest( "Evaluate JavaScript Throw Expressions" );

SetupTest();
AddTestData();
AddTestComment();
StopTest();

function SetupTest() {
    CONTRACTID = "@mozilla.org/js/xpc/test/CallJS;1";
	CLASS = Components.classes[CONTRACTID].createInstance();
	IFACE = Components.interfaces.nsIXPCTestCallJS;
	ERROR = Components.results;

	caller = CLASS.QueryInterface(IFACE);

	JSO = new Function();
	JSO.prototype.Evaluate = new Function( "s", 
		"this.result = eval(s); return this.result" );
	JSO.prototype.gotException = false;
	JSO.prototype.exception = {};
	JSO.prototype.EvaluateAndReturnError = new Function( "s", "r", "r = eval(s); return r;" );

	

	ERRORS_WE_GOT = {};

	for ( var p in Components.results ) {
		ERRORS_WE_GOT[p] = false;
	}
}


function AddTestData() {

var data = [
 ["3",              3,         false],
 ["undefined",      undefined, false],
 ["throw -1",       undefined, true, null,true],
 ["throw 'str'",    undefined, true, "NS_ERROR_XPC_JS_THREW_STRING"],
 ["throw {}",       undefined, true, "NS_ERROR_XPC_JS_THREW_JS_OBJECT"],
 ["throw 1",        undefined, true, "NS_ERROR_XPC_JS_THREW_NUMBER"],
 ["throw Components.results['NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT']",  
                    undefined, true, "NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT"],
 ["throw {message:'error message'};", 
                    undefined, true, "NS_ERROR_XPC_JS_THREW_JS_OBJECT"],
 ["throw null",     undefined, true, "NS_ERROR_XPC_JS_THREW_NULL"],
 ["throw new JSException('msg', 'NS_ERROR_XPC_CANT_SET_OUT_VAL', true)",
                    undefined, true, "NS_ERROR_XPC_CANT_SET_OUT_VAL"],
 ["throw caller",   undefined, true, "NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT"],
 ["boo",            undefined, true, "NS_ERROR_XPC_JAVASCRIPT_ERROR_WITH_DETAILS"],
 ["...ignore this sentinel entry..."]
];

    for(var i = 0; i < data.length-1; i++)
        TestEvaluate(data[i][0],
                     data[i][1],
                     data[i][2],
                     data[i][3],
                     null,
                     data[i][4]);


	



if (0) {
	TestEvaluateToString( 
		1,
		true,
		undefined,
		true,
		"NS_ERROR_XPC_NEED_OUT_OBJECT" );
	
}		
}

function TestEvaluate (evalArg, eResult, eGotException, eExceptionName, jso,
                       errorHasNoName) {
	if ( !jso )
		var jso = new JSO();

	caller.SetJSObject(jso);
	
	try {
		caller.Evaluate(evalArg);
	} catch (e) {
		jso.gotException = true;
		jso.exception = e;
	} finally {
		AddTestCase(
			"caller.Evaluate(" +evalArg+ "); jso.result ",
			eResult,
			jso.result);

		AddTestCase(
			"caller.Evaluate(" +evalArg+ "); jso.gotException ",
			eGotException,
			jso.gotException );

		AddTestCase(
			"caller.Evaluate(" +evalArg+ "); jso.exception.name",
			eExceptionName,
			jso.exception.name );
        if (errorHasNoName) {
		    AddTestCase(
			    "caller.Evaluate(" +evalArg+ "); jso.exception.result",
			    jso.exception.result,
			    jso.exception.result );
        } else {
		    AddTestCase(
			    "caller.Evaluate(" +evalArg+ "); jso.exception.result",
			    Components.results[eExceptionName],
			    jso.exception.result );
		}

		if ( jso.gotException ) {
			ERRORS_WE_GOT[ jso.exception.name ] = true;
		}
	}
}

function TestEvaluateToString (evalArg, outArg, eResult, eGotException, eExceptionName ) {
	var jso = new JSO();
	caller.SetJSObject(jso);
	
	try {
		caller.EvaluateToString(evalArg, outArg);
	} catch (e) {
		jso.gotException = true;
		jso.exception = e;
	} finally {
		AddTestCase(
			"caller.EvaluateToString(" +evalArg+","+outArg+"); jso.result ",
			eResult,
			jso.result);

		AddTestCase(
			"caller.EvaluateToString(" +evalArg+","+outArg+"); outArg ",
			eResult,
			outArg);

		AddTestCase(
			"jso.gotException ",
			eGotException,
			jso.gotException );

		AddTestCase(
			"jso.exception.name",
			eExceptionName,
			jso.exception.name );

		AddTestCase(
			"jso.exception.result",
			Components.results[eExceptionName],
			jso.exception.result );

		if ( jso.gotException ) {
			ERRORS_WE_GOT[ jso.exception.name ] = true;
		}
	}
}

function TestEvaluateArgs( argsArray, eResult, eGotException, eExceptionName ) {
	var jso = new JSO;
	caller.SetJSObject(jso);
	
	try {
		if ( argsArray.length > 0 ) {
			eval( "caller.Evaluate("+argsArray.toString()+")" );
		} else {
			caller.Evaluate();
		}
	} catch (e) {
		jso.gotException = true;
		jso.exception = e;
	} finally {
		AddTestCase(
			"callerEvaluate(" +argsArray+ "); jso.result ",
			eResult,
			jso.result);

		AddTestCase(
			"jso.gotException ",
			eGotException,
			jso.gotException );

		AddTestCase(
			"jso.exception.result",
			Components.results[eExceptionName],
			jso.exception.result );

		AddTestCase(
			"jso.exception.name",
			eExceptionName,
			jso.exception.name );

		if ( jso.gotException ) {
			ERRORS_WE_GOT[ jso.exception.name ] = true;
		}

	}
}
function TestSetJSObject( jso, eResult, eGotException, eExceptionName ) {
	var exception = {};
	var gotException = false;
	var result;
	
	try {
		result = caller.SetJSObject( jso );
	} catch (e) {
		gotException = true;	
		exception = e;
	} finally {
		AddTestCase(
			"caller.SetJSObject(" + jso +")",
			eResult,
			result );

		AddTestCase(
			"gotException? ",
			eGotException,
			gotException);

		AddTestCase(
			"exception.name",
			eExceptionName,
			exception.name );

		if ( gotException ) {
			ERRORS_WE_GOT[ exception.name ] = true;
		}
	}
}

function AddTestComment() {
	var s = "This test exercised the exceptions defined in "+
		"Components.results. The following errors were not exercised:\n";

	for ( var p in ERRORS_WE_GOT ) {
		if ( ERRORS_WE_GOT[p] == false ) {
			s += p +"\n";
		}
	}

	AddComment(s);

}

function JSException ( message, name, data ) {
	this.message = message;
	this.name = name;
	this.result = Components.results[name];
	this.location = 0;
	this.data = (data) ? data : null;
	this.initialize = new Function();
	this.toString = new Function ( "return this.message" );
}

function TryIt(s) {
	try {
		eval(s);
	} catch (e) {
		Enumerate(e);
	}
}
