



'use strict';

module.metadata = {
  'stability': 'deprecated'
};

const {
  requiresAddonGlobal, attach, detach, destroy, WorkerHost
} = require('../content/utils');

exports.WorkerHost = WorkerHost;
exports.detach = detach;
exports.attach = attach;
exports.destroy = destroy;
exports.requiresAddonGlobal = requiresAddonGlobal;
