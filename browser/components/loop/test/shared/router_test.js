





var expect = chai.expect;

describe("loop.shared.router", function() {
  "use strict";

  var sandbox;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("BaseRouter", function() {
    var router;

    beforeEach(function() {
      router = new loop.shared.router.BaseRouter();
    });

    describe("#loadView", function() {
      it("should set the active view", function() {
        var TestView = loop.shared.views.BaseView.extend({});
        var view = new TestView();

        router.loadView(view);

        expect(router.activeView).eql(view);
      });
    });
  });
});
