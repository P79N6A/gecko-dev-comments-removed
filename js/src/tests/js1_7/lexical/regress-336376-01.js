





var BUGNUMBER     = "336376";
var summary = "Tests reserved words in contexts in which they are not reserved";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);

















const ALL_TESTS =
  [
    "CONTEXT_OBJECT_LITERAL_PROPERTY",
    "CONTEXT_OBJECT_PROPERTY_DOT_REFERENCE",
    "CONTEXT_OBJECT_PROPERTY_DOT_REFERENCE_IS_FUNCTION",
    "CONTEXT_OBJECT_PROPERTY_DOT_GET",
    "CONTEXT_OBJECT_PROPERTY_DOT_SET",
    ];

function r(keyword, tests)
{
  






  function Reserved(keyword, tests)
  {
    this.keyword = keyword;
    if (tests)
      this.tests = tests;
    else
      this.tests = ALL_TESTS;
  }
  Reserved.prototype =
    {
      toString:
      function()
      {
	return "'" + this.keyword + "' being run against tests " +
	this.tests;
      }
    };
  return new Reserved(keyword, tests);
}


const ECMA_262_3_KEYWORD =
  [
    r("break"),
    r("case"),
    r("catch"),
    r("continue"),
    r("default"),
    r("delete"),
    r("do"),
    r("else"),
    r("finally"),
    r("for"),
    r("function"),
    r("if"),
    r("in"),
    r("instanceof"),
    r("new"),
    r("return"),
    r("switch"),
    r("this"),
    r("throw"),
    r("try"),
    r("typeof"),
    r("var"),
    r("void"),
    r("while"),
    r("with"),
    ];


const ECMA_262_3_FUTURERESERVEDKEYWORD =
  [
    r("abstract"),
    r("boolean"),
    r("byte"),
    r("char"),
    r("class"),
    r("const"),
    r("debugger"),
    r("double"),
    r("enum"),
    r("export"),
    r("extends"),
    r("final"),
    r("float"),
    r("goto"),
    r("implements"),
    r("import"),
    r("int"),
    r("interface"),
    r("long"),
    r("native"),
    r("package"),
    r("private"),
    r("protected"),
    r("public"),
    r("short"),
    r("static"),
    r("super"),
    r("synchronized"),
    r("throws"),
    r("transient"),
    r("volatile"),
    ];


const PSEUDO_RESERVED =
  [
    r("true"),
    r("false"),
    r("null"),
    r("each"),  
    ];


const ECMA_262_4_RESERVED_WORDS =
  [
    r("let")
    ];











function Failure(keyword, test, error)
{
  this.keyword = keyword;
  this.test = test;
  this.error = error;
}
Failure.prototype =
{
  toString:
  function()
  {
    return "*** FAILURE on '" + this.keyword + "'!\n" +
    "* test:     " + this.test + "\n" +
    "* error:    " + this.error + "\n";
  }
};

function Tester()
{
  this._failedTests = [];
}
Tester.prototype =
{
  testReservedWords:
  function(reservedArray)
  {
    var rv;
    for (var i = 0, sz = reservedArray.length; i < sz; i++)
    {
      var res = reservedArray[i];
      if (!res)
	continue;

      var tests = res.tests;
      for (var j = 0, sz2 = tests.length; j < sz2; j++)
      {
	var test = tests[j];
	if (!test)
	  continue;

	try
	{
	  this._tests[test](res.keyword);
	}
	catch (e)
	{
	  this._failedTests.push(new Failure(res.keyword, test, e));
	}
      }
    }
  },
  flushErrors:
  function ()
  {
    if (this._failedTests.length > 0) {
      var except = "*************************\n" +
      "* FAILURES ENCOUNTERED! *\n" +
      "*************************\n";
      for (var i = 0, sz = this._failedTests.length; i < sz; i++)
	except += this._failedTests[i];
      throw except;
    }
  },
  _tests:
  {
    CONTEXT_OBJECT_LITERAL_PROPERTY:
    function(keyword)
    {
      try
      {
	eval("var o = { " + keyword + ": 17 };\n" +
	     "if (o['" + keyword + "'] != 17)\n" +
	     "throw \"o['" + keyword + "'] == 17\";");
      }
      catch (e)
      {
	throw e;
      }
    },
    CONTEXT_OBJECT_PROPERTY_DOT_REFERENCE:
    function(keyword)
    {
      try
      {
	eval("var o = { \"" + keyword + "\": 17, baz: null };\n" +
	     "if (o." + keyword + " != 17)\n" +
	     "throw \"o." + keyword + " == 17\";");
      }
      catch (e)
      {
	throw e;
      }
    },
    CONTEXT_OBJECT_PROPERTY_DOT_REFERENCE_IS_FUNCTION:
    function(keyword)
    {
      try
      {
	eval("var o = { '" + keyword + "': function() { return 17; }, baz: null };\n" +
	     "if (o." + keyword + "() != 17)\n" +
	     "throw \"o." + keyword + " == 17\";");
      }
      catch (e)
      {
	throw e;
      }
    },
    CONTEXT_OBJECT_PROPERTY_DOT_GET:
    function(keyword)
    {
      try
      {
	var o = {};
	eval("o['" + keyword + "'] = 17;\n" +
	     "if (o." + keyword + " != 17)\n" +
	     "throw \"'o." + keyword + " != 17' failed!\";");
      }
      catch (e)
      {
	throw e;
      }
    },
    CONTEXT_OBJECT_PROPERTY_DOT_SET:
    function(keyword)
    {
      try
      {
	var o = {};
	eval("o." + keyword + " = 17;\n" +
	     "if (o['" + keyword + "'] != 17)\n" +
	     "throw \"'o." + keyword + " = 17' failed!\";");
      }
      catch (e)
      {
	throw e;
      }
    },
  }
};






var failed = false;

try
{
  var tester = new Tester();
  tester.testReservedWords(ECMA_262_3_KEYWORD);
  tester.testReservedWords(ECMA_262_3_FUTURERESERVEDKEYWORD);
  tester.testReservedWords(PSEUDO_RESERVED);
  tester.testReservedWords(ECMA_262_4_RESERVED_WORDS);
  tester.flushErrors();
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);
