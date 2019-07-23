





















































var SECTION = "LC3";
var TITLE   = "Throw JS types as exceptions through Java";
startTest();

var exception = "No exception thrown";
var result = "Failed";
var data_type = "no type";




try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw 'foo';");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "foo")
	result = "Passed!";
}

new TestCase("Throwing JS string through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw 42;");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "42")
	result = "Passed!";
}

new TestCase("Throwing JS number (int) through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw 4.2;");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "4.2")
	result = "Passed!";
}

new TestCase("Throwing JS number (float) through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw false;");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "false")
	result = "Passed!";
}

new TestCase("Throwing JS boolean through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw {a:5};");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    result = "Passed!";
}

new TestCase("Throwing JS Object through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw new Date();");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    result = "Passed!";
}

new TestCase("Throwing JS Object (Date)through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw new String();");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    result = "Passed!";
}

new TestCase("Throwing JS Object (String) through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw undefined");
} catch ( e ) {
    exception = "Exception";
    data_type = typeof e;
    result = "Passed!";
}

new TestCase("Throwing undefined through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );




exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw null;");
} catch ( e ) {
    exception = "Exception";
    data_type = typeof e;
    result = "Passed!";
}

new TestCase("Throwing null through Java "+
	     "\n=> threw ("+data_type+") "+exception+" ",
	     "Passed!",
	     result );

test();


