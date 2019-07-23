








































function html(node)
{
    var type = node.nodeType;
    if (type == Node.ELEMENT_NODE) {

        
        dump("<" + node.tagName)

        
        attributes = node.attributes;
        if (null != attributes) {
            var countAttrs = attributes.length;
            var index = 0
            while(index < countAttrs) {
                att = attributes[index];
                if (null != att) {
                    dump(" " + att.value)
                }
                index++
            }
        }

        
        dump(">")

        
        if (node.hasChildNodes()) {
            
            var children = node.childNodes;
            var length = children.length;
            var count = 0;
            while(count < length) {
                child = children[count]
                html(child)
                count++
            }
            dump("</" + node.tagName + ">");
        }

        
    }
    
    else if (type == Node.TEXT_NODE) {
        dump(node.data)
    }
}

html(document.documentElement)
dump("\n")
