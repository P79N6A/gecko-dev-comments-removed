





var loop = loop || {};
loop.desktopRouter = (function() {
  "use strict";

  







  var extendedRouter = {
    navigate: function(to) {
      this[this.routes[to]]();
    }
  };

  var DesktopRouter = loop.shared.router.BaseRouter.extend(extendedRouter);

  var DesktopConversationRouter =
    loop.shared.router.BaseConversationRouter.extend(extendedRouter);

  return {
    DesktopRouter: DesktopRouter,
    DesktopConversationRouter: DesktopConversationRouter
  };
})();
