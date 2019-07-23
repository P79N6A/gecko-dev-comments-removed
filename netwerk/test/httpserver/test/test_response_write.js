







































var tests =
  [
   new Test("http://localhost:4444/writeString",
            null, check_1234, succeeded),
   new Test("http://localhost:4444/writeInt",
            null, check_1234, succeeded),
  ];

function run_test()
{
  var srv = createServer();

  srv.registerPathHandler("/writeString", writeString);
  srv.registerPathHandler("/writeInt", writeInt);
  srv.start(4444);

  runHttpTests(tests, testComplete(srv));
}




function succeeded(ch, cx, status, data)
{
  do_check_true(Components.isSuccessCode(status));
  do_check_eq(data.map(function(v) String.fromCharCode(v)).join(""), "1234");
}

function check_1234(ch, cx)
{
  do_check_eq(ch.getResponseHeader("Content-Length"), "4");
}



function writeString(metadata, response)
{
  response.write("1234");
}

function writeInt(metadata, response)
{
  response.write(1234);
}
