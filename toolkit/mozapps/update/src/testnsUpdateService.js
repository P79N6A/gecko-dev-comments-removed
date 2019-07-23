











































var xbDEBUG = false;
var top = new Object();
var window = new Object();
window.document = new Object();
var navigator = new Object();
navigator.userAgent = new Object();

navigator.userAgent.indexOf = function () {
  return -1;
}


function parseErrorStack(excp)
{
    var stack = [];
    var name;

    if (!excp || !excp.stack)
    {
        return stack;
    }

    var stacklist = excp.stack.split('\n');

    for (var i = 0; i < stacklist.length - 1; i++)
    {
        var framedata = stacklist[i];

        name = framedata.match(/^(\w*)/)[1];
        if (!name) {
            name = 'anonymous';
        }

        stack[stack.length] = name;
    }
    

    while (stack.length && stack[stack.length - 1] == 'anonymous')
    {
        stack.length = stack.length - 1;
    }
    return stack;
}

function getStackTrace() {
    var result = '';

    if (typeof(arguments.caller) != 'undefined') { 
        for (var a = arguments.caller; a != null; a = a.caller) {
            result += '> ' + getFunctionName(a.callee) + '\n';
            if (a.caller == a) {
                result += '*';
                break;
            }
        }
    }
    else { 
        
        var testExcp;
        try
        {
            foo.bar;
        }
        catch(testExcp)
        {
            var stack = parseErrorStack(testExcp);
            for (var i = 1; i < stack.length; i++)
            {
                result += '> ' + stack[i] + '\n';
            }
        }
    }

    return result;
}

function JsUnitException(comment, message) {
    this.isJsUnitException = true;
    this.comment = comment;
    this.jsUnitMessage = message;
    this.stackTrace = getStackTrace();
}

function _displayStringForValue(aVar) {
    var result = '<' + aVar + '>';
    if (!(aVar === null || aVar === top.JSUNIT_UNDEFINED_VALUE)) {
        result += ' (' + _trueTypeOf(aVar) + ')';
    }
    return result;
}

function commentArg(expectedNumberOfNonCommentArgs, args) {
    if (argumentsIncludeComments(expectedNumberOfNonCommentArgs, args))
        return args[0];

    return null;
}

function _assert(comment, booleanValue, failureMessage) {
    if (!booleanValue)
        throw new JsUnitException(comment, failureMessage);
}

function argumentsIncludeComments(expectedNumberOfNonCommentArgs, args) {
    return args.length == expectedNumberOfNonCommentArgs + 1;
}

function nonCommentArg(desiredNonCommentArgIndex, expectedNumberOfNonCommentArgs, args) {
    return argumentsIncludeComments(expectedNumberOfNonCommentArgs, args) ?
           args[desiredNonCommentArgIndex] :
           args[desiredNonCommentArgIndex - 1];
}

function _validateArguments(expectedNumberOfNonCommentArgs, args) {
    if (!( args.length == expectedNumberOfNonCommentArgs ||
           (args.length == expectedNumberOfNonCommentArgs + 1 && typeof(args[0]) == 'string') ))
        error('Incorrect arguments passed to assert function');
}
function assertNull() {
    _validateArguments(1, arguments);
    var aVar = nonCommentArg(1, 1, arguments);
    _assert(commentArg(1, arguments), aVar === null, 'Expected ' + _displayStringForValue(null) + ' but was ' + _displayStringForValue(aVar));
}


global_object = this;
mytestManager = new Object();

mytestManager.executeTestFunction = function(functionName) {
  this._testFunctionName = functionName;
  var excep = null;
  assertNull(excep)
  var timeBefore = new Date();
  try {
    global_object[this._testFunctionName]();
  }
  catch (e1) {
    excep = e1
  }
  var timeTaken = (new Date() - timeBefore) / 1000;
  print (timeTaken);
  
  if (excep != null) {
    var problemMessage = this._testFunctionName + ' ';
    
    if (typeof(excep.isJsUnitException) == 'undefined' || !excep.isJsUnitException) {
      problemMessage += 'had an error';
      this.errorCount++;
    }
    else {
      problemMessage += 'failed';
      this.failureCount++;
    }
    print(problemMessage);
    print(this._problemDetailMessageFor(excep));
  }
}

mytestManager._problemDetailMessageFor = function (excep) {
    var result = null;
    if (typeof(excep.isJsUnitException) != 'undefined' && excep.isJsUnitException) {
        result = '';
        if (excep.comment != null)
            result += ('"' + excep.comment + '"\n');

        result += excep.jsUnitMessage;

        if (excep.stackTrace)
            result += '\n\nStack trace follows:\n' + excep.stackTrace;
    }
    else {
        result = 'Error message is:\n"';
        result +=
        (typeof(excep.description) == 'undefined') ?
        excep :
        excep.description;
        result += '"';
        if (typeof(excep.stack) != 'undefined') 
            result += '\n\nStack trace follows:\n' + excep.stack;
    }
    return result;
}

