

const Cc = Components.Constructor;
const Ci = Components.interfaces;

const tests = [
{ inStrings: ["%80",                 
              "%8f",
              "%90",
              "%9f",
              "%a0",
              "%bf",
              "%c0",
              "%c1",
              "%c2",
              "%df",
              "%e0",
              "%e0%a0",
              "%e0%bf",
              "%ed%80",
              "%ed%9f",
              "%ef",
              "%ef%bf",
              "%f0",
              "%f0%90",
              "%f0%90%80",
              "%f0%90%bf",
              "%f0%bf",
              "%f0%bf%80",
              "%f0%bf%bf",
              "%f4",
              "%f4%80",
              "%f4%80%80",
              "%f4%80%bf",
              "%f4%8f",
              "%f4%8f%80",
              "%f4%8f%bf",
              "%f5",
              "%f7",
              "%f8",
              "%fb",
              "%fc",
              "%fd"],
  expected: "ABC\ufffdXYZ" },

{ inStrings: ["%c0%af",              
              "%c1%af"],             
  expected: "ABC\ufffd\ufffdXYZ" },

{ inStrings: ["%e0%80%80",           
              "%e0%80%af",           
              "%e0%9f%bf",
                                     
              "%ed%a0%80",           
              "%ed%ad%bf",           
              "%ed%ae%80",           
              "%ed%af%bf",           
              "%ed%b0%80",           
              "%ed%be%80",           
              "%ed%bf%bf"],          
  expected: "ABC\ufffd\ufffd\ufffdXYZ" },

{ inStrings: ["%f0%80%80%80",        
              "%f0%80%80%af",        
              "%f0%8f%bf%bf",
              "%f4%90%80%80",
              "%f4%bf%bf%bf",
              "%f5%80%80%80",
              "%f7%bf%bf%bf"],
  expected: "ABC\ufffd\ufffd\ufffd\ufffdXYZ" },

{ inStrings: ["%f8%80%80%80%80",     
              "%f8%80%80%80%af",     
              "%fb%bf%bf%bf%bf"],
  expected: "ABC\ufffd\ufffd\ufffd\ufffd\ufffdXYZ" },

                                     
{ inStrings: ["%ed%a0%80%ed%b0%80",  
              "%ed%a0%80%ed%bf%bf",  
              "%ed%ad%bf%ed%b0%80",  
              "%ed%ad%bf%ed%bf%bf",  
              "%ed%ae%80%ed%b0%80",  
              "%ed%ae%80%ed%bf%bf",  
              "%ed%af%bf%ed%b0%80",  
              "%ed%ad%bf%ed%bf%bf",  
              "%fc%80%80%80%80%80",  
              "%fc%80%80%80%80%af",  
              "%fd%bf%bf%bf%bf%bf"],
  expected: "ABC\ufffd\ufffd\ufffd\ufffd\ufffd\ufffdXYZ" },
];


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
  for (var t of tests) {
    for (var inStr of t.inStrings) {
      testCaseInputStream(inStr, t.expected);
    }
  }
}
