


"use strict";

const { utils: Cu } = Components;
const {require} = Cu.import(`${ROOT}/toolkit/require.js`, {});
const {Bootstrap} = require(`${ROOT}/sdk/addon/bootstrap.js`);
const {startup, shutdown, install, uninstall} = new Bootstrap();
