










const test = [

              ["%D8%35%DC%20%00%2D%00%2D",

               "\uD835\uDC20--"],

              ["%D8%35%00%2D%00%2D",

               "\uFFFD--"],

              ["%DC%20%00%2D%00%2D",

               "\uFFFD--"],

              ["%D8%35%D8%35%00%2D%00%2D",

               "\uFFFD\uFFFD--"],

              ["%DC%20%DC%20%00%2D%00%2D",

	       "\uFFFD\uFFFD--"],

              ["%DC%20%D8%35%00%2D%00%2D",

               "\uFFFD\uFFFD--"],

              ["%D8%35%D8%35%DC%20%00%2D%00%2D",

               "\uFFFD\uD835\uDC20--"],

              ["%DC%20%D8%35%DC%20%00%2D%00%2D",

               "\uFFFD\uD835\uDC20--"],

              ["%D8%35%DC%20%D8%35%00%2D%00%2D",

               "\uD835\uDC20\uFFFD--"],

              ["%D8%35%DC%20%DC%20%00%2D%00%2D",

               "\uD835\uDC20\uFFFD--"],

              ["%D8%35%",

               ""],

              ["%D8",

              ""]];

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

  
  do_check_eq(escape(outStr), escape(expectedText));
}



const MINIMUM_BUFFER_SIZE=32;
function padBytes(str)
{
  var padding = "";
  for (var i = 0; i < MINIMUM_BUFFER_SIZE; ++i) {
    padding += "%00%2D";
  }
  return padding + str;
}

function padUnichars(str)
{
  var padding = "";
  for (var i = 0; i < MINIMUM_BUFFER_SIZE; ++i) {
    padding += "-";
  }
  return padding + str;
}


function flip(str) { return str.replace(/(%..)(%..)/g, "$2$1"); }

function run_test()
{
  for (var i = 0; i < 12; ++i) {
    for (var bufferLength = MINIMUM_BUFFER_SIZE;
	 bufferLength < MINIMUM_BUFFER_SIZE + 4;
	 ++ bufferLength) {
      var testText = padBytes(test[i][0]);
      var expectedText = padUnichars(test[i][1]);
      testCase(testText, expectedText, bufferLength, "UTF-16BE");
      testCase(flip(testText), expectedText, bufferLength, "UTF-16LE");
    }
  }
}
