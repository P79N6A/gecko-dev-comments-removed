






var expect = chai.expect;

describe("loop.shared.utils", function() {
  "use strict";

  var sandbox;
  var sharedUtils = loop.shared.utils;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
  });

  afterEach(function() {
    navigator.mozLoop = undefined;
    sandbox.restore();
  });

  it("should detect iOS", function() {
    expect(sharedUtils.isIOS("iPad")).eql(true);
    expect(sharedUtils.isIOS("iPod")).eql(true);
    expect(sharedUtils.isIOS("iPhone")).eql(true);
    expect(sharedUtils.isIOS("iPhone Simulator")).eql(true);
  });

  it("shouldn't detect iOS with other platforms", function() {
    expect(sharedUtils.isIOS("MacIntel")).eql(false);
  });

  describe("#isFirefox", function() {
    it("should detect Firefox", function() {
      expect(sharedUtils.isFirefox("Firefox")).eql(true);
      expect(sharedUtils.isFirefox("Gecko/Firefox")).eql(true);
      expect(sharedUtils.isFirefox("Firefox/Gecko")).eql(true);
      expect(sharedUtils.isFirefox("Gecko/Firefox/Chuck Norris")).eql(true);
    });

    it("shouldn't detect Firefox with other platforms", function() {
      expect(sharedUtils.isFirefox("Opera")).eql(false);
    });
  });

  describe("#isFirefoxOS", function() {
    describe("without mozActivities", function() {
      it("shouldn't detect FirefoxOS on mobile platform", function() {
        expect(sharedUtils.isFirefoxOS("mobi")).eql(false);
      });

      it("shouldn't detect FirefoxOS on non mobile platform", function() {
        expect(sharedUtils.isFirefoxOS("whatever")).eql(false);
      });
    });

    describe("with mozActivities", function() {
      var realMozActivity;

      before(function() {
        realMozActivity = window.MozActivity;
        window.MozActivity = {};
      });

      after(function() {
        window.MozActivity = realMozActivity;
      });

      it("should detect FirefoxOS on mobile platform", function() {
        expect(sharedUtils.isFirefoxOS("mobi")).eql(true);
      });

      it("shouldn't detect FirefoxOS on non mobile platform", function() {
        expect(sharedUtils.isFirefoxOS("whatever")).eql(false);
      });
    });
  });

  describe("#formatDate", function() {
    beforeEach(function() {
      sandbox.stub(Date.prototype, "toLocaleDateString").returns("fake result");
    });

    it("should call toLocaleDateString with arguments", function() {
      sharedUtils.formatDate(1000);

      sinon.assert.calledOnce(Date.prototype.toLocaleDateString);
      sinon.assert.calledWithExactly(Date.prototype.toLocaleDateString,
        navigator.language,
        {year: "numeric", month: "long", day: "numeric"}
      );
    });

    it("should return the formatted string", function() {
      expect(sharedUtils.formatDate(1000)).eql("fake result");
    });
  });

  describe("#getBoolPreference", function() {
    afterEach(function() {
      localStorage.removeItem("test.true");
    });

    describe("mozLoop set", function() {
      beforeEach(function() {
        navigator.mozLoop = {
          getLoopPref: function(prefName) {
            return prefName === "test.true";
          }
        };
      });

      it("should return the mozLoop preference", function() {
        expect(sharedUtils.getBoolPreference("test.true")).eql(true);
      });

      it("should not use the localStorage value", function() {
        localStorage.setItem("test.false", true);

        expect(sharedUtils.getBoolPreference("test.false")).eql(false);
      });
    });

    describe("mozLoop not set", function() {
      it("should return the localStorage value", function() {
        localStorage.setItem("test.true", true);

        expect(sharedUtils.getBoolPreference("test.true")).eql(true);
      });
    });
  });

  describe("#composeCallUrlEmail", function() {
    var composeEmail;

    beforeEach(function() {
      
      sandbox.stub(navigator.mozL10n, "get", function(id) {
        switch(id) {
          case "share_email_subject4": return "subject";
          case "share_email_body4":    return "body";
        }
      });
      composeEmail = sandbox.spy();
      navigator.mozLoop = {
        getLoopPref: sandbox.spy(),
        composeEmail: composeEmail
      };
    });

    it("should compose a call url email", function() {
      sharedUtils.composeCallUrlEmail("http://invalid", "fake@invalid.tld");

      sinon.assert.calledOnce(composeEmail);
      sinon.assert.calledWith(composeEmail,
                              "subject", "body", "fake@invalid.tld");
    });
  });
});
