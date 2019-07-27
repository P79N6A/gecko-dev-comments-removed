




const TEST_URI = "http://mochi.test:8888/browser/browser/devtools/commandline/"+
                 "test/browser_cmd_cookie.html";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.audit(options, [
        {
          setup: 'cookie list',
          exec: {
            output: [ /zap=zep/, /zip=zop/ ],
          }
        },
        {
          setup: "cookie set zup banana",
          check: {
            args: {
              name: { value: 'zup' },
              value: { value: 'banana' },
            }
          },
          exec: {
            output: ""
          }
        },
        {
          setup: "cookie list",
          exec: {
            output: [ /zap=zep/, /zip=zop/, /zup=banana/, /Edit/ ]
          }
        }
    ]);
  }).then(finish, helpers.handleError);
}

