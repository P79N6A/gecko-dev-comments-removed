



"use strict";

addMessageListener("devtools:test:history", function ({ data }) {
  content.history[data.direction]();
});

addMessageListener("devtools:test:navigate", function ({ data }) {
  content.location = data.location;
});

addMessageListener("devtools:test:reload", function ({ data }) {
  data = data || {};
  content.location.reload(data.forceget);
});

addMessageListener("devtools:test:console", function ({ data }) {
  let method = data.shift();
  content.console[method].apply(content.console, data);
});



addMessageListener("devtools:test:eval", function ({ data }) {
  sendAsyncMessage("devtools:test:eval:response", {
    value: content.eval(data.script),
    id: data.id
  });
});

addEventListener("load", function() {
  sendAsyncMessage("devtools:test:load");
}, true);
