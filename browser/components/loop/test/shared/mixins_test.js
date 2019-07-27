






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
