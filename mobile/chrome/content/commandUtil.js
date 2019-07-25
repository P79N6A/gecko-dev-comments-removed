








































var CommandUpdater = {
  
















  _getControllerForCommand: function(command) {
    try {
      var controller = top.document.commandDispatcher.getControllerForCommand(command);
      if (controller && controller.isCommandEnabled(command))
        return controller;
    }
    catch(e) {
    }
    var controllerCount = window.controllers.getControllerCount();
    for (var i = 0; i < controllerCount; ++i) {
      var current = window.controllers.getControllerAt(i);
      try {
        if (current.supportsCommand(command) && current.isCommandEnabled(command))
          return current;
      }
      catch (e) {
      }
    }
    return controller || window.controllers.getControllerForCommand(command);
  },

  





  updateCommand: function(command) {
    var enabled = false;
    try {
      var controller = this._getControllerForCommand(command);
      if (controller) {
        enabled = controller.isCommandEnabled(command);
      }
    }
    catch(ex) { }

    this.enableCommand(command, enabled);
  },

  





  updateCommands: function(_commands) {
    var commands = _commands.split(",");
    for (var command in commands) {
      this.updateCommand(commands[command]);
    }
  },

  






  enableCommand: function(command, enabled) {
    var element = document.getElementById(command);
    if (!element)
      return;
    if (enabled)
      element.removeAttribute("disabled");
    else
      element.setAttribute("disabled", "true");
  },

  





  doCommand: function(command) {
    var controller = this._getControllerForCommand(command);
    if (!controller)
      return;
    controller.doCommand(command);
  },

  






  setMenuValue: function(command, labelAttribute) {
    var commandNode = top.document.getElementById(command);
    if (commandNode)
    {
      var label = commandNode.getAttribute(labelAttribute);
      if ( label )
        commandNode.setAttribute('label', label);
    }
  },

  






  setAccessKey: function(command, valueAttribute) {
    var commandNode = top.document.getElementById(command);
    if (commandNode)
    {
      var value = commandNode.getAttribute(valueAttribute);
      if ( value )
        commandNode.setAttribute('accesskey', value);
    }
  },

  








  onEvent: function(node, event) {
    var numControllers = node.controllers.getControllerCount();
    var controller;

    for ( var controllerIndex = 0; controllerIndex < numControllers; controllerIndex++ )
    {
      controller = node.controllers.getControllerAt(controllerIndex);
      if ( controller )
        controller.onEvent(event);
    }
  }
};
