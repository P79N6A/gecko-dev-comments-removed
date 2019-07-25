








































let CommandUpdater = {
  
















  _getControllerForCommand: function(command) {
    try {
      let commandDispatcher = top.document.commandDispatcher;
      let controller = commandDispatcher.getControllerForCommand(command);
      if (controller && controller.isCommandEnabled(command))
        return controller;
    }
    catch (e) { }

    let controllerCount = window.controllers.getControllerCount();
    for (let i = 0; i < controllerCount; ++i) {
      let current = window.controllers.getControllerAt(i);
      try {
        if (current.supportsCommand(command) &&
            current.isCommandEnabled(command))
          return current;
      }
      catch (e) { }
    }
    return controller || window.controllers.getControllerForCommand(command);
  },

  





  updateCommand: function(command) {
    let enabled = false;
    try {
      let controller = this._getControllerForCommand(command);
      if (controller) {
        enabled = controller.isCommandEnabled(command);
      }
    }
    catch (ex) { }

    this.enableCommand(command, enabled);
  },

  





  updateCommands: function(_commands) {
    let commands = _commands.split(',');
    for (let command in commands) {
      this.updateCommand(commands[command]);
    }
  },

  






  enableCommand: function(command, enabled) {
    let element = document.getElementById(command);
    if (!element)
      return;

    if (enabled)
      element.removeAttribute('disabled');
    else
      element.setAttribute('disabled', 'true');
  },

  





  doCommand: function(command) {
    let controller = this._getControllerForCommand(command);
    if (!controller)
      return;
    controller.doCommand(command);
  },

  






  setMenuValue: function(command, labelAttribute) {
    let commandNode = top.document.getElementById(command);
    if (commandNode) {
      let label = commandNode.getAttribute(labelAttribute);
      if (label)
        commandNode.setAttribute('label', label);
    }
  },

  






  setAccessKey: function(command, valueAttribute) {
    let commandNode = top.document.getElementById(command);
    if (commandNode) {
      let value = commandNode.getAttribute(valueAttribute);
      if (value)
        commandNode.setAttribute('accesskey', value);
    }
  },

  








  onEvent: function(node, event) {
    let numControllers = node.controllers.getControllerCount();
    let controller;

    for (let i = 0; i < numControllers; i++) {
      controller = node.controllers.getControllerAt(i);
      if (controller)
        controller.onEvent(event);
    }
  }
};

