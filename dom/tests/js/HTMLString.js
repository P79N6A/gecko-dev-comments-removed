








































function htmlString(node, indent)
{
    var html = ""
    indent += "  "

    var type = node.nodeType
    if (type == Node.ELEMENT) {

        
        html += "\n" + indent + "<" + node.tagName

        
        attributes = node.attributes
        if (null != attributes) {
            var countAttrs = attributes.length
            var index = 0
            while(index < countAttrs) {
                att = attributes[index]
                if (null != att) {
                    html += " "
                    html += att.name + "=" + att.value;
                }
                index++
            }
        }

        
        html += ">"

        
        if (node.hasChildNodes) {
            
            var children = node.childNodes
            var length = children.length
            var count = 0;
            while(count < length) {
                child = children[count]
                html += htmlString(child, indent)
                count++
            }
        }

        
        html += "\n" + indent + "</" + node.tagName + ">"
    }
    
    else if (type == Node.TEXT) {
        html += node.data
    }

    return html;
}

htmlString(document.documentElement, "") 


  