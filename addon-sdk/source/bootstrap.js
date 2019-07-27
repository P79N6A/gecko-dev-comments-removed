


"use strict";




const { utils: Cu } = Components;
const rootURI = __SCRIPT_URI_SPEC__.replace("bootstrap.js", "");
const { require } = Cu.import(`${rootURI}/lib/toolkit/require.js`, {});
const { Bootstrap } = require(`${rootURI}/lib/sdk/addon/bootstrap.js`);
const { startup, shutdown, install, uninstall } = new Bootstrap(rootURI);
