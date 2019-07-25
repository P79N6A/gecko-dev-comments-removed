






















package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Auto;
import nu.validator.htmlparser.annotation.IdType;
import nu.validator.htmlparser.annotation.Local;
import nu.validator.htmlparser.annotation.NsUri;
import nu.validator.htmlparser.annotation.Prefix;
import nu.validator.htmlparser.annotation.QName;
import nu.validator.htmlparser.common.Interner;
import nu.validator.htmlparser.common.XmlViolationPolicy;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;








public final class HtmlAttributes implements Attributes {

    

    private static final AttributeName[] EMPTY_ATTRIBUTENAMES = new AttributeName[0];

    private static final String[] EMPTY_STRINGS = new String[0];

    

    public static final HtmlAttributes EMPTY_ATTRIBUTES = new HtmlAttributes(
            AttributeName.HTML);

    private int mode;

    private int length;

    private @Auto AttributeName[] names;

    private @Auto String[] values; 

    

    private String idValue;

    private int xmlnsLength;

    private AttributeName[] xmlnsNames;

    private String[] xmlnsValues;

    

    public HtmlAttributes(int mode) {
        this.mode = mode;
        this.length = 0;
        



        this.names = new AttributeName[5];
        this.values = new String[5];

        

        this.idValue = null;

        this.xmlnsLength = 0;

        this.xmlnsNames = HtmlAttributes.EMPTY_ATTRIBUTENAMES;

        this.xmlnsValues = HtmlAttributes.EMPTY_STRINGS;

        
    }
    














    void destructor() {
        clear(0);
    }
    
    





    public int getIndex(AttributeName name) {
        for (int i = 0; i < length; i++) {
            if (names[i] == name) {
                return i;
            }
        }
        return -1;
    }

    
    
    public int getIndex(String qName) {
        for (int i = 0; i < length; i++) {
            if (names[i].getQName(mode).equals(qName)) {
                return i;
            }
        }
        return -1;
    }
    
    public int getIndex(String uri, String localName) {
        for (int i = 0; i < length; i++) {
            if (names[i].getLocal(mode).equals(localName)
                    && names[i].getUri(mode).equals(uri)) {
                return i;
            }
        }
        return -1;
    }

    public @IdType String getType(String qName) {
        int index = getIndex(qName);
        if (index == -1) {
            return null;
        } else {
            return getType(index);
        }
    }

    public @IdType String getType(String uri, String localName) {
        int index = getIndex(uri, localName);
        if (index == -1) {
            return null;
        } else {
            return getType(index);
        }
    }
    
    public String getValue(String qName) {
        int index = getIndex(qName);
        if (index == -1) {
            return null;
        } else {
            return getValue(index);
        }
    }

    public String getValue(String uri, String localName) {
        int index = getIndex(uri, localName);
        if (index == -1) {
            return null;
        } else {
            return getValue(index);
        }
    }
    
    
    
    public int getLength() {
        return length;
    }

    public @Local String getLocalName(int index) {
        if (index < length && index >= 0) {
            return names[index].getLocal(mode);
        } else {
            return null;
        }
    }

    
    
    public @QName String getQName(int index) {
        if (index < length && index >= 0) {
            return names[index].getQName(mode);
        } else {
            return null;
        }
    }

    public @IdType String getType(int index) {
        if (index < length && index >= 0) {
            return (names[index] == AttributeName.ID) ? "ID" : "CDATA";
        } else {
            return null;
        }
    }

    
    
    public AttributeName getAttributeName(int index) {
        if (index < length && index >= 0) {
            return names[index];
        } else {
            return null;
        }
    }

    public @NsUri String getURI(int index) {
        if (index < length && index >= 0) {
            return names[index].getUri(mode);
        } else {
            return null;
        }
    }

    public @Prefix String getPrefix(int index) {
        if (index < length && index >= 0) {
            return names[index].getPrefix(mode);
        } else {
            return null;
        }
    }

    public String getValue(int index) {
        if (index < length && index >= 0) {
            return values[index];
        } else {
            return null;
        }
    }

    




    public String getValue(AttributeName name) {
        int index = getIndex(name);
        if (index == -1) {
            return null;
        } else {
            return getValue(index);
        }
    }
    
    

    public String getId() {
        return idValue;
    }

    public int getXmlnsLength() {
        return xmlnsLength;
    }

    public @Local String getXmlnsLocalName(int index) {
        if (index < xmlnsLength && index >= 0) {
            return xmlnsNames[index].getLocal(mode);
        } else {
            return null;
        }
    }

    public @NsUri String getXmlnsURI(int index) {
        if (index < xmlnsLength && index >= 0) {
            return xmlnsNames[index].getUri(mode);
        } else {
            return null;
        }
    }

    public String getXmlnsValue(int index) {
        if (index < xmlnsLength && index >= 0) {
            return xmlnsValues[index];
        } else {
            return null;
        }
    }
    
    public int getXmlnsIndex(AttributeName name) {
        for (int i = 0; i < xmlnsLength; i++) {
            if (xmlnsNames[i] == name) {
                return i;
            }
        }
        return -1;
    }
    
    public String getXmlnsValue(AttributeName name) {
        int index = getXmlnsIndex(name);
        if (index == -1) {
            return null;
        } else {
            return getXmlnsValue(index);
        }
    }
    
