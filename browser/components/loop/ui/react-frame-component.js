











window.queuedFrames = [];












window.Frame = React.createClass({
  propTypes: {
    style: React.PropTypes.object,
    head: React.PropTypes.node,
    width: React.PropTypes.number,
    height: React.PropTypes.number,
    onContentsRendered: React.PropTypes.func
  },
  render: function() {
    return React.createElement("iframe", {
      style: this.props.style,
      head: this.props.head,
      width: this.props.width,
      height: this.props.height
    });
  },
  componentDidMount: function() {
    this.renderFrameContents();
  },
  renderFrameContents: function() {
    var doc = this.getDOMNode().contentDocument;
    if (doc && doc.readyState === "complete") {
      
      window.queuedFrames.splice(window.queuedFrames.indexOf(this), 1);

      var iframeHead = doc.querySelector("head");
      var parentHeadChildren = document.querySelector("head").children;

      [].forEach.call(parentHeadChildren, function(parentHeadNode) {
        iframeHead.appendChild(parentHeadNode.cloneNode(true));
      });

      var contents = React.createElement("div",
        undefined,
        this.props.head,
        this.props.children
      );

      React.render(contents, doc.body, this.fireOnContentsRendered.bind(this));
    } else {
      
      
      if (window.queuedFrames.indexOf(this) === -1) {
        window.queuedFrames.push(this);
      }
      setTimeout(this.renderFrameContents.bind(this), 0);
    }
  },
  



























  fireOnContentsRendered: function() {
    if (!this.props.onContentsRendered) {
      return;
    }

    var contentWindow;
    try {
      contentWindow = this.getDOMNode().contentWindow;
      if (!contentWindow) {
        throw new Error("no content window returned");
      }

    } catch (ex) {
      console.error("exception getting content window", ex);
    }

    
    
    
    setTimeout(this.props.onContentsRendered.bind(undefined, contentWindow),
               3000);
  },
  componentDidUpdate: function() {
    this.renderFrameContents();
  },
  componentWillUnmount: function() {
    React.unmountComponentAtNode(React.findDOMNode(this).contentDocument.body);
  }
});
