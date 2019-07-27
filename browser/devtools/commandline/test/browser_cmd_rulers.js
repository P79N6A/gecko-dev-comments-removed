


"use strict";



let TEST_PAGE = "data:text/html;charset=utf-8,foo";

function test() {
  return Task.spawn(spawnTest).then(finish, helpers.handleError);
}

function* spawnTest() {
  let options = yield helpers.openTab(TEST_PAGE);
  yield helpers.openToolbar(options);

  yield helpers.audit(options, [
    {
      setup: 'rulers',
      check: {
        input:  'rulers',
        markup: 'VVVVVV',
        status: 'VALID'
      }
    },
    {
      setup: 'rulers on',
      check: {
        input:  'rulers on',
        markup: 'VVVVVVVEE',
        status: 'ERROR'
      },
      exec: {
        output: 'Error: Too many arguments'
      }
    },
    {
      setup: 'rulers --visible',
      check: {
        input:  'rulers --visible',
        markup: 'VVVVVVVEEEEEEEEE',
        status: 'ERROR'
      },
      exec: {
        output: 'Error: Too many arguments'
      }
    }
  ]);

  yield helpers.closeToolbar(options);
  yield helpers.closeTab(options);
}
