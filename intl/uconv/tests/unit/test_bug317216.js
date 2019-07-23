










const test = [

              ["%00%2D%00%2D%D8%35%DC%20%00%2D%00%2D",

               "--\uD835\uDC20--"],

              ["%00%2D%00%2D%D8%35%00%2D%00%2D",

               "--\uFFFD--"],

              ["%00%2D%00%2D%DC%20%00%2D%00%2D",

               "--\uFFFD--"],

              ["%00%2D%00%2D%D8%35%D8%35%00%2D%00%2D",

               "--\uFFFD\uFFFD--"],

              ["%00%2D%00%2D%DC%20%DC%20%00%2D%00%2D",

              "--\uFFFD\uFFFD--"],

              ["%00%2D%00%2D%DC%20%D8%35%00%2D%00%2D",

               "--\uFFFD\uFFFD--"],

              ["%00%2D%00%2D%D8%35%D8%35%DC%20%00%2D%00%2D",

               "--\uFFFD\uD835\uDC20--"],

              ["%00%2D%00%2D%DC%20%D8%35%DC%20%00%2D%00%2D",

               "--\uFFFD\uD835\uDC20--"],

              ["%00%2D%00%2D%D8%35%DC%20%D8%35%00%2D%00%2D",

               "--\uD835\uDC20\uFFFD--"],

              ["%00%2D%00%2D%D8%35%DC%20%DC%20%00%2D%00%2D",

               "--\uD835\uDC20\uFFFD--"],

              ["%00%2D%00%2D%00%2D%00%2D%D8%35%",

               "----"],

              ["%00%2D%00%2D%00%2D%00%2D%D8",

              "----"]];

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


function flip(str) { return str.replace(/(%..)(%..)/g, "$2$1"); }

function run_test()
{
  for (var i = 0; i < 12; ++i) {
    for (var bufferLength = 4; bufferLength < 8; ++ bufferLength) {
      testCase(test[i][0], test[i][1], bufferLength, "UTF-16BE");
      testCase(flip(test[i][0]), test[i][1], bufferLength, "UTF-16LE");
    }
  }
}
