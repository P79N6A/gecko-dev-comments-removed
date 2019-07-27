











"use strict";

Services.scriptloader.loadSubScript(getRootDirectory(gTestPath) +
                                    "loader_common.js", this);

let ChromeUtils = {};
Services.scriptloader.loadSubScript(
  "chrome://mochikit/content/tests/SimpleTest/ChromeUtils.js", ChromeUtils);


let Output = {
  print: info,
};


let Assert = {
  ok: _mochitestAssert.ok,
  equal: _mochitestAssert.equal,
};


let add_task_in_parent_process = add_task;
let add_task_in_child_process = function () {};
let add_task_in_both_processes = add_task;

Services.scriptloader.loadSubScript(getRootDirectory(gTestPath) +
                                    "head_common.js", this);



