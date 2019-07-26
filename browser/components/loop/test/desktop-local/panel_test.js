





var expect = chai.expect;

describe("loop.panel", function() {
  "use strict";

  var sandbox, notifier, fakeXHR, requests = [], savedMozLoop;

  function createTestRouter(fakeDocument) {
    return new loop.panel.PanelRouter({
      notifier: notifier,
      document: fakeDocument
    });
  }

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

    window.navigator.mozLoop = {
      get serverUrl() {
        return "http://example.com";
      },
      getStrings: function() {
        return "{}";
      },
      get locale() {
        return "en-US";
      }
    };
    document.mozL10n.initialize(window.navigator.mozLoop);
  });

  afterEach(function() {
    delete window.navigator.mozLoop;
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

      it("should require a document", function() {
        expect(function() {
          new loop.panel.PanelRouter({notifier: notifier});
        }).to.Throw(Error, /missing required document/);
      });
    });

    describe("constructed", function() {
      var router;

      beforeEach(function() {
        router = createTestRouter({
          hidden: true,
          addEventListener: sandbox.spy()
        });

        sandbox.stub(router, "loadView");
      });

      describe("#home", function() {
        it("should reset the PanelView", function() {
          sandbox.stub(router, "reset");

          router.home();

          sinon.assert.calledOnce(router.reset);
        });
      });

      describe("#reset", function() {
        it("should clear all pending notifications", function() {
          router.reset();

          sinon.assert.calledOnce(notifier.clear);
        });

        it("should load the home view", function() {
          router.reset();

          sinon.assert.calledOnce(router.loadView);
          sinon.assert.calledWithExactly(router.loadView,
            sinon.match.instanceOf(loop.panel.PanelView));
        });
      });

      describe("Events", function() {
        it("should listen to document visibility changes", function() {
          var fakeDocument = {
            hidden: true,
            addEventListener: sandbox.spy()
          };

          var router = createTestRouter(fakeDocument);

          sinon.assert.calledOnce(fakeDocument.addEventListener);
          sinon.assert.calledWith(fakeDocument.addEventListener,
                                  "visibilitychange");
        });

        it("should trigger panel:open when the panel document is visible",
          function(done) {
            var router = createTestRouter({
              hidden: false,
              addEventListener: function(name, cb) {
                setTimeout(function() {
                  cb({currentTarget: {hidden: false}});
                }, 0);
              }
            });

            router.once("panel:open", function() {
              done();
            });
          });

        it("should trigger panel:closed when the panel document is hidden",
          function(done) {
            var router = createTestRouter({
              addEventListener: function(name, cb) {
                hidden: true,
                setTimeout(function() {
                  cb({currentTarget: {hidden: true}});
                }, 0);
              }
            });

            router.once("panel:closed", function() {
              done();
            });
          });
      });
    });
  });

  describe("loop.panel.PanelView", function() {
    beforeEach(function() {
      $("#fixtures").append('<div id="messages"></div><div id="main"></div>');
    });

    describe("#getCallUrl", function() {
      it("should reset all pending notifications", function() {
        var requestCallUrl = sandbox.stub(loop.shared.Client.prototype,
                                          "requestCallUrl");
        var view = new loop.panel.PanelView({notifier: notifier}).render();

        view.getCallUrl({preventDefault: sandbox.spy()});

        sinon.assert.calledOnce(view.notifier.clear, "clear");
      });

      it("should request a call url to the server", function() {
        var requestCallUrl = sandbox.stub(loop.shared.Client.prototype,
                                          "requestCallUrl");
        var view = new loop.panel.PanelView({notifier: notifier});
        sandbox.stub(view, "getNickname").returns("foo");

        view.getCallUrl({preventDefault: sandbox.spy()});

        sinon.assert.calledOnce(requestCallUrl);
        sinon.assert.calledWith(requestCallUrl, "foo");
      });

      it("should set the call url form in a pending state", function() {
        var requestCallUrl = sandbox.stub(loop.shared.Client.prototype,
                                          "requestCallUrl");
        sandbox.stub(loop.panel.PanelView.prototype, "setPending");

        var view = new loop.panel.PanelView({notifier: notifier});

        view.getCallUrl({preventDefault: sandbox.spy()});

        sinon.assert.calledOnce(view.setPending);
      });

      it("should clear the pending state when a response is received",
        function() {
          sandbox.stub(loop.panel.PanelView.prototype,
                       "clearPending");
          var requestCallUrl = sandbox.stub(
            loop.shared.Client.prototype, "requestCallUrl", function(_, cb) {
              cb("fake error");
            });
          var view = new loop.panel.PanelView({notifier: notifier});

          view.getCallUrl({preventDefault: sandbox.spy()});

          sinon.assert.calledOnce(view.clearPending);
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

        sinon.assert.calledOnce(view.notifier.clear);
      });
    });

    describe("events", function() {
      describe("goBack", function() {
        it("should update the button state");
      });

      describe("changeButtonState", function() {
         it("should do set the disabled state if there is no text");
         it("should do set the enabled state if there is text");
      });
    });
  });
});
