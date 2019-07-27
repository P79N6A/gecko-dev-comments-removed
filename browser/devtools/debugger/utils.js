


"use strict";

const utils = {
  









  addCommands: function(commandset, commands) {
    Object.keys(commands).forEach(name => {
      let node = document.createElement('command');
      node.id = name;
      
      
      node.setAttribute('oncommand', ' ');
      node.addEventListener('command', commands[name]);
      commandset.appendChild(node);
    });
  }
};
