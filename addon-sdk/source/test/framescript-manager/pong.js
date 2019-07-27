


"use strict";

exports.onPing = message =>
  message.target.sendAsyncMessage("framescript-manager/pong", message.data);