    public AttributeName getXmlnsAttributeName(int index) {
        if (index < xmlnsLength && index >= 0) {
            return xmlnsNames[index];
        } else {
            return null;
        }
    }

    

    void addAttribute(AttributeName name, String value
            
            , XmlViolationPolicy xmlnsPolicy
    
    ) throws SAXException {
        
        if (name == AttributeName.ID) {
            idValue = value;
        }

        if (name.isXmlns()) {
            if (xmlnsNames.length == xmlnsLength) {
                int newLen = xmlnsLength == 0 ? 2 : xmlnsLength << 1;
                AttributeName[] newNames = new AttributeName[newLen];
                System.arraycopy(xmlnsNames, 0, newNames, 0, xmlnsNames.length);
                xmlnsNames = newNames;
                String[] newValues = new String[newLen];
                System.arraycopy(xmlnsValues, 0, newValues, 0, xmlnsValues.length);
                xmlnsValues = newValues;
            }
            xmlnsNames[xmlnsLength] = name;
            xmlnsValues[xmlnsLength] = value;
            xmlnsLength++;
            switch (xmlnsPolicy) {
                case FATAL:
                    
                    throw new SAXException("Saw an xmlns attribute.");
                case ALTER_INFOSET:
                    return;
                case ALLOW:
                    
            }
        }

        

        if (names.length == length) {
            int newLen = length << 1; 
            
            
            AttributeName[] newNames = new AttributeName[newLen];
            System.arraycopy(names, 0, newNames, 0, names.length);
            names = newNames;
            String[] newValues = new String[newLen];
            System.arraycopy(values, 0, newValues, 0, values.length);
            values = newValues;
        }
        names[length] = name;
        values[length] = value;
        length++;
    }

    void clear(int m) {
        for (int i = 0; i < length; i++) {
            names[i].release();
            names[i] = null;
            Portability.releaseString(values[i]);
            values[i] = null;
        }
        length = 0;
        mode = m;
        
        idValue = null;
        for (int i = 0; i < xmlnsLength; i++) {
            xmlnsNames[i] = null;
            xmlnsValues[i] = null;
        }
        xmlnsLength = 0;
        
    }
    
    



    void releaseValue(int i) {
        Portability.releaseString(values[i]);        
    }
    
    



    void clearWithoutReleasingContents() {
        for (int i = 0; i < length; i++) {
            names[i] = null;
            values[i] = null;
        }
        length = 0;
    }

    boolean contains(AttributeName name) {
        for (int i = 0; i < length; i++) {
            if (name.equalsAnother(names[i])) {
                return true;
            }
        }
        
        for (int i = 0; i < xmlnsLength; i++) {
            if (name.equalsAnother(xmlnsNames[i])) {
                return true;
            }
        }
        
        return false;
    }

    public void adjustForMath() {
        mode = AttributeName.MATHML;
    }

    public void adjustForSvg() {
        mode = AttributeName.SVG;
    }

    public HtmlAttributes cloneAttributes(Interner interner) throws SAXException {
        assert (length == 0 && xmlnsLength == 0) || mode == 0 || mode == 3;
        HtmlAttributes clone = new HtmlAttributes(0);
        for (int i = 0; i < length; i++) {
            clone.addAttribute(names[i].cloneAttributeName(interner), Portability.newStringFromString(values[i])
            
                   , XmlViolationPolicy.ALLOW
            
            );
        }
        
        for (int i = 0; i < xmlnsLength; i++) {
            clone.addAttribute(xmlnsNames[i],
                    xmlnsValues[i], XmlViolationPolicy.ALLOW);
        }
        
        return clone; 
    }
    
    public boolean equalsAnother(HtmlAttributes other) {
        assert mode == 0 || mode == 3 : "Trying to compare attributes in foreign content.";
        int otherLength = other.getLength();
        if (length != otherLength) {
            return false;
        }
        for (int i = 0; i < length; i++) {
            
            boolean found = false;
            
            
            @Local String ownLocal = names[i].getLocal(AttributeName.HTML);
            for (int j = 0; j < otherLength; j++) {
                if (ownLocal == other.names[j].getLocal(AttributeName.HTML)) {
                    found = true;
                    if (!Portability.stringEqualsString(values[i], other.values[j])) {
                        return false;
                    }
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }
    
    
    
    void processNonNcNames(TreeBuilder<?> treeBuilder, XmlViolationPolicy namePolicy) throws SAXException {
        for (int i = 0; i < length; i++) {
            AttributeName attName = names[i];
            if (!attName.isNcName(mode)) {
                String name = attName.getLocal(mode);
                switch (namePolicy) {
                    case ALTER_INFOSET:
                        names[i] = AttributeName.create(NCName.escapeName(name));
                        
                    case ALLOW:
                        if (attName != AttributeName.XML_LANG) {
                            treeBuilder.warn("Attribute \u201C" + name + "\u201D is not serializable as XML 1.0.");
                        }
                        break;
                    case FATAL:
                        treeBuilder.fatal("Attribute \u201C" + name + "\u201D is not serializable as XML 1.0.");
                        break;
                }
            }
        }
    }
    
    public void merge(HtmlAttributes attributes) throws SAXException {
        int len = attributes.getLength();
        for (int i = 0; i < len; i++) {
            AttributeName name = attributes.getAttributeName(i);
            if (!contains(name)) {
                addAttribute(name, attributes.getValue(i), XmlViolationPolicy.ALLOW);
            }
        }
    }


    
    
}
