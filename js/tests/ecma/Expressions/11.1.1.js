





































gTestfile = '11.1.1.js';











var SECTION = "11.1.1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " The this keyword");

var GLOBAL_OBJECT = this.toString();



new TestCase( SECTION,
	      "Global Code: this.toString()",
	      GLOBAL_OBJECT,
	      this.toString() );

new TestCase( SECTION,
	      "Global Code:  eval('this.toString()')",
	      GLOBAL_OBJECT,
	      eval('this.toString()') );



new TestCase( SECTION,
	      "Anonymous Code: var MYFUNC = new Function('return this.toString()'); MYFUNC()",
	      GLOBAL_OBJECT,
	      eval("var MYFUNC = new Function('return this.toString()'); MYFUNC()") );



new TestCase( SECTION,
	      "Anonymous Code: var MYFUNC = new Function('return (eval(\"this.toString()\")'); (MYFUNC()).toString()",
	      GLOBAL_OBJECT,
	      eval("var MYFUNC = new Function('return eval(\"this.toString()\")'); (MYFUNC()).toString()") );



new TestCase( SECTION,
	      "Anonymous Code: var MYFUNC = new Function('this.THIS = this'); ((new MYFUNC()).THIS).toString()",
	      "[object Object]",
	      eval("var MYFUNC = new Function('this.THIS = this'); ((new MYFUNC()).THIS).toString()") );

new TestCase( SECTION,
	      "Anonymous Code: var MYFUNC = new Function('this.THIS = this'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1",
	      true,
	      eval("var MYFUNC = new Function('this.THIS = this'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1") );

new TestCase( SECTION,
	      "Anonymous Code: var MYFUNC = new Function('this.THIS = eval(\"this\")'); ((new MYFUNC().THIS).toString()",
	      "[object Object]",
	      eval("var MYFUNC = new Function('this.THIS = eval(\"this\")'); ((new MYFUNC()).THIS).toString()") );

new TestCase( SECTION,
	      "Anonymous Code: var MYFUNC = new Function('this.THIS = eval(\"this\")'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1",
	      true,
	      eval("var MYFUNC = new Function('this.THIS = eval(\"this\")'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1") );


new TestCase( SECTION,
	      "Function Code:  ReturnThis()",
	      GLOBAL_OBJECT,
	      ReturnThis() );

new TestCase( SECTION,
	      "Function Code:  ReturnEvalThis()",
	      GLOBAL_OBJECT,
	      ReturnEvalThis() );


new TestCase( SECTION,
	      "var MYOBJECT = new ReturnThis(); MYOBJECT.toString()",
	      "[object Object]",
	      eval("var MYOBJECT = new ReturnThis(); MYOBJECT.toString()") );

new TestCase( SECTION,
	      "var MYOBJECT = new ReturnEvalThis(); MYOBJECT.toString()",
	      "[object Object]",
	      eval("var MYOBJECT = new ReturnEvalThis(); MYOBJECT.toString()") );

test();

function ReturnThis() {
  return this.toString();
}

function ReturnEvalThis() {
  return( eval("this.toString()") );
}
