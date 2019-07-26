





const { Class } = require("sdk/core/heritage");
const { registerPlugin, Plugin } = require("projecteditor/plugins/core");
const { getLocalizedString } = require("projecteditor/helpers/l10n");

var DeletePlugin = Class({
  extends: Plugin,

  init: function(host) {
    this.host.addCommand({
      id: "cmd-delete"
    });
    this.host.createMenuItem({
      parent: "#directory-menu-popup",
      label: getLocalizedString("projecteditor.deleteLabel"),
      command: "cmd-delete"
    });
  },

  onCommand: function(cmd) {
    if (cmd === "cmd-delete") {
      let tree = this.host.projectTree;
      let resource = tree.getSelectedResource();
      let parent = resource.parent;
      tree.deleteResource(resource).then(() => {
        this.host.project.refresh();
      })
    }
  }
});

exports.DeletePlugin = DeletePlugin;
registerPlugin(DeletePlugin);
