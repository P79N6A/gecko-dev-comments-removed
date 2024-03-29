


"use strict";

const {loadModule} = require("framescript/manager");
const {withTab, receiveMessage} = require("./util");
const {getBrowserForTab} = require("sdk/tabs/utils");

exports.testLoadModule = withTab(function*(assert, tab) {
  const {messageManager} = getBrowserForTab(tab);

  loadModule(messageManager,
             require.resolve("./framescript-manager/frame-script"),
             true,
             "onInit");

  const message = yield receiveMessage(messageManager, "framescript-manager/ready");

  assert.deepEqual(message.data, {state: "ready"},
                   "received ready message from the loaded module");

  messageManager.sendAsyncMessage("framescript-manager/ping", {x: 1});

  const pong = yield receiveMessage(messageManager, "framescript-manager/pong");

  assert.deepEqual(pong.data, {x: 1},
                    "received pong back");
}, "data:text/html,<h1>Message Manager</h1>");


require("sdk/test").run(exports);
