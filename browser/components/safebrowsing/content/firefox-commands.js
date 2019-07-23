














































function PROT_CommandController(commandHandlerMap) {
  this.debugZone = "commandhandler";
  this.cmds_ = commandHandlerMap;
}





PROT_CommandController.prototype.supportsCommand = function(cmd) { 
  return (cmd in this.cmds_); 
}






PROT_CommandController.prototype.isCommandEnabled = function(cmd) { 
  return true; 
}
  





PROT_CommandController.prototype.doCommand = function(cmd) {
  return this.cmds_[cmd](); 
}
 



PROT_CommandController.prototype.onEvent = function(cmd) { }

