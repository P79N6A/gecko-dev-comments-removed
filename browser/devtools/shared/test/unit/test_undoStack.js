




const Cu = Components.utils;
let {Loader} = Cu.import("resource://gre/modules/commonjs/toolkit/loader.js", {});

let loader = new Loader.Loader({
  paths: {
    "": "resource://gre/modules/commonjs/",
    "devtools": "resource:///modules/devtools",
  },
  globals: {},
});
let require = Loader.Require(loader, { id: "undo-test" })

let {UndoStack} = require("devtools/shared/undo");

const MAX_SIZE = 5;

function run_test()
{
  let str = "";
  let stack = new UndoStack(MAX_SIZE);

  function add(ch) {
    stack.do(function() {
      str += ch;
    }, function() {
      str = str.slice(0, -1);
    });
  }

  do_check_false(stack.canUndo());
  do_check_false(stack.canRedo());

  
  add("a");
  do_check_true(stack.canUndo());
  do_check_false(stack.canRedo());

  add("b");
  add("c");
  add("d");
  add("e");

  do_check_eq(str, "abcde");

  
  stack.undo();

  do_check_eq(str, "abcd");
  do_check_true(stack.canRedo());

  stack.redo();
  do_check_eq(str, "abcde")
  do_check_false(stack.canRedo());

  
  stack.undo();
  do_check_eq(str, "abcd");

  add("q");
  do_check_eq(str, "abcdq");
  do_check_false(stack.canRedo());

  stack.undo();
  do_check_eq(str, "abcd");
  stack.redo();
  do_check_eq(str, "abcdq");

  
  while (stack.canUndo()) {
    stack.undo();
  }
  do_check_eq(str, "");

  
  while (stack.canRedo()) {
    stack.redo();
  }
  do_check_eq(str, "abcdq");

  
  add("1");
  add("2");
  add("3");

  do_check_eq(str, "abcdq123");

  
  while (stack.canUndo()) {
    stack.undo();
  }

  do_check_eq(str, "abc");
}
