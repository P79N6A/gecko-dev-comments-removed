






var expect = chai.expect;

describe("loop.shared.utils", function() {
  "use strict";

  var sandbox;
  var sharedUtils = loop.shared.utils;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("Helper", function() {
    var helper;

    beforeEach(function() {
      helper = new sharedUtils.Helper();
    });

    describe("#isIOS", function() {
      it("should detect iOS", function() {
        expect(helper.isIOS("iPad")).eql(true);
        expect(helper.isIOS("iPod")).eql(true);
        expect(helper.isIOS("iPhone")).eql(true);
        expect(helper.isIOS("iPhone Simulator")).eql(true);
      });

      it("shouldn't detect iOS with other platforms", function() {
        expect(helper.isIOS("MacIntel")).eql(false);
      });
    });

    describe("#isFirefox", function() {
      it("should detect Firefox", function() {
        expect(helper.isFirefox("Firefox")).eql(true);
        expect(helper.isFirefox("Gecko/Firefox")).eql(true);
        expect(helper.isFirefox("Firefox/Gecko")).eql(true);
        expect(helper.isFirefox("Gecko/Firefox/Chuck Norris")).eql(true);
      });

      it("shouldn't detect Firefox with other platforms", function() {
        expect(helper.isFirefox("Opera")).eql(false);
      });
    });
  });

  describe("#getBoolPreference", function() {
    afterEach(function() {
      navigator.mozLoop = undefined;
      localStorage.removeItem("test.true");
    });

    describe("mozLoop set", function() {
      beforeEach(function() {
        navigator.mozLoop = {
          getLoopBoolPref: function(prefName) {
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
});
