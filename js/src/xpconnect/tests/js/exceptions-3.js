












































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
	for ( p in Components.results ) {
		if ( p == "NS_OK" ) {
		TestEvaluateAndReturnError(
			Components.results[p],
			undefined,
			false);
		} else {
		TestEvaluateAndReturnError(
			Components.results[p],
			undefined,
			true,
			p);
		}
	}
}

function TestEvaluateAndReturnError( code, eResult, eGotException, eName ) {
	var result;
	var jso = new JSO();

	try {
		result = caller.SetJSObject( jso );
		caller.EvaluateAndReturnError( code );
	} catch (e) {
		jso.gotException = true;
		jso.exception = e;
	} finally {
		AddTestCase(
			"caller.EvaluateAndReturnError("+ code +")",
			eResult,
			jso.result 
		);

		AddTestCase(
			"caller.EvaluateAndReturnError("+ code +"); "+
			"jso.gotException",
			eGotException,
			jso.gotException
		);

		AddTestCase(
			"caller.EvaluateAndReturnError("+ code +"); "+
			"jso.exception.code",
			code,
			jso.exception.result);

		AddTestCase(
			"caller.EvaluateAndReturnError("+ code +"); "+
			"jso.exception.name",
			eName,
			jso.exception.name );
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
	this.code = Components.results[name];
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
