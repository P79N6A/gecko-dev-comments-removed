



'use strict';

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci } = require('chrome');
const system = require('../sdk/system');
const runtime = require('../sdk/system/runtime');
const oscpu = Cc["@mozilla.org/network/protocol;1?name=http"]
                 .getService(Ci.nsIHttpProtocolHandler).oscpu;
const hostname = Cc["@mozilla.org/network/dns-service;1"]
                 .getService(Ci.nsIDNSService).myHostName;
const isWindows = system.platform === 'win32';
const endianness = ((new Uint32Array((new Uint8Array([1,2,3,4])).buffer))[0] === 0x04030201) ? 'LE' : 'BE';




exports.tmpdir = () => system.pathFor('TmpD');




exports.endianness = () => endianness;




exports.hostname = () => hostname;






exports.type = () => runtime.OS;






exports.platform = () => system.platform;





exports.arch = () => system.architecture;




exports.release = () => {
  let match = oscpu.match(/(\d[\.\d]*)/);
  return match && match.length > 1 ? match[1] : oscpu;
};




exports.EOL = isWindows ? '\r\n' : '\n';




exports.loadavg = () => [0, 0, 0];

['uptime', 'totalmem', 'freemem', 'cpus'].forEach(method => {
  exports[method] = () => { throw new Error('os.' + method + ' is not supported.'); };
});