mytestManager.runAllTests = function () {
  for (var test in global_object) {
    if ((typeof(global_object[test]) == "function") && test.match('^test')) {
      mytestManager.executeTestFunction(test);
    }
  }
}

function assertThrows(assertion, command) {

  try {
    eval(command);
    _assert(command, false, 'Expected assertion, but got none');
  }
  catch (e) {
    if (typeof(e.isJsUnitException) != 'undefined' && e.isJsUnitException) {
      throw e;
    }
    else {
      _assert(command, (e && e == assertion), 'Expected assertion ' + assertion + ', but was ' + e);
    }
  }
 
}

function assertDoesNotThrow(assertion, command) {

  try {
    eval(command);
  }
  catch (e) {
    if (typeof(e.isJsUnitException) != 'undefined' && e.isJsUnitException) {
      throw e;
    }
    else {
      _assert(command, (e && e != assertion), 'Expected anything but assertion ' + assertion + ', and caught ' + e);
    }
  }
 
}
function doUpdatePatchFromXML(updateString) {
  
  var parser = Components.classes["@mozilla.org/xmlextras/domparser;1"]
    .createInstance(Components.interfaces.nsIDOMParser);
  var doc = parser.parseFromString(updateString, "text/xml");
  
  var updateCount = doc.documentElement.childNodes.length;
  
  for (var i = 0; i < updateCount; ++i) {
    var update = doc.documentElement.childNodes.item(i);
    if (update.nodeType != Node.ELEMENT_NODE ||
        update.localName != "update")
      continue;
    
    update.QueryInterface(Components.interfaces.nsIDOMElement);
    
    for (var i = 0; i < update.childNodes.length; ++i) {
      
      patchElement = update.childNodes.item(i);
      if (patchElement.nodeType != Node.ELEMENT_NODE ||
          patchElement.localName != "patch")
        continue;
      
      patchElement.QueryInterface(Components.interfaces.nsIDOMElement);
      
      var patch = new UpdatePatch(patchElement);
    }
  }
}

updateXMLThrows = new Object();
updateXMLThrows.zeroComplete = '<?xml version="1.0"?><updates><update type="minor" version="1.7" extensionVersion="1.7" buildID="2005102512"><patch type="complete" URL="DUMMY" hashFunction="MD5" hashValue="9a76b75088671550245428af2194e083" size="0"/><patch type="partial" URL="DUMMY" hashFunction="MD5" hashValue="71000cd10efc7371402d38774edf3b2c" size="652"/></update></updates>';
updateXMLThrows.zeroPartial  = '<?xml version="1.0"?><updates><update type="minor" version="1.7" extensionVersion="1.7" buildID="2005102512"><patch type="complete" URL="DUMMY" hashFunction="MD5" hashValue="9a76b75088671550245428af2194e083" size="8780423"/><patch type="partial" URL="DUMMY" hashFunction="MD5" hashValue="71000cd10efc7371402d38774edf3b2c" size="0"/></update></updates>';
updateXMLThrows.zeroBoth     = '<?xml version="1.0"?><updates><update type="minor" version="1.7" extensionVersion="1.7" buildID="2005102512"><patch type="complete" URL="DUMMY" hashFunction="MD5" hashValue="9a76b75088671550245428af2194e083" size="0"/><patch type="partial" URL="DUMMY" hashFunction="MD5" hashValue="71000cd10efc7371402d38774edf3b2c" size="0"/></update></updates>';

updateXML = new Object();
updateXML.nonZeroBoth  = '<?xml version="1.0"?><updates><update type="minor" version="1.7" extensionVersion="1.7" buildID="2005102512"><patch type="complete" URL="DUMMY" hashFunction="MD5" hashValue="9a76b75088671550245428af2194e083" size="8780423"/><patch type="partial" URL="DUMMY" hashFunction="MD5" hashValue="71000cd10efc7371402d38774edf3b2c" size="652"/></update></updates>';

for (var i in updateXMLThrows) {
  eval("function testUpdatePatchThrowsExceptionFor" + i
    + "() { assertThrows(Components.results.NS_ERROR_ILLEGAL_VALUE, "
    + "\"doUpdatePatchFromXML(updateXMLThrows['"+i+"'])\");}");
}

for (var i in updateXML) {
  eval("function testUpdatePatchThrowsExceptionFor" + i
    + "() { assertDoesNotThrow(Components.results.NS_ERROR_ILLEGAL_VALUE, "
    + "\"doUpdatePatchFromXML(updateXML['"+i+"'])\");}");
}

mytestManager.runAllTests();
