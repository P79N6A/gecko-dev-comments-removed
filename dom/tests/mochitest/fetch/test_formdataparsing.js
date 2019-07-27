var boundary = "1234567891011121314151617";



function testFormDataParsing(fn) {

  function makeTest(shouldPass, input, testFn) {
    var obj = fn(input);
    return obj.formData().then(function(fd) {
      ok(shouldPass, "Expected test to be valid FormData for " + input);
      if (testFn) {
        return testFn(fd);
      }
    }, function(e) {
      if (shouldPass) {
        ok(false, "Expected test to pass for " + input);
      } else {
        ok(e.name == "TypeError", "Error should be a TypeError.");
      }
    });
  }

  
  var tests =
    [
      [ true,

        boundary +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',

        function(fd) {
          is(fd.get("greeting"), '"hello"');
        }
      ],
      [ false,

        
        boundary +
        '\r\nContent-Disposition: form-datafoobar; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ true,

        '--' +
        boundary +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',

        function(fd) {
          is(fd.get("greeting"), '"hello"');
        }
      ],
      [ false,
        boundary + "\r\n\r\n" + boundary + '-',
      ],
      [ false,
        
        boundary + "\r\n\r\n" + boundary,
      ],
      [ false,

        
        '-' +
        boundary +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        'invalid' +
        boundary +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary + 'suffix' +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary + 'suffix' +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        
        boundary.substr(3) +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary +
        
        '\rContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary +
        
        '\r\nContent-Disposition: mixed; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary +
        
        '\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary +
        
        '\r\nContent-Dispositypo: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary +
        
        '\r\nContent-Disposition: form-data;\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary +
        
        '\r\nContent-Disposition: form-data; name="greeting"\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        
        boundary + "\r\n\r\n" + boundary +
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n"hello"\r\n' +
        boundary + '-',
      ],
      [ false,

        boundary +
        
        
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n' +
        boundary + '-',
      ],
      [ true,

        boundary +
        
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n\r\n' +
        boundary + '-',

        function(fd) {
          is(fd.get("greeting"), "", "Empty value is allowed.");
        }
      ],
      [ false,
        boundary +
        
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n' +
        boundary + '\r\n' +
        boundary + '-',
      ],
      [ false,
        boundary +
        
        '\r\nContent-Disposition: form-data; name="greeting"\r\n\r\n' +
        boundary
      ],
      [ true,
        boundary +
        
        '\r\nContent-Disposition: form-data; name="file"; filename=file1.txt\r\n\r\n\r\n' +
        boundary + '-',

        function(fd) {
          var f = fd.get("file");
          ok(f instanceof File, "Entry with filename attribute should be read as File.");
          is(f.name, "file1.txt", "Filename should match.");
          is(f.type, "text/plain", "Default content-type should be text/plain.");
          return readAsText(f).then(function(text) {
            is(text, "", "File should be empty.");
          });
        }
      ],
      [ true,
        boundary +
        
        '\r\nContent-Disposition: form-data; name="file"; filename="file1.txt"\r\n\r\n\r\n' +
        boundary + '-',

        function(fd) {
          var f = fd.get("file");
          ok(f instanceof File, "Entry with filename attribute should be read as File.");
          is(f.name, "file1.txt", "Filename should match.");
          is(f.type, "text/plain", "Default content-type should be text/plain.");
          return readAsText(f).then(function(text) {
            is(text, "", "File should be empty.");
          });
        }
      ],
      [ false,
        boundary +
        
        '\r\nContent-Disposition: form-data; name="file"; filename="[\n@;xt"\r\n\r\n\r\n' +
        boundary + '-',
      ],
      [ true,
        boundary +
        '\r\nContent-Disposition: form-data; name="file"; filename="[@;xt"\r\n\r\n\r\n' +
        boundary + '-',

        function(fd) {
          var f = fd.get("file");
          ok(f instanceof File, "Entry with filename attribute should be read as File.");
          is(f.name, "[@", "Filename should match.");
        }
      ],
      [ true,
        boundary +
        '\r\nContent-Disposition: form-data; name="file"; filename="file with   spaces"\r\n\r\n\r\n' +
        boundary + '-',

        function(fd) {
          var f = fd.get("file");
          ok(f instanceof File, "Entry with filename attribute should be read as File.");
          is(f.name, "file with spaces", "Filename should match.");
        }
      ],
      [ true,
        boundary + '\r\n' +
        'Content-Disposition: form-data; name="file"; filename="xml.txt"\r\n' +
        'content-type       : application/xml\r\n' +
        '\r\n' +
        '<body>foobar\r\n\r\n</body>\r\n' +
        boundary + '-',

        function(fd) {
          var f = fd.get("file");
          ok(f instanceof File, "Entry with filename attribute should be read as File.");
          is(f.name, "xml.txt", "Filename should match.");
          is(f.type, "application/xml", "content-type should be application/xml.");
          return readAsText(f).then(function(text) {
            is(text, "<body>foobar\r\n\r\n</body>", "File should have correct text.");
          });
        }
      ],
    ];

  var promises = [];
  for (var i = 0; i < tests.length; ++i) {
    var test = tests[i];
    promises.push(makeTest(test[0], test[1], test[2]));
  }

  return Promise.all(promises);
}

function makeRequest(body) {
  var req = new Request("", { method: 'post', body: body,
                              headers: {
                                'Content-Type': 'multipart/form-data; boundary=' + boundary
                              }});
  return req;
}

function makeResponse(body) {
  var res = new Response(body, { headers: {
                                   'Content-Type': 'multipart/form-data; boundary=' + boundary
                                 }});
  return res;
}

function runTest() {
  return Promise.all([testFormDataParsing(makeRequest),
                      testFormDataParsing(makeResponse)]);
}
