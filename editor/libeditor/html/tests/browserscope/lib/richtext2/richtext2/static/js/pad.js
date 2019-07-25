































function convertSelectionIndicators(pad) {
  
  
  
  
  if (/[^>][{}\|][^<>]/.test(pad) ||
      /^[{}\|][^<]/.test(pad) ||
      /[^>][{}\|]$/.test(pad) ||
      /^[{}\|]*$/.test(pad)) {
    throw SETUP_BAD_SELECTION_SPEC;
  }

  
  pad = pad.replace(/\{\>/g, ATTRNAME_SEL_START + '="1">');  
  pad = pad.replace(/\}\>/g, ATTRNAME_SEL_END + '="1">');  
  pad = pad.replace(/\|\>/g, ATTRNAME_SEL_START + '="1" ' + 
                             ATTRNAME_SEL_END + '="1">'); 

  
  pad = pad.replace('{', '<!--[-->');
  pad = pad.replace('}', '<!--]-->');
  pad = pad.replace('|', '<!--[--><!--]-->');

  
  
  pad = pad.replace(/\^/, '[]');

  return pad;
}










function deriveSelectionPoint(root, marker) {
  switch (root.nodeType) {
    case DOM_NODE_TYPE_ELEMENT:
      if (root.attributes) {
        
        if (marker == '[' && root.getAttribute(ATTRNAME_SEL_START)) {
          root.removeAttribute(ATTRNAME_SEL_START);
          return {node: root, offs: 0};
        }
        if (marker == ']' && root.getAttribute(ATTRNAME_SEL_END)) {
          root.removeAttribute(ATTRNAME_SEL_END);
          return {node: root, offs: 0};
        }
      }
      for (var i = 0; i < root.childNodes.length; ++i) {
        var pair = deriveSelectionPoint(root.childNodes[i], marker);
        if (pair.node) {
          return pair;
        }
      }
      break;
      
    case DOM_NODE_TYPE_TEXT:
      var pos = root.data.indexOf(marker);
      if (pos != -1) {
        
        var nodeText = root.data;
        root.data = nodeText.substr(0, pos) + nodeText.substr(pos + 1);
        return {node: root, offs: pos };
      }
      break;

    case DOM_NODE_TYPE_COMMENT:
      var pos = root.data.indexOf(marker);
      if (pos != -1) {
        
        var helper = root.previousSibling;

        for (pos = 0; helper; ++pos ) {
          helper = helper.previousSibling;
        }
        helper = root;
        root = root.parentNode;
        root.removeChild(helper);
        return {node: root, offs: pos };
      }
      break;
  }

  return {node: null, offs: 0 };
}

























function initContainer(suite, group, test, container) {
  var pad = getTestParameter(suite, group, test, PARAM_PAD);
  pad = canonicalizeSpaces(pad);
  pad = convertSelectionIndicators(pad);

  if (container.editorID) {
    container.body.innerHTML = container.canary + container.tagOpen + pad + container.tagClose + container.canary;
    container.editor = container.doc.getElementById(container.editorID);
  } else {
    container.body.innerHTML = pad;
    container.editor = container.body;
  }

  win = container.win;
  doc = container.doc;
  body = container.body;
  editor = container.editor;
  sel = null;

  if (!editor) {
    throw SETUP_CONTAINER;
  }

  if (getTestParameter(suite, group, test, PARAM_STYLE_WITH_CSS)) {
    try {
      container.doc.execCommand('styleWithCSS', false, true);
    } catch (ex) {
      
    }
  }

  var selAnchor = deriveSelectionPoint(editor, '[');
  var selFocus  = deriveSelectionPoint(editor, ']');

  
  if (!selAnchor || !selFocus) {
    throw SETUP_SELECTION;
  }

  if (!selAnchor.node || !selFocus.node) {
    if (selAnchor.node || selFocus.node) {
      
      throw SETUP_BAD_SELECTION_SPEC;
    }
    sel = null;
    return;
  }

  if (selAnchor.node === selFocus.node) {
    if (selAnchor.offs > selFocus.offs) {
      
      
      
      
      --selAnchor.offs;
    }

    if (selAnchor.offs === selFocus.offs) {
      createCaret(selAnchor.node, selAnchor.offs).select();
      try {
        sel = win.getSelection();
      } catch (ex) {
        sel = undefined;
      }
      return;
    }
  }

  createFromNodes(selAnchor.node, selAnchor.offs, selFocus.node, selFocus.offs).select();

  try {
    sel = win.getSelection();
  } catch (ex) {
    sel = undefined;
  }
}






function resetContainer(container) {
  
  container.body.removeAttribute('style');
  container.body.removeAttribute('color');
  container.body.removeAttribute('bgcolor');

  try {
    container.doc.execCommand('styleWithCSS', false, false);
  } catch (ex) {
    
  }
}




function initEditorDocs() {
  for (var c = 0; c < containers.length; ++c) {
    var container = containers[c];

    container.iframe = document.getElementById('iframe-' + container.id);
    container.win = container.iframe.contentWindow;
    container.doc = container.win.document;
    container.body = container.doc.body;
    

    
    try {
      container.win.getSelection().selectAllChildren(editor);
    } catch (ex) {
      
    }
    
    try {
      container.doc.execCommand('styleWithCSS', false, false);
    } catch (ex) {
      
    }
  }
}
