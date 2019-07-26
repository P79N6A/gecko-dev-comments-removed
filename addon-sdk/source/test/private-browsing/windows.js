


'use strict';

const { pb, pbUtils } = require('./helper');
const { openDialog } = require('window/utils');

exports["test Per Window Private Browsing getter"] = function(assert, done) {
  let win = openDialog({
    private: true
  });

  win.addEventListener('DOMContentLoaded', function onload() {
    win.removeEventListener('DOMContentLoaded', onload, false);

    assert.equal(pbUtils.getMode(win),
                 true, 'Newly opened window is in PB mode');
    assert.equal(pb.isActive, false, 'PB mode is not active');

    win.addEventListener("unload", function onunload() {
      win.removeEventListener('unload', onload, false);
      assert.equal(pb.isActive, false, 'PB mode is not active');
      done();
    }, false);
    win.close();
  }, false);
}

require("test").run(exports);
