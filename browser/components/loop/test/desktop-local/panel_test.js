





var expect = chai.expect;

describe("loop.panel", function() {
  "use strict";

  var sandbox, notifier, fakeXHR, requests = [];

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
    fakeXHR = sandbox.useFakeXMLHttpRequest();
    requests = [];
    
    fakeXHR.xhr.onCreate = function (xhr) {
      requests.push(xhr);
    };
    notifier = {
      clear: sandbox.spy(),
      notify: sandbox.spy(),
      warn: sandbox.spy(),
      warnL10n: sandbox.spy(),
      error: sandbox.spy(),
      errorL10n: sandbox.spy()
    };
  });

  afterEach(function() {
    $("#fixtures").empty();
    sandbox.restore();
  });

  describe("loop.panel.PanelRouter", function() {
    describe("#constructor", function() {
      it("should require a notifier", function() {
        expect(function() {
          new loop.panel.PanelRouter();
        }).to.Throw(Error, /missing required notifier/);
      });
    });

    describe("constructed", function() {
      var router;

      beforeEach(function() {
        router = new loop.panel.PanelRouter({notifier: notifier});
        sandbox.stub(router, "loadView");
      });

      describe("#home", function() {
        it("should load the PanelView", function() {
          router.home();

          sinon.assert.calledOnce(router.loadView);
          sinon.assert.calledWithExactly(router.loadView,
            sinon.match.instanceOf(loop.panel.PanelView));
        });
      });
    });
  });

  describe("loop.panel.PanelView", function() {
    beforeEach(function() {
      $("#fixtures").append('<div id="messages"></div><div id="main"></div>');
    });

    describe("#getCallUrl", function() {
      it("should request a call url to the server", function() {
        var requestCallUrl = sandbox.stub(loop.shared.Client.prototype,
                                          "requestCallUrl");
        var view = new loop.panel.PanelView({notifier: notifier});
        sandbox.stub(view, "getNickname").returns("foo");

        view.getCallUrl({preventDefault: sandbox.spy()});

        sinon.assert.calledOnce(requestCallUrl);
        sinon.assert.calledWith(requestCallUrl, "foo");
      });

      it("should notify the user when the operation failed", function() {
        var requestCallUrl = sandbox.stub(
          loop.shared.Client.prototype, "requestCallUrl", function(_, cb) {
            cb("fake error");
          });
        var view = new loop.panel.PanelView({notifier: notifier});

        view.getCallUrl({preventDefault: sandbox.spy()});

        sinon.assert.calledOnce(view.notifier.errorL10n);
        sinon.assert.calledWithExactly(view.notifier.errorL10n,
                                       "unable_retrieve_url");
      });
    });

    describe("#onCallUrlReceived", function() {
      it("should update the text field with the call url", function() {
        var view = new loop.panel.PanelView({notifier: notifier});
        view.render();

        view.onCallUrlReceived("http://call.me/");

        expect(view.$("#call-url").val()).eql("http://call.me/");
      });

      it("should reset all pending notifications", function() {
        var view = new loop.panel.PanelView({notifier: notifier}).render();

        view.onCallUrlReceived("http://call.me/");

        sinon.assert.calledOnce(view.notifier.clear, "clear");
      });
    });
  });
});
