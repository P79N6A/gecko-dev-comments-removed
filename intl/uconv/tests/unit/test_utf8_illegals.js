

const Cc = Components.Constructor;
const Ci = Components.interfaces;
        
const inStrings1 = new Array("%c0%af",              
                             "%e0%80%af",
                             "%f0%80%80%af",
                             "%f8%80%80%80%af",
                             "%fc%80%80%80%80%af",
                                                    
                             "%ed%a0%80",           
                             "%ed%ad%bf",           
                             "%ed%ae%80",           
                             "%ed%af%bf",           
                             "%ed%b0%80",           
                             "%ed%be%80",           
                             "%ed%bf%bf");          
const expected1 = "ABC\ufffdXYZ";
                                                    
const inStrings2 = new Array("%ed%a0%80%ed%b0%80",  
                             "%ed%a0%80%ed%bf%bf",  
                             "%ed%ad%bf%ed%b0%80",  
                             "%ed%ad%bf%ed%bf%bf",  
                             "%ed%ae%80%ed%b0%80",  
                             "%ed%ae%80%ed%bf%bf",  
                             "%ed%af%bf%ed%b0%80",  
                             "%ed%ad%bf%ed%bf%bf"); 
const expected2 = "ABC\ufffd\ufffdXYZ";

function testCaseInputStream(inStr, expected)
{
  var dataURI = "data:text/plain; charset=UTF-8,ABC" + inStr + "XYZ"
  dump(inStr + "==>");

  var IOService = Cc("@mozilla.org/network/io-service;1",
		     "nsIIOService");
  var ConverterInputStream =
      Cc("@mozilla.org/intl/converter-input-stream;1",
	 "nsIConverterInputStream",
	 "init");

  var ios = new IOService();
  var channel = ios.newChannel(dataURI, "", null);
  var testInputStream = channel.open();
  var testConverter = new ConverterInputStream(testInputStream,
					       "UTF-8",
					       16,
					       0xFFFD);

  if (!(testConverter instanceof Ci.nsIUnicharLineInputStream))
      throw "not line input stream";

  var outStr = "";
  var more;
  do {
      
      var line = {};
      more = testConverter.readLine(line);
      outStr += line.value;
  } while (more);

  dump(outStr + "; expected=" + expected + "\n");
  do_check_eq(outStr, expected);
  do_check_eq(outStr.length, expected.length);
}

function run_test() {
    for (var i = 0; i < inStrings1.length; ++i) {
	var inStr = inStrings1[i];
	testCaseInputStream(inStr, expected1);
    }
    for (var i = 0; i < inStrings2.length; ++i) {
	var inStr = inStrings2[i];
	testCaseInputStream(inStr, expected2);
    }
}
