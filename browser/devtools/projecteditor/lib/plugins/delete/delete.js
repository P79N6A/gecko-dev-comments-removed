





const { Class } = require("sdk/core/heritage");
const { registerPlugin, Plugin } = require("projecteditor/plugins/core");
const { confirm } = require("projecteditor/helpers/prompts");
const { getLocalizedString } = require("projecteditor/helpers/l10n");

var DeletePlugin = Class({
  extends: Plugin,
  shouldConfirm: true,

  init: function(host) {
    this.host.addCommand(this, {
      id: "cmd-delete"
    });
    this.contextMenuItem = this.host.createMenuItem({
      parent: this.host.contextMenuPopup,
      label: getLocalizedString("projecteditor.deleteLabel"),
      command: "cmd-delete"
    });
  },

  confirmDelete: function(resource) {
    let deletePromptMessage = resource.isDir ?
      getLocalizedString("projecteditor.deleteFolderPromptMessage") :
      getLocalizedString("projecteditor.deleteFilePromptMessage");
    return !this.shouldConfirm || confirm(
      getLocalizedString("projecteditor.deletePromptTitle"),
      deletePromptMessage
    );
  },

  onContextMenuOpen: function(resource) {
    
    
    
    
    
    if (!resource.parent) {
      this.contextMenuItem.setAttribute("hidden", "true");
    } else {
      this.contextMenuItem.removeAttribute("hidden");
    }
  },

  onCommand: function(cmd) {
    if (cmd === "cmd-delete") {
      let tree = this.host.projectTree;
      let resource = tree.getSelectedResource();

      if (!this.confirmDelete(resource)) {
        return;
      }

      resource.delete().then(() => {
        this.host.project.refresh();
      });
    }
  }
});

exports.DeletePlugin = DeletePlugin;
registerPlugin(DeletePlugin);
