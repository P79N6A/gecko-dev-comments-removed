






















package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Inline;
import nu.validator.htmlparser.annotation.Local;
import nu.validator.htmlparser.annotation.NsUri;

final class StackNode<T> {
    final int flags;

    final @Local String name;

    final @Local String popName;

    final @NsUri String ns;

    final T node;

    
    HtmlAttributes attributes;

    private int refcount = 1;

    @Inline public int getFlags() {
        return flags;
    }

    public int getGroup() {
        return flags & ElementName.GROUP_MASK;
    }

    public boolean isScoping() {
        return (flags & ElementName.SCOPING) != 0;
    }

    public boolean isSpecial() {
        return (flags & ElementName.SPECIAL) != 0;
    }

    public boolean isFosterParenting() {
        return (flags & ElementName.FOSTER_PARENTING) != 0;
    }

    public boolean isHtmlIntegrationPoint() {
        return (flags & ElementName.HTML_INTEGRATION_POINT) != 0;
    }
    
    











    StackNode(int flags, @NsUri String ns, @Local String name,
            T node, @Local String popName, HtmlAttributes attributes) {
        this.flags = flags;
        this.name = name;
        this.popName = popName;
        this.ns = ns;
        this.node = node;
        this.attributes = attributes;
        this.refcount = 1;
    }

    





    StackNode(ElementName elementName, T node) {
        this.flags = elementName.getFlags();
        this.name = elementName.name;
        this.popName = elementName.name;
        this.ns = "http://www.w3.org/1999/xhtml";
        this.node = node;
        this.attributes = null;
        this.refcount = 1;
        assert !elementName.isCustom() : "Don't use this constructor for custom elements.";
    }

    






    StackNode(ElementName elementName, T node, HtmlAttributes attributes) {
        this.flags = elementName.getFlags();
        this.name = elementName.name;
        this.popName = elementName.name;
        this.ns = "http://www.w3.org/1999/xhtml";
        this.node = node;
        this.attributes = attributes;
        this.refcount = 1;
        assert !elementName.isCustom() : "Don't use this constructor for custom elements.";
    }

    






    StackNode(ElementName elementName, T node, @Local String popName) {
        this.flags = elementName.getFlags();
        this.name = elementName.name;
        this.popName = popName;
        this.ns = "http://www.w3.org/1999/xhtml";
        this.node = node;
        this.attributes = null;
        this.refcount = 1;
    }

    









    StackNode(ElementName elementName, @Local String popName, T node) {
        this.flags = prepareSvgFlags(elementName.getFlags());
        this.name = elementName.name;
        this.popName = popName;
        this.ns = "http://www.w3.org/2000/svg";
        this.node = node;
        this.attributes = null;
        this.refcount = 1;
    }

    







    StackNode(ElementName elementName, T node, @Local String popName,
            boolean markAsIntegrationPoint) {
        this.flags = prepareMathFlags(elementName.getFlags(), markAsIntegrationPoint);
        this.name = elementName.name;
        this.popName = popName;
        this.ns = "http://www.w3.org/1998/Math/MathML";
        this.node = node;
        this.attributes = null;
        this.refcount = 1;
    }
    
    private static int prepareSvgFlags(int flags) {
        flags &= ~(ElementName.FOSTER_PARENTING | ElementName.SCOPING | ElementName.SPECIAL);
        if ((flags & ElementName.SCOPING_AS_SVG) != 0) {
            flags |= (ElementName.SCOPING | ElementName.SPECIAL | ElementName.HTML_INTEGRATION_POINT);
        }
        return flags;
    }

    private static int prepareMathFlags(int flags, boolean markAsIntegrationPoint) {
        flags &= ~(ElementName.FOSTER_PARENTING | ElementName.SCOPING | ElementName.SPECIAL);
        if ((flags & ElementName.SCOPING_AS_MATHML) != 0) {
            flags |= (ElementName.SCOPING | ElementName.SPECIAL);
        }
        if (markAsIntegrationPoint) {
            flags |= ElementName.HTML_INTEGRATION_POINT;
        }
        return flags;
    }
    
    @SuppressWarnings("unused") private void destructor() {
        Portability.delete(attributes);
    }

    public void dropAttributes() {
        attributes = null;
    }

    
    


    @Override public @Local String toString() {
        return name;
    }

    

    public void retain() {
        refcount++;
    }

    public void release() {
        refcount--;
        if (refcount == 0) {
            Portability.delete(this);
        }
    }
}
