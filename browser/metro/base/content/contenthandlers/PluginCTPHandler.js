



var PluginCTPHandler = {
  init: function() {
    addEventListener("PluginClickToPlay", this, false);
  },

  addLinkClickCallback: function (linkNode, callbackName ) {
     
     let self = this;
     let callbackArgs = Array.prototype.slice.call(arguments).slice(2);
     linkNode.addEventListener("click",
                               function(evt) {
                                 if (!evt.isTrusted)
                                   return;
                                 evt.preventDefault();
                                 if (callbackArgs.length == 0)
                                   callbackArgs = [ evt ];
                                 (self[callbackName]).apply(self, callbackArgs);
                               },
                               true);
 
     linkNode.addEventListener("keydown",
                               function(evt) {
                                 if (!evt.isTrusted)
                                   return;
                                 if (evt.keyCode == evt.DOM_VK_RETURN) {
                                   evt.preventDefault();
                                   if (callbackArgs.length == 0)
                                     callbackArgs = [ evt ];
                                   evt.preventDefault();
                                   (self[callbackName]).apply(self, callbackArgs);
                                 }
                               },
                               true);
   },

  handleEvent : function(event) {
    if (event.type != "PluginClickToPlay")
      return;
    let plugin = event.target;
    PluginHandler.addLinkClickCallback(plugin, "reloadToEnablePlugin");
  },

  reloadToEnablePlugin: function() {
    sendAsyncMessage("Browser:PluginClickToPlayClicked", { });
  }
};


