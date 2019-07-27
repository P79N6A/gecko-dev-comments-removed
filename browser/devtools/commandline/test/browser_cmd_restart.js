




const TEST_URI = "data:text/html;charset=utf-8,gcli-command-restart";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.audit(options, [
      {
        setup: 'restart',
        check: {
          input:  'restart',
          markup: 'VVVVVVV',
          status: 'VALID',
          args: {
            nocache: { value: false },
            safemode: { value: false },
          }
        },
      },
      {
        setup: 'restart --nocache',
        check: {
          input:  'restart --nocache',
          markup: 'VVVVVVVVVVVVVVVVV',
          status: 'VALID',
          args: {
            nocache: { value: true },
            safemode: { value: false },
          }
        },
      },
      {
        setup: 'restart --safemode',
        check: {
          input:  'restart --safemode',
          markup: 'VVVVVVVVVVVVVVVVVV',
          status: 'VALID',
          args: {
            nocache: { value: false },
            safemode: { value: true },
          }
        },
      },
      {
        setup: 'restart --safemode --nocache',
        check: {
          input:  'restart --safemode --nocache',
          markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVV',
          status: 'VALID',
          args: {
            nocache: { value: true },
            safemode: { value: true },
          }
        },
      },
    ]);
  }).then(finish, helpers.handleError);
}
