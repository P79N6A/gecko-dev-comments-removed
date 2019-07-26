



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci, components: { ID: parseUUID } } = require('chrome');
const { generateUUID } = Cc['@mozilla.org/uuid-generator;1'].
                          getService(Ci.nsIUUIDGenerator);



exports.uuid = function uuid(id) id ? parseUUID(id) : generateUUID()
