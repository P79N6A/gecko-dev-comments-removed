






var expect = chai.expect;

describe("loop.shared.mixins", function() {
  "use strict";

  var sandbox;
  var sharedMixins = loop.shared.mixins;

  beforeEach(function() {
    sandbox = sinon.sandbox.create();
  });

  afterEach(function() {
    sandbox.restore();
  });

  describe("loop.webapp.UrlHashChangeMixin", function() {
    function createTestComponent(onUrlHashChange) {
      var TestComp = React.createClass({
        mixins: [loop.shared.mixins.UrlHashChangeMixin],
        onUrlHashChange: onUrlHashChange || function(){},
        render: function() {
          return React.DOM.div();
        }
      });
      return new TestComp();
    }

    it("should watch for hashchange event", function() {
      var addEventListener = sandbox.spy();
      sharedMixins.setRootObject({
        addEventListener: addEventListener
      });

      TestUtils.renderIntoDocument(createTestComponent());

      sinon.assert.calledOnce(addEventListener);
      sinon.assert.calledWith(addEventListener, "hashchange");
    });

    it("should call onUrlHashChange when the url is updated", function() {
      sharedMixins.setRootObject({
        addEventListener: function(name, cb) {
          if (name === "hashchange") {
            cb();
          }
        }
      });
      var onUrlHashChange = sandbox.stub();

      TestUtils.renderIntoDocument(createTestComponent(onUrlHashChange));

      sinon.assert.calledOnce(onUrlHashChange);
    });
  });

  describe("loop.webapp.DocumentLocationMixin", function() {
    var reloadStub, TestComp;

    beforeEach(function() {
      reloadStub = sandbox.stub();

      sharedMixins.setRootObject({
        location: {
          reload: reloadStub
        }
      });

      TestComp = React.createClass({
        mixins: [loop.shared.mixins.DocumentLocationMixin],
        render: function() {
          return React.DOM.div();
        }
      });
    });

    it("should call window.location.reload", function() {
      var comp = TestUtils.renderIntoDocument(TestComp());

      comp.locationReload();

      sinon.assert.calledOnce(reloadStub);
    });
  });

  describe("loop.panel.DocumentVisibilityMixin", function() {
    var comp, TestComp, onDocumentVisibleStub, onDocumentHiddenStub;

    beforeEach(function() {
      onDocumentVisibleStub = sandbox.stub();
      onDocumentHiddenStub = sandbox.stub();

      TestComp = React.createClass({
        mixins: [loop.shared.mixins.DocumentVisibilityMixin],
        onDocumentHidden: onDocumentHiddenStub,
        onDocumentVisible: onDocumentVisibleStub,
        render: function() {
          return React.DOM.div();
        }
      });
    });

    afterEach(function() {
      loop.shared.mixins.setRootObject(window);
    });

    function setupFakeVisibilityEventDispatcher(event) {
      loop.shared.mixins.setRootObject({
        document: {
          addEventListener: function(_, fn) {
            fn(event);
          },
          removeEventListener: sandbox.stub()
        }
      });
    }

    it("should call onDocumentVisible when document visibility changes to visible",
      function() {
        setupFakeVisibilityEventDispatcher({target: {hidden: false}});

        comp = TestUtils.renderIntoDocument(TestComp());

        sinon.assert.calledOnce(onDocumentVisibleStub);
      });

    it("should call onDocumentVisible when document visibility changes to hidden",
      function() {
        setupFakeVisibilityEventDispatcher({target: {hidden: true}});

        comp = TestUtils.renderIntoDocument(TestComp());

        sinon.assert.calledOnce(onDocumentHiddenStub);
      });
  });
});
