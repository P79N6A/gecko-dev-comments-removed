


"use strict";

const { windowToMessageManager, nodeToMessageManager } = require("framescript/util");


const onInit = (context) => {
  context.addMessageListener("framescript-util/window/request", _ => {
    windowToMessageManager(context.content.window).sendAsyncMessage(
      "framescript-util/window/response", {window: true});
  });

  context.addMessageListener("framescript-util/node/request", message => {
    const node = context.content.document.querySelector(message.data);
    nodeToMessageManager(node).sendAsyncMessage(
      "framescript-util/node/response", {node: true});
  });
};
exports.onInit = onInit;
