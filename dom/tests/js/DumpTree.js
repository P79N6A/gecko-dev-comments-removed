








































function traverse(node, indent)
{
    dump("\n")
    indent += "  "
    var type = node.nodeType;

    
    if (type == Node.ELEMENT_NODE) {

        dump(indent + node.tagName)

        
        if (node.hasChildNodes()) {
            var children = node.childNodes;
            var length = children.length;
            var count = 0;
            while(count < length) {
                child = children[count]
                traverse(child, indent)
                count++
            }
        }
    }
    
    else if (type == Node.TEXT_NODE) {
        dump(indent + "Text")
    }
}

var node = document.documentElement

traverse(node, "")
dump("\n")

  