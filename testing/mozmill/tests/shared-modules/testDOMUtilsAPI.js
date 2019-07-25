



































var MODULE_NAME = 'DOMUtilsAPI';








function nodeCollector(aRoot) {
  this._root = aRoot.wrappedJSObject ? aRoot.wrappedJSObject : aRoot;
  this._document = this._root.ownerDocument ? this._root.ownerDocument : this._root;
  this._nodes = [ ];
}




nodeCollector.prototype = {
  





  get elements() {
    var elements = [ ];

    Array.forEach(this._nodes, function(element) {
      elements.push(new elementslib.Elem(element));
    });

    return elements;
  },

  





  get nodes() {
    return this._nodes;
  },

  





  set nodes(aNodeList) {
    if (aNodeList) {
      this._nodes = [ ];

      Array.forEach(aNodeList, function(node) {
        this._nodes.push(node);
      }, this);
    }
  },

  





  get root() {
    return this._root;
  },

  





  set root(aRoot) {
    if (aRoot) {
      this._root = aRoot;
      this._nodes = [ ];
    }
  },

  












  filter : function nodeCollector_filter(aCallback, aThisObject) {
    if (!aCallback)
      throw new Error(arguments.callee.name + ": No callback specified");

    this.nodes = Array.filter(this.nodes, aCallback, aThisObject);

    return this;
  },

  











  filterByDOMProperty : function nodeCollector_filterByDOMProperty(aProperty, aValue) {
    return this.filter(function(node) {
      if (aProperty && aValue)
        return node.getAttribute(aProperty) == aValue;
      else if (aProperty)
        return node.hasAttribute(aProperty);
      else
        return true;
    });
  },

  











  filterByJSProperty : function nodeCollector_filterByJSProperty(aProperty, aValue) {
    return this.filter(function(node) {
      if (aProperty && aValue)
        return node.aProperty == aValue;
      else if (aProperty)
        return node.aProperty !== undefined;
      else
        return true;
    });
  },

  










  queryAnonymousNodes : function nodeCollector_queryAnonymousNodes(aAttribute, aValue) {
    var node = this._document.getAnonymousElementByAttribute(this._root,
                                                             aAttribute,
                                                             aValue);
    this.nodes = node ? [node] : [ ];

    return this;
  },

  








  queryNodes : function nodeCollector_queryNodes(aSelector) {
    this.nodes = this._root.querySelectorAll(aSelector);

    return this;
  }
}
