


"use strict";

module.metadata = {
  "stability": "unstable"
};

if (require("./system/xul-app").is("Fennec")) {
  module.exports = require("./windows/tabs-fennec").tabs;
}
else {
  module.exports = require("./tabs/tabs-firefox");
}

const tabs = module.exports;
