











window.queuedFrames = [];












window.Frame = React.createClass({
  propTypes: {
    style: React.PropTypes.object,
    head: React.PropTypes.node,
    width: React.PropTypes.number,
    height: React.PropTypes.number,
    onContentsRendered: React.PropTypes.func,
    className: React.PropTypes.string,
    




    cssClass: React.PropTypes.string
  },
  render: function() {
    return React.createElement("iframe", {
      style: this.props.style,
      head: this.props.head,
      width: this.props.width,
      height: this.props.height,
      className: this.props.className
    });
  },
  componentDidMount: function() {
    this.renderFrameContents();
  },
  renderFrameContents: function() {
    function isStyleSheet(node) {
      return node.tagName.toLowerCase() === "link" &&
        node.getAttribute("rel") === "stylesheet";
    }

    var childDoc = this.getDOMNode().contentDocument;
    if (childDoc && childDoc.readyState === "complete") {
      
      window.queuedFrames.splice(window.queuedFrames.indexOf(this), 1);

      var iframeHead = childDoc.querySelector("head");
      var parentHeadChildren = document.querySelector("head").children;

      [].forEach.call(parentHeadChildren, function(parentHeadNode) {

        
        if (isStyleSheet(parentHeadNode)) {
          
          
          
          
          
          
          if (parentHeadNode.hasAttribute("class") &&
              parentHeadNode.getAttribute("class") !== this.props.cssClass) {
            return;
          }
        }

        iframeHead.appendChild(parentHeadNode.cloneNode(true));
      }.bind(this));

      var contents = React.createElement("div",
        undefined,
        this.props.head,
        this.props.children
      );

      React.render(contents, childDoc.body, this.fireOnContentsRendered);

      
      
      
      if (document.location.search === "?rtl=1") {
        childDoc.documentElement.setAttribute("lang", "ar");
        childDoc.documentElement.setAttribute("dir", "rtl");
      }
    } else {
      
      
      if (window.queuedFrames.indexOf(this) === -1) {
        window.queuedFrames.push(this);
      }
      setTimeout(this.renderFrameContents, 0);
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
