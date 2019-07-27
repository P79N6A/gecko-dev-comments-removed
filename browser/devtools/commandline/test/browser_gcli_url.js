















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_url.js");
}




exports.testDefault = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: options.isPhantomjs, 
      setup:    'urlc',
      check: {
        input:  'urlc',
        markup: 'VVVV',
        hints:        ' <url>',
        status: 'ERROR',
        args: {
          url: {
            value: undefined,
            arg: '',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    'urlc example',
      check: {
        input:  'urlc example',
        markup: 'VVVVVIIIIIII',
        hints:              ' -> http://example/',
        predictions: [
          'http://example/',
          'https://example/',
          'http://localhost:9999/example'
        ],
        status: 'ERROR',
        args: {
          url: {
            value: undefined,
            arg: ' example',
            status: 'INCOMPLETE'
          }
        }
      },
    },
    {
      setup:    'urlc example.com/',
      check: {
        input:  'urlc example.com/',
        markup: 'VVVVVIIIIIIIIIIII',
        hints:                   ' -> http://example.com/',
        status: 'ERROR',
        args: {
          url: {
            value: undefined,
            arg: ' example.com/',
            status: 'INCOMPLETE'
          }
        }
      },
    },
    {
      setup:    'urlc http://example.com/index?q=a#hash',
      check: {
        input:  'urlc http://example.com/index?q=a#hash',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV',
        hints:                                        '',
        status: 'VALID',
        args: {
          url: {
            value: function(data) {
              assert.is(data.hash, '#hash', 'url hash');
            },
            arg: ' http://example.com/index?q=a#hash',
            status: 'VALID'
          }
        }
      },
      exec: { output: /"url": ?/ }
    }
  ]);
};
