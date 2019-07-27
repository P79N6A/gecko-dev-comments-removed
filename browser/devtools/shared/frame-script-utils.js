



"use strict";

addMessageListener("devtools:test:history", function ({ data }) {
  content.history[data.direction]();
});
