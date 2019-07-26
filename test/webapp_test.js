





var expect = chai.expect;

describe("Router", function() {
  "use strict";

  var router;

  beforeEach(function() {
    router = new loop.webapp.Router();
  });

  describe("#constructor", function() {
    it("should define a default active view", function() {
      expect(router.view).to.be.an.instanceOf(loop.webapp.HomeView);
    });
  });

  describe("#loadView", function() {
    it("should set the active view", function() {
      router.loadView(new loop.webapp.CallLauncherView({token: "fake"}));

      expect(router.view).to.be.an.instanceOf(loop.webapp.CallLauncherView);
    });
  });
});

describe("CallLauncherView", function() {
  "use strict";

  describe("#constructor", function() {
    it("should require a token option", function() {
      expect(function() {
        new loop.webapp.CallLauncherView();
      }).to.Throw(Error, /missing required token/);
    });
  });
});
