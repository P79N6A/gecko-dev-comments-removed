





const test = [

	      ["abcdefghijklmnopqrstuvwxyz12test00%8e%80foobar",

               "abcdefghijklmnopqrstuvwxyz12test00\uFFFDfoobar"],

	      ["abcdefghijklmnopqrstuvwxyz12test01%8efoobar",

               "abcdefghijklmnopqrstuvwxyz12test01\uFFFDfoobar"],

              ["abcdefghijklmnopqrstuvwxyz12test02%bf%80foobar",

	       "abcdefghijklmnopqrstuvwxyz12test02\uFFFDfoobar"],

              ["abcdefghijklmnopqrstuvwxyz12test03%bffoobar",

	       "abcdefghijklmnopqrstuvwxyz12test03\uFFFDfoobar"]];

const IOService = Components.Constructor("@mozilla.org/network/io-service;1",
                                         "nsIIOService");
const ConverterInputStream =
      Components.Constructor("@mozilla.org/intl/converter-input-stream;1",
                             "nsIConverterInputStream",
                             "init");
const ios = new IOService();

function testCase(testText, expectedText, bufferLength, charset)
{
  var dataURI = "data:text/plain;charset=" + charset + "," + testText;

  var channel = ios.newChannel(dataURI, "", null);
  var testInputStream = channel.open();
  var testConverter = new ConverterInputStream(testInputStream,
                                               charset,
                                               bufferLength,
                                               0xFFFD);

  if (!(testConverter instanceof
        Components.interfaces.nsIUnicharLineInputStream))
    throw "not line input stream";

  var outStr = "";
  var more;
  do {
    
    var line = {};
    more = testConverter.readLine(line);
    outStr += line.value;
  } while (more);

  if (outStr != expectedText) {
    dump("Failed with bufferLength = " + bufferLength + "\n");
    if (outStr.length == expectedText.length) {
      for (i = 0; i < outStr.length; ++i) {
	if (outStr.charCodeAt(i) != expectedText.charCodeAt(i)) {
	  dump(i + ": " + outStr.charCodeAt(i).toString(16) + " != " + expectedText.charCodeAt(i).toString(16) + "\n");
	}
      }
    }
  }

  
  do_check_eq(escape(outStr), escape(expectedText));
}

function run_test()
{
  for (var i = 0; i < test.length; ++i) {
    for (var bufferLength = 32; bufferLength < 40; ++ bufferLength) {
      testCase(test[i][0], test[i][1], bufferLength, "EUC-JP");
    }
  }
}
