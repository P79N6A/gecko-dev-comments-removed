



"use strict";

module.metadata = {
  "stability": "stable"
};

const { CC } = require('chrome');
const { id, name, prefixURI, rootURI, metadata,
        version, loadReason } = require('@loader/options');

const { readURISync } = require('./net/url');

const addonDataURI = prefixURI + name + '/data/';

function uri(path) {
  return addonDataURI + (path || '');
}





exports.uri = 'addon:' + id;
exports.id = id;
exports.name = name;
exports.loadReason = loadReason;
exports.version = version;

exports.packed = rootURI.indexOf('jar:') === 0
exports.data = Object.freeze({
  url: uri,
  load: function read(path) {
    return readURISync(uri(path));
  }
});
exports.isPrivateBrowsingSupported = ((metadata.permissions || {})['private-browsing'] === true) ?
                                     true : false;
