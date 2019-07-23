


































package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Inline;
import nu.validator.htmlparser.annotation.Local;
import nu.validator.htmlparser.annotation.NoLength;
import nu.validator.htmlparser.common.EncodingDeclarationHandler;
import nu.validator.htmlparser.common.TokenHandler;
import nu.validator.htmlparser.common.XmlViolationPolicy;

import org.xml.sax.ErrorHandler;
import org.xml.sax.Locator;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
















public class Tokenizer implements Locator {

    public static final int DATA = 0;

    public static final int RCDATA = 1;

    public static final int CDATA = 2;

    public static final int PLAINTEXT = 3;

    private static final int TAG_OPEN = 4;

    private static final int CLOSE_TAG_OPEN_PCDATA = 5;

    private static final int TAG_NAME = 6;

    private static final int BEFORE_ATTRIBUTE_NAME = 7;

    private static final int ATTRIBUTE_NAME = 8;

    private static final int AFTER_ATTRIBUTE_NAME = 9;

    private static final int BEFORE_ATTRIBUTE_VALUE = 10;

    private static final int ATTRIBUTE_VALUE_DOUBLE_QUOTED = 11;

    private static final int ATTRIBUTE_VALUE_SINGLE_QUOTED = 12;

    private static final int ATTRIBUTE_VALUE_UNQUOTED = 13;

    private static final int AFTER_ATTRIBUTE_VALUE_QUOTED = 14;

    private static final int BOGUS_COMMENT = 15;

    private static final int MARKUP_DECLARATION_OPEN = 16;

    private static final int DOCTYPE = 17;

    private static final int BEFORE_DOCTYPE_NAME = 18;

    private static final int DOCTYPE_NAME = 19;

    private static final int AFTER_DOCTYPE_NAME = 20;

    private static final int BEFORE_DOCTYPE_PUBLIC_IDENTIFIER = 21;

    private static final int DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED = 22;

    private static final int DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED = 23;

    private static final int AFTER_DOCTYPE_PUBLIC_IDENTIFIER = 24;

    private static final int BEFORE_DOCTYPE_SYSTEM_IDENTIFIER = 25;

    private static final int DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED = 26;

    private static final int DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED = 27;

    private static final int AFTER_DOCTYPE_SYSTEM_IDENTIFIER = 28;

    private static final int BOGUS_DOCTYPE = 29;

    private static final int COMMENT_START = 30;

    private static final int COMMENT_START_DASH = 31;

    private static final int COMMENT = 32;

    private static final int COMMENT_END_DASH = 33;

    private static final int COMMENT_END = 34;

    private static final int COMMENT_END_SPACE = 35;

    private static final int COMMENT_END_BANG = 36;

    private static final int CLOSE_TAG_OPEN_NOT_PCDATA = 37;

    private static final int MARKUP_DECLARATION_HYPHEN = 38;

    private static final int MARKUP_DECLARATION_OCTYPE = 39;

    private static final int DOCTYPE_UBLIC = 40;

    private static final int DOCTYPE_YSTEM = 41;

    private static final int CONSUME_CHARACTER_REFERENCE = 42;

    private static final int CONSUME_NCR = 43;

    private static final int CHARACTER_REFERENCE_LOOP = 44;

    private static final int HEX_NCR_LOOP = 45;

    private static final int DECIMAL_NRC_LOOP = 46;

    private static final int HANDLE_NCR_VALUE = 47;

    private static final int SELF_CLOSING_START_TAG = 48;

    private static final int CDATA_START = 49;

    private static final int CDATA_SECTION = 50;

    private static final int CDATA_RSQB = 51;

    private static final int CDATA_RSQB_RSQB = 52;

    private static final int TAG_OPEN_NON_PCDATA = 53;

    private static final int ESCAPE_EXCLAMATION = 54;

    private static final int ESCAPE_EXCLAMATION_HYPHEN = 55;

    private static final int ESCAPE = 56;

    private static final int ESCAPE_HYPHEN = 57;

    private static final int ESCAPE_HYPHEN_HYPHEN = 58;

    private static final int BOGUS_COMMENT_HYPHEN = 59;

    


    private static final int LEAD_OFFSET = (0xD800 - (0x10000 >> 10));

    



    private static final @NoLength char[] LT_GT = { '<', '>' };

    



    private static final @NoLength char[] LT_SOLIDUS = { '<', '/' };

    



    private static final @NoLength char[] RSQB_RSQB = { ']', ']' };

    


    private static final @NoLength char[] REPLACEMENT_CHARACTER = { '\uFFFD' };

    

    


    private static final @NoLength char[] SPACE = { ' ' };

    

    


    private static final @NoLength char[] LF = { '\n' };

    


    private static final int BUFFER_GROW_BY = 1024;

    


    private static final @NoLength char[] CDATA_LSQB = "CDATA[".toCharArray();

    


    private static final @NoLength char[] OCTYPE = "octype".toCharArray();

    


    private static final @NoLength char[] UBLIC = "ublic".toCharArray();

    


    private static final @NoLength char[] YSTEM = "ystem".toCharArray();

    private static final char[] TITLE_ARR = { 't', 'i', 't', 'l', 'e' };

    private static final char[] SCRIPT_ARR = { 's', 'c', 'r', 'i', 'p', 't' };

    private static final char[] STYLE_ARR = { 's', 't', 'y', 'l', 'e' };

    private static final char[] PLAINTEXT_ARR = { 'p', 'l', 'a', 'i', 'n', 't',
            'e', 'x', 't' };

    private static final char[] XMP_ARR = { 'x', 'm', 'p' };

    private static final char[] TEXTAREA_ARR = { 't', 'e', 'x', 't', 'a', 'r',
            'e', 'a' };

    private static final char[] IFRAME_ARR = { 'i', 'f', 'r', 'a', 'm', 'e' };

    private static final char[] NOEMBED_ARR = { 'n', 'o', 'e', 'm', 'b', 'e',
            'd' };

    private static final char[] NOSCRIPT_ARR = { 'n', 'o', 's', 'c', 'r', 'i',
            'p', 't' };

    private static final char[] NOFRAMES_ARR = { 'n', 'o', 'f', 'r', 'a', 'm',
            'e', 's' };

    


    protected final TokenHandler tokenHandler;

    protected EncodingDeclarationHandler encodingDeclarationHandler;

    

    


    protected ErrorHandler errorHandler;

    

    


    protected boolean lastCR;

    protected int stateSave;

    private int returnStateSave;

    protected int index;

    private boolean forceQuirks;

    private char additional;

    private int entCol;

    private int lo;

    private int hi;

    private int candidate;

    private int strBufMark;

    private int prevValue;

    protected int value;

    private boolean seenDigits;

    protected int cstart;

    



    private String publicId;

    



    private String systemId;

    


    private char[] strBuf;

    


    private int strBufLen;

    



    
    


    private char[] longStrBuf;

    


    private int longStrBufLen;

    



    
    


    private HtmlAttributes attributes;

    


    private final char[] bmpChar;

    


    private final char[] astralChar;

    


    protected ElementName contentModelElement = null;

    private char[] contentModelElementNameAsArray;

    


    protected boolean endTag;

    


    private ElementName tagName = null;

    


    protected AttributeName attributeName = null;

    

    


    private boolean wantsComments = false;

    


    protected boolean html4;

    


    private boolean metaBoundaryPassed;

    

    


    private @Local String doctypeName;

    


    private String publicIdentifier;

    


    private String systemIdentifier;

    

    


    private XmlViolationPolicy contentSpacePolicy = XmlViolationPolicy.ALTER_INFOSET;

    


    private XmlViolationPolicy commentPolicy = XmlViolationPolicy.ALTER_INFOSET;

    private XmlViolationPolicy xmlnsPolicy = XmlViolationPolicy.ALTER_INFOSET;

    private XmlViolationPolicy namePolicy = XmlViolationPolicy.ALTER_INFOSET;

    private boolean html4ModeCompatibleWithXhtml1Schemata;

    private final boolean newAttributesEachTime;

    

    private int mappingLangToXmlLang;

    private boolean shouldSuspend;

    protected boolean confident;

    private int line;

    

    protected LocatorImpl ampersandLocation;

    public Tokenizer(TokenHandler tokenHandler, boolean newAttributesEachTime) {
        this.tokenHandler = tokenHandler;
        this.encodingDeclarationHandler = null;
        this.newAttributesEachTime = newAttributesEachTime;
        this.bmpChar = new char[1];
        this.astralChar = new char[2];
    }

    

    





    public Tokenizer(TokenHandler tokenHandler) {
        this.tokenHandler = tokenHandler;
        this.encodingDeclarationHandler = null;
        
        this.newAttributesEachTime = false;
        
        this.bmpChar = new char[1];
        this.astralChar = new char[2];
    }

    public void initLocation(String newPublicId, String newSystemId) {
        this.systemId = newSystemId;
        this.publicId = newPublicId;

    }

    void destructor() {
        Portability.releaseArray(bmpChar);
        Portability.releaseArray(astralChar);
    }

    

    




    public boolean isMappingLangToXmlLang() {
        return mappingLangToXmlLang == AttributeName.HTML_LANG;
    }

    





    public void setMappingLangToXmlLang(boolean mappingLangToXmlLang) {
        this.mappingLangToXmlLang = mappingLangToXmlLang ? AttributeName.HTML_LANG
                : AttributeName.HTML;
    }

    




    public void setErrorHandler(ErrorHandler eh) {
        this.errorHandler = eh;
    }

    public ErrorHandler getErrorHandler() {
        return this.errorHandler;
    }

    





    public void setCommentPolicy(XmlViolationPolicy commentPolicy) {
        this.commentPolicy = commentPolicy;
    }

    





    public void setContentNonXmlCharPolicy(
            XmlViolationPolicy contentNonXmlCharPolicy) {
        if (contentNonXmlCharPolicy != XmlViolationPolicy.ALLOW) {
            throw new IllegalArgumentException(
                    "Must use ErrorReportingTokenizer to set contentNonXmlCharPolicy to non-ALLOW.");
        }
    }

    





    public void setContentSpacePolicy(XmlViolationPolicy contentSpacePolicy) {
        this.contentSpacePolicy = contentSpacePolicy;
    }

    





    public void setXmlnsPolicy(XmlViolationPolicy xmlnsPolicy) {
        if (xmlnsPolicy == XmlViolationPolicy.FATAL) {
            throw new IllegalArgumentException("Can't use FATAL here.");
        }
        this.xmlnsPolicy = xmlnsPolicy;
    }

    public void setNamePolicy(XmlViolationPolicy namePolicy) {
        this.namePolicy = namePolicy;
    }

    





    public void setHtml4ModeCompatibleWithXhtml1Schemata(
            boolean html4ModeCompatibleWithXhtml1Schemata) {
        this.html4ModeCompatibleWithXhtml1Schemata = html4ModeCompatibleWithXhtml1Schemata;
    }

    

    
    







    public void setContentModelFlag(int contentModelFlag,
            @Local String contentModelElement) {
        this.stateSave = contentModelFlag;
        if (contentModelFlag == Tokenizer.DATA) {
            return;
        }
        
        char[] asArray = Portability.newCharArrayFromLocal(contentModelElement);
        this.contentModelElement = ElementName.elementNameByBuffer(asArray, 0,
                asArray.length);
        Portability.releaseArray(asArray);
        contentModelElementToArray();
    }

    







    public void setContentModelFlag(int contentModelFlag,
            ElementName contentModelElement) {
        this.stateSave = contentModelFlag;
        this.contentModelElement = contentModelElement;
        contentModelElementToArray();
    }

    private void contentModelElementToArray() {
        switch (contentModelElement.group) {
            case TreeBuilder.TITLE:
                contentModelElementNameAsArray = TITLE_ARR;
                return;
            case TreeBuilder.SCRIPT:
                contentModelElementNameAsArray = SCRIPT_ARR;
                return;
            case TreeBuilder.STYLE:
                contentModelElementNameAsArray = STYLE_ARR;
                return;
            case TreeBuilder.PLAINTEXT:
                contentModelElementNameAsArray = PLAINTEXT_ARR;
                return;
            case TreeBuilder.XMP:
                contentModelElementNameAsArray = XMP_ARR;
                return;
            case TreeBuilder.TEXTAREA:
                contentModelElementNameAsArray = TEXTAREA_ARR;
                return;
            case TreeBuilder.IFRAME:
                contentModelElementNameAsArray = IFRAME_ARR;
                return;
            case TreeBuilder.NOEMBED:
                contentModelElementNameAsArray = NOEMBED_ARR;
                return;
            case TreeBuilder.NOSCRIPT:
                contentModelElementNameAsArray = NOSCRIPT_ARR;
                return;
            case TreeBuilder.NOFRAMES:
                contentModelElementNameAsArray = NOFRAMES_ARR;
                return;
            default:
                assert false;
                return;
        }
    }

    


    public void setLineNumber(int line) {
        this.line = line;
    }

    

    


    @Inline public int getLineNumber() {
        return line;
    }

    

    


    @Inline public int getColumnNumber() {
        return -1;
    }

    


    public String getPublicId() {
        return publicId;
    }

    


    public String getSystemId() {
        return systemId;
    }

    

    

    public void notifyAboutMetaBoundary() {
        metaBoundaryPassed = true;
    }

    void turnOnAdditionalHtml4Errors() {
        html4 = true;
    }

    

    HtmlAttributes emptyAttributes() {
        
        if (newAttributesEachTime) {
            return new HtmlAttributes(mappingLangToXmlLang);
        } else {
            
            return HtmlAttributes.EMPTY_ATTRIBUTES;
            
        }
        
    }

    private void clearStrBufAndAppendCurrentC(char c) {
        strBuf[0] = c;

        strBufLen = 1;
        
    }

    private void clearStrBufAndAppendForceWrite(char c) {
        strBuf[0] = c; 

        strBufLen = 1;
        
        
    }

    private void clearStrBufForNextState() {
        strBufLen = 0;
        
    }

    





    private void appendStrBuf(char c) {
        
        
        
        if (strBufLen == strBuf.length) {
            char[] newBuf = new char[strBuf.length + Tokenizer.BUFFER_GROW_BY];
            System.arraycopy(strBuf, 0, newBuf, 0, strBuf.length);
            Portability.releaseArray(strBuf);
            strBuf = newBuf;
        }
        strBuf[strBufLen++] = c;
        
    }

    





    private void appendStrBufForceWrite(char c) {
        
        
        
        
        if (strBufLen == strBuf.length) {
            char[] newBuf = new char[strBuf.length + Tokenizer.BUFFER_GROW_BY];
            System.arraycopy(strBuf, 0, newBuf, 0, strBuf.length);
            Portability.releaseArray(strBuf);
            strBuf = newBuf;
        }
        strBuf[strBufLen++] = c;
        
    }

    







    protected String strBufToString() {
        
        
        
        return Portability.newStringFromBuffer(strBuf, 0, strBufLen);
        
    }

    





    private void strBufToDoctypeName() {
        doctypeName = Portability.newLocalNameFromBuffer(strBuf, 0, strBufLen);
    }

    





    private void emitStrBuf() throws SAXException {
        if (strBufLen > 0) {
            
            
            
            tokenHandler.characters(strBuf, 0, strBufLen);
            
        }
    }

    private void clearLongStrBufForNextState() {
        
        longStrBufLen = 0;
    }

    private void clearLongStrBuf() {
        
        longStrBufLen = 0;
    }

    private void clearLongStrBufAndAppendCurrentC(char c) {
        longStrBuf[0] = c;
        longStrBufLen = 1;
        
    }

    private void clearLongStrBufAndAppendToComment(char c) {
        longStrBuf[0] = c;
        
        longStrBufLen = 1;
    }

    





    private void appendLongStrBuf(char c) {
        
        
        
        if (longStrBufLen == longStrBuf.length) {
            char[] newBuf = new char[longStrBufLen + (longStrBufLen >> 1)];
            System.arraycopy(longStrBuf, 0, newBuf, 0, longStrBuf.length);
            Portability.releaseArray(longStrBuf);
            longStrBuf = newBuf;
        }
        longStrBuf[longStrBufLen++] = c;
        
    }

    private void appendSecondHyphenToBogusComment() throws SAXException {
        
        switch (commentPolicy) {
            case ALTER_INFOSET:
                
                appendLongStrBuf(' ');
                
            case ALLOW:
                warn("The document is not mappable to XML 1.0 due to two consecutive hyphens in a comment.");
                
                appendLongStrBuf('-');
                
                break;
            case FATAL:
                fatal("The document is not mappable to XML 1.0 due to two consecutive hyphens in a comment.");
                break;
        }
        
    }

    
    private void maybeAppendSpaceToBogusComment() throws SAXException {
        switch (commentPolicy) {
            case ALTER_INFOSET:
                
                appendLongStrBuf(' ');
                
            case ALLOW:
                warn("The document is not mappable to XML 1.0 due to a trailing hyphen in a comment.");
                break;
            case FATAL:
                fatal("The document is not mappable to XML 1.0 due to a trailing hyphen in a comment.");
                break;
        }
    }

    

    private void adjustDoubleHyphenAndAppendToLongStrBufAndErr(char c)
            throws SAXException {
        errConsecutiveHyphens();
        
        switch (commentPolicy) {
            case ALTER_INFOSET:
                
                longStrBufLen--;
                appendLongStrBuf(' ');
                appendLongStrBuf('-');
                
            case ALLOW:
                warn("The document is not mappable to XML 1.0 due to two consecutive hyphens in a comment.");
                
                appendLongStrBuf(c);
                
                break;
            case FATAL:
                fatal("The document is not mappable to XML 1.0 due to two consecutive hyphens in a comment.");
                break;
        }
        
    }

    private void appendLongStrBuf(char[] buffer, int offset, int length) {
        int reqLen = longStrBufLen + length;
        if (longStrBuf.length < reqLen) {
            char[] newBuf = new char[reqLen + (reqLen >> 1)];
            System.arraycopy(longStrBuf, 0, newBuf, 0, longStrBuf.length);
            Portability.releaseArray(longStrBuf);
            longStrBuf = newBuf;
        }
        System.arraycopy(buffer, offset, longStrBuf, longStrBufLen, length);
        longStrBufLen = reqLen;
    }

    





    private void appendLongStrBuf(char[] arr) {
        
        appendLongStrBuf(arr, 0, arr.length);
    }

    


    private void appendStrBufToLongStrBuf() {
        
        
        
        
        appendLongStrBuf(strBuf, 0, strBufLen);
        
    }

    







    private String longStrBufToString() {
        
        
        
        
        return Portability.newStringFromBuffer(longStrBuf, 0, longStrBufLen);
        
    }

    







    private void emitComment(int provisionalHyphens, int pos)
            throws SAXException {
        
        if (wantsComments) {
            
            
            
            
            
            tokenHandler.comment(longStrBuf, 0, longStrBufLen
                    - provisionalHyphens);
            
            
        }
        
        cstart = pos + 1;
    }

    









    protected void flushChars(@NoLength char[] buf, int pos)
            throws SAXException {
        if (pos > cstart) {
            tokenHandler.characters(buf, cstart, pos - cstart);
        }
        cstart = 0x7fffffff;
    }

    








    public void fatal(String message) throws SAXException {
        SAXParseException spe = new SAXParseException(message, this);
        if (errorHandler != null) {
            errorHandler.fatalError(spe);
        }
        throw spe;
    }

    






    public void err(String message) throws SAXException {
        if (errorHandler == null) {
            return;
        }
        SAXParseException spe = new SAXParseException(message, this);
        errorHandler.error(spe);
    }

    public void errTreeBuilder(String message) throws SAXException {
        ErrorHandler eh = null;
        if (tokenHandler instanceof TreeBuilder<?>) {
            TreeBuilder<?> treeBuilder = (TreeBuilder<?>) tokenHandler;
            eh = treeBuilder.getErrorHandler();
        }
        if (eh == null) {
            eh = errorHandler;
        }
        if (eh == null) {
            return;
        }
        SAXParseException spe = new SAXParseException(message, this);
        eh.error(spe);
    }

    






    public void warn(String message) throws SAXException {
        if (errorHandler == null) {
            return;
        }
        SAXParseException spe = new SAXParseException(message, this);
        errorHandler.warning(spe);
    }

    


    private void resetAttributes() {
        
        if (newAttributesEachTime) {
            attributes = null;
        } else {
            
            attributes.clear(mappingLangToXmlLang);
            
        }
        
    }

    private void strBufToElementNameString() {
        
        
        
        tagName = ElementName.elementNameByBuffer(strBuf, 0, strBufLen);
        
    }

    private int emitCurrentTagToken(boolean selfClosing, int pos)
            throws SAXException {
        cstart = pos + 1;
        maybeErrSlashInEndTag(selfClosing);
        stateSave = Tokenizer.DATA;
        HtmlAttributes attrs = (attributes == null ? HtmlAttributes.EMPTY_ATTRIBUTES
                : attributes);
        if (endTag) {
            



            maybeErrAttributesOnEndTag(attrs);
            tokenHandler.endTag(tagName);
        } else {
            tokenHandler.startTag(tagName, attrs, selfClosing);
        }
        resetAttributes();
        return stateSave;
    }

    private void attributeNameComplete() throws SAXException {
        
        
        
        
        attributeName = AttributeName.nameByBuffer(strBuf, 0, strBufLen
        
                , namePolicy != XmlViolationPolicy.ALLOW
        
        );
        

        
        if (attributes == null) {
            attributes = new HtmlAttributes(mappingLangToXmlLang);
        }
        

        







        if (attributes.contains(attributeName)) {
            errDuplicateAttribute();
            attributeName.release();
            attributeName = null;
        }
    }

    private void addAttributeWithoutValue() throws SAXException {
        
        if (metaBoundaryPassed && AttributeName.CHARSET == attributeName
                && ElementName.META == tagName) {
            err("A \u201Ccharset\u201D attribute on a \u201Cmeta\u201D element found after the first 512 bytes.");
        }
        
        if (attributeName != null) {
            
            if (html4) {
                if (attributeName.isBoolean()) {
                    if (html4ModeCompatibleWithXhtml1Schemata) {
                        attributes.addAttribute(attributeName,
                                attributeName.getLocal(AttributeName.HTML),
                                xmlnsPolicy);
                    } else {
                        attributes.addAttribute(attributeName, "", xmlnsPolicy);
                    }
                } else {
                    err("Attribute value omitted for a non-boolean attribute. (HTML4-only error.)");
                    attributes.addAttribute(attributeName, "", xmlnsPolicy);
                }
            } else {
                if (AttributeName.SRC == attributeName
                        || AttributeName.HREF == attributeName) {
                    warn("Attribute \u201C"
                            + attributeName.getLocal(AttributeName.HTML)
                            + "\u201D without an explicit value seen. The attribute may be dropped by IE7.");
                }
                
                attributes.addAttribute(attributeName,
                        Portability.newEmptyString()
                        
                        , xmlnsPolicy
                
                );
                
            }
            
        }
    }

    private void addAttributeWithValue() throws SAXException {
        
        if (metaBoundaryPassed && ElementName.META == tagName
                && AttributeName.CHARSET == attributeName) {
            err("A \u201Ccharset\u201D attribute on a \u201Cmeta\u201D element found after the first 512 bytes.");
        }
        
        if (attributeName != null) {
            String value = longStrBufToString(); 
            
            
            if (!endTag && html4 && html4ModeCompatibleWithXhtml1Schemata
                    && attributeName.isCaseFolded()) {
                value = newAsciiLowerCaseStringFromString(value);
            }
            
            attributes.addAttribute(attributeName, value
            
                    , xmlnsPolicy
            
            );
        }
    }

    

    private static String newAsciiLowerCaseStringFromString(String str) {
        if (str == null) {
            return null;
        }
        char[] buf = new char[str.length()];
        for (int i = 0; i < str.length(); i++) {
            char c = str.charAt(i);
            if (c >= 'A' && c <= 'Z') {
                c += 0x20;
            }
            buf[i] = c;
        }
        return new String(buf);
    }

    

    protected void startErrorReporting() throws SAXException {

    }

    public void start() throws SAXException {
        confident = false;
        strBuf = new char[64];
        strBufLen = 0;
        longStrBuf = new char[1024];
        longStrBufLen = 0;
        stateSave = Tokenizer.DATA;
        line = 1;
        lastCR = false;
        
        html4 = false;
        metaBoundaryPassed = false;
        
        tokenHandler.startTokenization(this);
        
        wantsComments = tokenHandler.wantsComments();
        
        index = 0;
        forceQuirks = false;
        additional = '\u0000';
        entCol = -1;
        lo = 0;
        hi = (NamedCharacters.NAMES.length - 1);
        candidate = -1;
        strBufMark = 0;
        prevValue = -1;
        value = 0;
        seenDigits = false;
        shouldSuspend = false;
        
        if (!newAttributesEachTime) {
            
            attributes = new HtmlAttributes(mappingLangToXmlLang);
            
        }
        startErrorReporting();
        
    }

    public boolean tokenizeBuffer(UTF16Buffer buffer) throws SAXException {
        int state = stateSave;
        int returnState = returnStateSave;
        char c = '\u0000';
        shouldSuspend = false;
        lastCR = false;

        int start = buffer.getStart();
        


        int pos = start - 1;

        





        switch (state) {
            case DATA:
            case RCDATA:
            case CDATA:
            case PLAINTEXT:
            case CDATA_SECTION:
            case ESCAPE:
            case ESCAPE_EXCLAMATION:
            case ESCAPE_EXCLAMATION_HYPHEN:
            case ESCAPE_HYPHEN:
            case ESCAPE_HYPHEN_HYPHEN:
                cstart = start;
                break;
            default:
                cstart = Integer.MAX_VALUE;
                break;
        }

        




        pos = stateLoop(state, c, pos, buffer.getBuffer(), false, returnState,
                buffer.getEnd());
        if (pos == buffer.getEnd()) {
            
            buffer.setStart(pos);
        } else {
            buffer.setStart(pos + 1);
        }
        return lastCR;
    }

    
    
    private int stateLoop(int state, char c, int pos, @NoLength char[] buf,
            boolean reconsume, int returnState, int endPos) throws SAXException {
        stateloop: for (;;) {
            switch (state) {
                case DATA:
                    dataloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        switch (c) {
                            case '&':
                                







                                flushChars(buf, pos);
                                clearStrBufAndAppendCurrentC(c);
                                rememberAmpersandLocation('\u0000');
                                returnState = state;
                                state = Tokenizer.CONSUME_CHARACTER_REFERENCE;
                                continue stateloop;
                            case '<':
                                









                                flushChars(buf, pos);

                                state = Tokenizer.TAG_OPEN;
                                break dataloop; 
                            
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                continue;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                



                                


                                continue;
                        }
                    }
                    
                case TAG_OPEN:
                    tagopenloop: for (;;) {
                        



                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        



                        if (c >= 'A' && c <= 'Z') {
                            




                            endTag = false;
                            




                            clearStrBufAndAppendForceWrite((char) (c + 0x20));
                            
                            state = Tokenizer.TAG_NAME;
                            



                            break tagopenloop;
                            
                        } else if (c >= 'a' && c <= 'z') {
                            




                            endTag = false;
                            


                            clearStrBufAndAppendCurrentC(c);
                            
                            state = Tokenizer.TAG_NAME;
                            



                            break tagopenloop;
                            
                        }
                        switch (c) {
                            case '!':
                                



                                state = Tokenizer.MARKUP_DECLARATION_OPEN;
                                continue stateloop;
                            case '/':
                                



                                state = Tokenizer.CLOSE_TAG_OPEN_PCDATA;
                                continue stateloop;
                            case '?':
                                


                                errProcessingInstruction();
                                


                                clearLongStrBufAndAppendToComment(c);
                                state = Tokenizer.BOGUS_COMMENT;
                                continue stateloop;
                            case '>':
                                


                                errLtGt();
                                




                                tokenHandler.characters(Tokenizer.LT_GT, 0, 2);
                                
                                cstart = pos + 1;
                                state = Tokenizer.DATA;
                                continue stateloop;
                            default:
                                


                                errBadCharAfterLt(c);
                                


                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                



                                cstart = pos;
                                state = Tokenizer.DATA;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case TAG_NAME:
                    tagnameloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                strBufToElementNameString();
                                state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                strBufToElementNameString();
                                state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                break tagnameloop;
                            
                            case '/':
                                



                                strBufToElementNameString();
                                state = Tokenizer.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '>':
                                



                                strBufToElementNameString();
                                state = emitCurrentTagToken(false, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                if (c >= 'A' && c <= 'Z') {
                                    







                                    c += 0x20;
                                }
                                




                                appendStrBuf(c);
                                


                                continue;
                        }
                    }
                    
                case BEFORE_ATTRIBUTE_NAME:
                    beforeattributenameloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                continue;
                            case '/':
                                



                                state = Tokenizer.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '>':
                                



                                state = emitCurrentTagToken(false, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            case '\"':
                            case '\'':
                            case '<':
                            case '=':
                                




                                errBadCharBeforeAttributeNameOrNull(c);
                                



                            default:
                                



                                if (c >= 'A' && c <= 'Z') {
                                    






                                    c += 0x20;
                                }
                                



                                clearStrBufAndAppendCurrentC(c);
                                


                                
                                


                                state = Tokenizer.ATTRIBUTE_NAME;
                                break beforeattributenameloop;
                            
                        }
                    }
                    
                case ATTRIBUTE_NAME:
                    attributenameloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                attributeNameComplete();
                                state = Tokenizer.AFTER_ATTRIBUTE_NAME;
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                attributeNameComplete();
                                state = Tokenizer.AFTER_ATTRIBUTE_NAME;
                                continue stateloop;
                            case '/':
                                



                                attributeNameComplete();
                                addAttributeWithoutValue();
                                state = Tokenizer.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '=':
                                



                                attributeNameComplete();
                                state = Tokenizer.BEFORE_ATTRIBUTE_VALUE;
                                break attributenameloop;
                            
                            case '>':
                                



                                attributeNameComplete();
                                addAttributeWithoutValue();
                                state = emitCurrentTagToken(false, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            case '\"':
                            case '\'':
                            case '<':
                                



                                errQuoteOrLtInAttributeNameOrNull(c);
                                



                            default:
                                if (c >= 'A' && c <= 'Z') {
                                    







                                    c += 0x20;
                                }
                                



                                appendStrBuf(c);
                                


                                continue;
                        }
                    }
                    
                case BEFORE_ATTRIBUTE_VALUE:
                    beforeattributevalueloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                continue;
                            case '"':
                                



                                clearLongStrBufForNextState();
                                state = Tokenizer.ATTRIBUTE_VALUE_DOUBLE_QUOTED;
                                break beforeattributevalueloop;
                            
                            case '&':
                                




                                clearLongStrBuf();
                                state = Tokenizer.ATTRIBUTE_VALUE_UNQUOTED;
                                reconsume = true;
                                continue stateloop;
                            case '\'':
                                



                                clearLongStrBufForNextState();
                                state = Tokenizer.ATTRIBUTE_VALUE_SINGLE_QUOTED;
                                continue stateloop;
                            case '>':
                                


                                errAttributeValueMissing();
                                


                                addAttributeWithoutValue();
                                state = emitCurrentTagToken(false, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            case '<':
                            case '=':
                                


                                errLtOrEqualsInUnquotedAttributeOrNull(c);
                                



                            default:
                                
                                errHtml4NonNameInUnquotedAttribute(c);
                                
                                



                                clearLongStrBufAndAppendCurrentC(c);
                                




                                state = Tokenizer.ATTRIBUTE_VALUE_UNQUOTED;
                                continue stateloop;
                        }
                    }
                    
                case ATTRIBUTE_VALUE_DOUBLE_QUOTED:
                    attributevaluedoublequotedloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '"':
                                



                                addAttributeWithValue();

                                state = Tokenizer.AFTER_ATTRIBUTE_VALUE_QUOTED;
                                break attributevaluedoublequotedloop;
                            
                            case '&':
                                





                                clearStrBufAndAppendCurrentC(c);
                                rememberAmpersandLocation('\"');
                                returnState = state;
                                state = Tokenizer.CONSUME_CHARACTER_REFERENCE;
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                



                                appendLongStrBuf(c);
                                



                                continue;
                        }
                    }
                    
                case AFTER_ATTRIBUTE_VALUE_QUOTED:
                    afterattributevaluequotedloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                continue stateloop;
                            case '/':
                                



                                state = Tokenizer.SELF_CLOSING_START_TAG;
                                break afterattributevaluequotedloop;
                            
                            case '>':
                                



                                state = emitCurrentTagToken(false, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            default:
                                


                                errNoSpaceBetweenAttributes();
                                



                                state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case SELF_CLOSING_START_TAG:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    


                    switch (c) {
                        case '>':
                            




                            
                            errHtml4XmlVoidSyntax();
                            
                            state = emitCurrentTagToken(true, pos);
                            if (shouldSuspend) {
                                break stateloop;
                            }
                            


                            continue stateloop;
                        default:
                            
                            errSlashNotFollowedByGt();
                            



                            state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                            reconsume = true;
                            continue stateloop;
                    }
                    
                case ATTRIBUTE_VALUE_UNQUOTED:
                    for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                addAttributeWithValue();
                                state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                addAttributeWithValue();
                                state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                continue stateloop;
                            case '&':
                                




                                clearStrBufAndAppendCurrentC(c);
                                rememberAmpersandLocation('\u0000');
                                returnState = state;
                                state = Tokenizer.CONSUME_CHARACTER_REFERENCE;
                                continue stateloop;
                            case '>':
                                



                                addAttributeWithValue();
                                state = emitCurrentTagToken(false, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            case '<':
                            case '\"':
                            case '\'':
                            case '=':
                                



                                errUnquotedAttributeValOrNull(c);
                                



                                
                            default:
                                
                                errHtml4NonNameInUnquotedAttribute(c);
                                
                                



                                appendLongStrBuf(c);
                                


                                continue;
                        }
                    }
                    
                case AFTER_ATTRIBUTE_NAME:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                continue;
                            case '/':
                                



                                addAttributeWithoutValue();
                                state = Tokenizer.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '=':
                                



                                state = Tokenizer.BEFORE_ATTRIBUTE_VALUE;
                                continue stateloop;
                            case '>':
                                



                                addAttributeWithoutValue();
                                state = emitCurrentTagToken(false, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            case '\"':
                            case '\'':
                            case '<':
                                errQuoteOrLtInAttributeNameOrNull(c);
                                



                            default:
                                addAttributeWithoutValue();
                                



                                if (c >= 'A' && c <= 'Z') {
                                    






                                    c += 0x20;
                                }
                                



                                clearStrBufAndAppendCurrentC(c);
                                


                                
                                


                                state = Tokenizer.ATTRIBUTE_NAME;
                                continue stateloop;
                        }
                    }
                    
                case BOGUS_COMMENT:
                    boguscommentloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        




















                        switch (c) {
                            case '>':
                                emitComment(0, pos);
                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '-':
                                appendLongStrBuf(c);
                                state = Tokenizer.BOGUS_COMMENT_HYPHEN;
                                break boguscommentloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                appendLongStrBuf(c);
                                continue;
                        }
                    }
                    
                case BOGUS_COMMENT_HYPHEN:
                    boguscommenthyphenloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '>':
                                
                                maybeAppendSpaceToBogusComment();
                                
                                emitComment(0, pos);
                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '-':
                                appendSecondHyphenToBogusComment();
                                continue boguscommenthyphenloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                state = Tokenizer.BOGUS_COMMENT;
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                state = Tokenizer.BOGUS_COMMENT;
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                appendLongStrBuf(c);
                                state = Tokenizer.BOGUS_COMMENT;
                                continue stateloop;
                        }
                    }
                    
                case MARKUP_DECLARATION_OPEN:
                    markupdeclarationopenloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        




























                        switch (c) {
                            case '-':
                                clearLongStrBufAndAppendToComment(c);
                                state = Tokenizer.MARKUP_DECLARATION_HYPHEN;
                                break markupdeclarationopenloop;
                            
                            case 'd':
                            case 'D':
                                clearLongStrBufAndAppendToComment(c);
                                index = 0;
                                state = Tokenizer.MARKUP_DECLARATION_OCTYPE;
                                continue stateloop;
                            case '[':
                                if (tokenHandler.inForeign()) {
                                    clearLongStrBufAndAppendToComment(c);
                                    index = 0;
                                    state = Tokenizer.CDATA_START;
                                    continue stateloop;
                                } else {
                                    
                                }
                            default:
                                errBogusComment();
                                clearLongStrBuf();
                                state = Tokenizer.BOGUS_COMMENT;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case MARKUP_DECLARATION_HYPHEN:
                    markupdeclarationhyphenloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '\u0000':
                                break stateloop;
                            case '-':
                                clearLongStrBufForNextState();
                                state = Tokenizer.COMMENT_START;
                                break markupdeclarationhyphenloop;
                            
                            default:
                                errBogusComment();
                                state = Tokenizer.BOGUS_COMMENT;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case COMMENT_START:
                    commentstartloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        





                        switch (c) {
                            case '-':
                                



                                appendLongStrBuf(c);
                                state = Tokenizer.COMMENT_START_DASH;
                                continue stateloop;
                            case '>':
                                


                                errPrematureEndOfComment();
                                
                                emitComment(0, pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                state = Tokenizer.COMMENT;
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                state = Tokenizer.COMMENT;
                                break commentstartloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                



                                appendLongStrBuf(c);
                                


                                state = Tokenizer.COMMENT;
                                break commentstartloop;
                            
                        }
                    }
                    
                case COMMENT:
                    commentloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '-':
                                



                                appendLongStrBuf(c);
                                state = Tokenizer.COMMENT_END_DASH;
                                break commentloop;
                            
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                



                                appendLongStrBuf(c);
                                


                                continue;
                        }
                    }
                    
                case COMMENT_END_DASH:
                    commentenddashloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        



                        switch (c) {
                            case '-':
                                



                                appendLongStrBuf(c);
                                state = Tokenizer.COMMENT_END;
                                break commentenddashloop;
                            
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                state = Tokenizer.COMMENT;
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                state = Tokenizer.COMMENT;
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                appendLongStrBuf(c);
                                


                                state = Tokenizer.COMMENT;
                                continue stateloop;
                        }
                    }
                    
                case COMMENT_END:
                    commentendloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        



                        switch (c) {
                            case '>':
                                



                                emitComment(2, pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '-':
                                
                                



                                adjustDoubleHyphenAndAppendToLongStrBufAndErr(c);
                                


                                continue;
                            case ' ':
                            case '\t':
                            case '\u000C':
                                





                                adjustDoubleHyphenAndAppendToLongStrBufAndErr(c);
                                state = Tokenizer.COMMENT_END_SPACE;
                                break commentendloop;
                            
                            case '\r':
                                adjustDoubleHyphenAndAppendToLongStrBufCarriageReturn();
                                state = Tokenizer.COMMENT_END_SPACE;
                                break stateloop;
                            case '\n':
                                adjustDoubleHyphenAndAppendToLongStrBufLineFeed();
                                state = Tokenizer.COMMENT_END_SPACE;
                                break commentendloop;
                            
                            case '!':
                                errHyphenHyphenBang();
                                appendLongStrBuf(c);
                                state = Tokenizer.COMMENT_END_BANG;
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                adjustDoubleHyphenAndAppendToLongStrBufAndErr(c);
                                


                                state = Tokenizer.COMMENT;
                                continue stateloop;
                        }
                    }
                case COMMENT_END_SPACE:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        




                        switch (c) {
                            case '>':
                                



                                emitComment(0, pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '-':
                                



                                appendLongStrBuf(c);
                                


                                state = Tokenizer.COMMENT_END_DASH;
                                continue stateloop;
                            case ' ':
                            case '\t':
                            case '\u000C':
                                






                                appendLongStrBuf(c);
                                continue;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                appendLongStrBuf(c);
                                


                                state = Tokenizer.COMMENT;
                                continue stateloop;
                        }
                    }
                    
                case COMMENT_END_BANG:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        




                        switch (c) {
                            case '>':
                                



                                emitComment(3, pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '-':
                                




                                appendLongStrBuf(c);
                                


                                state = Tokenizer.COMMENT_END_DASH;
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                






                                appendLongStrBuf(c);
                                


                                state = Tokenizer.COMMENT;
                                continue stateloop;
                        }
                    }
                    
                case COMMENT_START_DASH:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    




                    switch (c) {
                        case '-':
                            



                            appendLongStrBuf(c);
                            state = Tokenizer.COMMENT_END;
                            continue stateloop;
                        case '>':
                            errPrematureEndOfComment();
                            
                            emitComment(1, pos);
                            


                            state = Tokenizer.DATA;
                            continue stateloop;
                        case '\r':
                            appendLongStrBufCarriageReturn();
                            state = Tokenizer.COMMENT;
                            break stateloop;
                        case '\n':
                            appendLongStrBufLineFeed();
                            state = Tokenizer.COMMENT;
                            continue stateloop;
                        case '\u0000':
                            c = '\uFFFD';
                            
                        default:
                            




                            appendLongStrBuf(c);
                            


                            state = Tokenizer.COMMENT;
                            continue stateloop;
                    }
                    
                case MARKUP_DECLARATION_OCTYPE:
                    markupdeclarationdoctypeloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        if (index < 6) { 
                            char folded = c;
                            if (c >= 'A' && c <= 'Z') {
                                folded += 0x20;
                            }
                            if (folded == Tokenizer.OCTYPE[index]) {
                                appendLongStrBuf(c);
                            } else {
                                errBogusComment();
                                state = Tokenizer.BOGUS_COMMENT;
                                reconsume = true;
                                continue stateloop;
                            }
                            index++;
                            continue;
                        } else {
                            state = Tokenizer.DOCTYPE;
                            reconsume = true;
                            break markupdeclarationdoctypeloop;
                            
                        }
                    }
                    
                case DOCTYPE:
                    doctypeloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        initDoctypeFields();
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                state = Tokenizer.BEFORE_DOCTYPE_NAME;
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                state = Tokenizer.BEFORE_DOCTYPE_NAME;
                                break doctypeloop;
                            
                            default:
                                


                                errMissingSpaceBeforeDoctypeName();
                                



                                state = Tokenizer.BEFORE_DOCTYPE_NAME;
                                reconsume = true;
                                break doctypeloop;
                            
                        }
                    }
                    
                case BEFORE_DOCTYPE_NAME:
                    beforedoctypenameloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                continue;
                            case '>':
                                


                                errNamelessDoctype();
                                



                                forceQuirks = true;
                                


                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                if (c >= 'A' && c <= 'Z') {
                                    







                                    c += 0x20;
                                }
                                
                                



                                clearStrBufAndAppendCurrentC(c);
                                


                                state = Tokenizer.DOCTYPE_NAME;
                                break beforedoctypenameloop;
                            
                        }
                    }
                    
                case DOCTYPE_NAME:
                    doctypenameloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                strBufToDoctypeName();
                                state = Tokenizer.AFTER_DOCTYPE_NAME;
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                strBufToDoctypeName();
                                state = Tokenizer.AFTER_DOCTYPE_NAME;
                                break doctypenameloop;
                            
                            case '>':
                                



                                strBufToDoctypeName();
                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                






                                if (c >= 'A' && c <= 'Z') {
                                    c += 0x0020;
                                }
                                




                                appendStrBuf(c);
                                


                                continue;
                        }
                    }
                    
                case AFTER_DOCTYPE_NAME:
                    afterdoctypenameloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                continue;
                            case '>':
                                



                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case 'p':
                            case 'P':
                                index = 0;
                                state = Tokenizer.DOCTYPE_UBLIC;
                                break afterdoctypenameloop;
                            
                            case 's':
                            case 'S':
                                index = 0;
                                state = Tokenizer.DOCTYPE_YSTEM;
                                continue stateloop;
                            default:
                                


                                bogusDoctype();

                                



                                
                                


                                state = Tokenizer.BOGUS_DOCTYPE;
                                continue stateloop;
                        }
                    }
                    
                case DOCTYPE_UBLIC:
                    doctypeublicloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        





                        if (index < 5) { 
                            char folded = c;
                            if (c >= 'A' && c <= 'Z') {
                                folded += 0x20;
                            }
                            if (folded != Tokenizer.UBLIC[index]) {
                                bogusDoctype();
                                
                                state = Tokenizer.BOGUS_DOCTYPE;
                                reconsume = true;
                                continue stateloop;
                            }
                            index++;
                            continue;
                        } else {
                            state = Tokenizer.BEFORE_DOCTYPE_PUBLIC_IDENTIFIER;
                            reconsume = true;
                            break doctypeublicloop;
                            
                        }
                    }
                    
                case BEFORE_DOCTYPE_PUBLIC_IDENTIFIER:
                    beforedoctypepublicidentifierloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                





                                continue;
                            case '"':
                                




                                clearLongStrBufForNextState();
                                



                                state = Tokenizer.DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED;
                                break beforedoctypepublicidentifierloop;
                            
                            case '\'':
                                




                                clearLongStrBufForNextState();
                                



                                state = Tokenizer.DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED;
                                continue stateloop;
                            case '>':
                                
                                errExpectedPublicId();
                                



                                forceQuirks = true;
                                


                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = Tokenizer.BOGUS_DOCTYPE;
                                continue stateloop;
                        }
                    }
                    
                case DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED:
                    doctypepublicidentifierdoublequotedloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '"':
                                



                                publicIdentifier = longStrBufToString();
                                state = Tokenizer.AFTER_DOCTYPE_PUBLIC_IDENTIFIER;
                                break doctypepublicidentifierdoublequotedloop;
                            
                            case '>':
                                


                                errGtInPublicId();
                                



                                forceQuirks = true;
                                


                                publicIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                appendLongStrBuf(c);
                                



                                continue;
                        }
                    }
                    
                case AFTER_DOCTYPE_PUBLIC_IDENTIFIER:
                    afterdoctypepublicidentifierloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                continue;
                            case '"':
                                




                                clearLongStrBufForNextState();
                                



                                state = Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED;
                                break afterdoctypepublicidentifierloop;
                            
                            case '\'':
                                




                                clearLongStrBufForNextState();
                                



                                state = Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED;
                                continue stateloop;
                            case '>':
                                



                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = Tokenizer.BOGUS_DOCTYPE;
                                continue stateloop;
                        }
                    }
                    
                case DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED:
                    doctypesystemidentifierdoublequotedloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '"':
                                



                                systemIdentifier = longStrBufToString();
                                state = Tokenizer.AFTER_DOCTYPE_SYSTEM_IDENTIFIER;
                                continue stateloop;
                            case '>':
                                


                                errGtInSystemId();
                                



                                forceQuirks = true;
                                


                                systemIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                appendLongStrBuf(c);
                                



                                continue;
                        }
                    }
                    
                case AFTER_DOCTYPE_SYSTEM_IDENTIFIER:
                    afterdoctypesystemidentifierloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                continue;
                            case '>':
                                



                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            default:
                                




                                bogusDoctypeWithoutQuirks();
                                state = Tokenizer.BOGUS_DOCTYPE;
                                break afterdoctypesystemidentifierloop;
                            
                        }
                    }
                    
                case BOGUS_DOCTYPE:
                    for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '>':
                                



                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            default:
                                



                                continue;
                        }
                    }
                    
                case DOCTYPE_YSTEM:
                    doctypeystemloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        






                        if (index < 5) { 
                            char folded = c;
                            if (c >= 'A' && c <= 'Z') {
                                folded += 0x20;
                            }
                            if (folded != Tokenizer.YSTEM[index]) {
                                bogusDoctype();
                                state = Tokenizer.BOGUS_DOCTYPE;
                                reconsume = true;
                                continue stateloop;
                            }
                            index++;
                            continue stateloop;
                        } else {
                            state = Tokenizer.BEFORE_DOCTYPE_SYSTEM_IDENTIFIER;
                            reconsume = true;
                            break doctypeystemloop;
                            
                        }
                    }
                    
                case BEFORE_DOCTYPE_SYSTEM_IDENTIFIER:
                    beforedoctypesystemidentifierloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '\r':
                                silentCarriageReturn();
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                





                                continue;
                            case '"':
                                




                                clearLongStrBufForNextState();
                                



                                state = Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED;
                                continue stateloop;
                            case '\'':
                                




                                clearLongStrBufForNextState();
                                



                                state = Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED;
                                break beforedoctypesystemidentifierloop;
                            
                            case '>':
                                
                                errExpectedSystemId();
                                



                                forceQuirks = true;
                                


                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = Tokenizer.BOGUS_DOCTYPE;
                                continue stateloop;
                        }
                    }
                    
                case DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\'':
                                



                                systemIdentifier = longStrBufToString();
                                state = Tokenizer.AFTER_DOCTYPE_SYSTEM_IDENTIFIER;
                                continue stateloop;
                            case '>':
                                errGtInSystemId();
                                



                                forceQuirks = true;
                                


                                systemIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                appendLongStrBuf(c);
                                



                                continue;
                        }
                    }
                    
                case DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '\'':
                                



                                publicIdentifier = longStrBufToString();
                                state = Tokenizer.AFTER_DOCTYPE_PUBLIC_IDENTIFIER;
                                continue stateloop;
                            case '>':
                                errGtInPublicId();
                                



                                forceQuirks = true;
                                


                                publicIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = Tokenizer.DATA;
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                appendLongStrBuf(c);
                                



                                continue;
                        }
                    }
                    
                case CDATA_START:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        if (index < 6) { 
                            if (c == Tokenizer.CDATA_LSQB[index]) {
                                appendLongStrBuf(c);
                            } else {
                                errBogusComment();
                                state = Tokenizer.BOGUS_COMMENT;
                                reconsume = true;
                                continue stateloop;
                            }
                            index++;
                            continue;
                        } else {
                            cstart = pos; 
                            state = Tokenizer.CDATA_SECTION;
                            reconsume = true;
                            break; 
                        }
                    }
                    
                case CDATA_SECTION:
                    cdatasectionloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        switch (c) {
                            case ']':
                                flushChars(buf, pos);
                                state = Tokenizer.CDATA_RSQB;
                                break cdatasectionloop; 
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                continue;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            default:
                                continue;
                        }
                    }
                    
                case CDATA_RSQB:
                    cdatarsqb: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case ']':
                                state = Tokenizer.CDATA_RSQB_RSQB;
                                break cdatarsqb;
                            default:
                                tokenHandler.characters(Tokenizer.RSQB_RSQB, 0,
                                        1);
                                cstart = pos;
                                state = Tokenizer.CDATA_SECTION;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case CDATA_RSQB_RSQB:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    switch (c) {
                        case '>':
                            cstart = pos + 1;
                            state = Tokenizer.DATA;
                            continue stateloop;
                        default:
                            tokenHandler.characters(Tokenizer.RSQB_RSQB, 0, 2);
                            cstart = pos;
                            state = Tokenizer.CDATA_SECTION;
                            reconsume = true;
                            continue stateloop;

                    }
                    
                case ATTRIBUTE_VALUE_SINGLE_QUOTED:
                    attributevaluesinglequotedloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '\'':
                                



                                addAttributeWithValue();

                                state = Tokenizer.AFTER_ATTRIBUTE_VALUE_QUOTED;
                                continue stateloop;
                            case '&':
                                





                                clearStrBufAndAppendCurrentC(c);
                                rememberAmpersandLocation('\'');
                                returnState = state;
                                state = Tokenizer.CONSUME_CHARACTER_REFERENCE;
                                break attributevaluesinglequotedloop;
                            
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                continue;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                



                                appendLongStrBuf(c);
                                



                                continue;
                        }
                    }
                    
                case CONSUME_CHARACTER_REFERENCE:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    if (c == '\u0000') {
                        break stateloop;
                    }
                    







                    








                    switch (c) {
                        case ' ':
                        case '\t':
                        case '\n':
                        case '\r': 
                        case '\u000C':
                        case '<':
                        case '&':
                            emitOrAppendStrBuf(returnState);
                            if ((returnState & (~1)) == 0) {
                                cstart = pos;
                            }
                            state = returnState;
                            reconsume = true;
                            continue stateloop;
                        case '#':
                            



                            appendStrBuf('#');
                            state = Tokenizer.CONSUME_NCR;
                            continue stateloop;
                        default:
                            if (c == additional) {
                                emitOrAppendStrBuf(returnState);
                                state = returnState;
                                reconsume = true;
                                continue stateloop;
                            }
                            entCol = -1;
                            lo = 0;
                            hi = (NamedCharacters.NAMES.length - 1);
                            candidate = -1;
                            strBufMark = 0;
                            state = Tokenizer.CHARACTER_REFERENCE_LOOP;
                            reconsume = true;
                            
                    }
                    
                case CHARACTER_REFERENCE_LOOP:
                    outer: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        if (c == '\u0000') {
                            break stateloop;
                        }
                        entCol++;
                        






                        hiloop: for (;;) {
                            if (hi == -1) {
                                break hiloop;
                            }
                            if (entCol == NamedCharacters.NAMES[hi].length) {
                                break hiloop;
                            }
                            if (entCol > NamedCharacters.NAMES[hi].length) {
                                break outer;
                            } else if (c < NamedCharacters.NAMES[hi][entCol]) {
                                hi--;
                            } else {
                                break hiloop;
                            }
                        }

                        loloop: for (;;) {
                            if (hi < lo) {
                                break outer;
                            }
                            if (entCol == NamedCharacters.NAMES[lo].length) {
                                candidate = lo;
                                strBufMark = strBufLen;
                                lo++;
                            } else if (entCol > NamedCharacters.NAMES[lo].length) {
                                break outer;
                            } else if (c > NamedCharacters.NAMES[lo][entCol]) {
                                lo++;
                            } else {
                                break loloop;
                            }
                        }
                        if (hi < lo) {
                            break outer;
                        }
                        appendStrBuf(c);
                        continue;
                    }

                    
                    if (candidate == -1) {
                        
                        


                        errNoNamedCharacterMatch();
                        emitOrAppendStrBuf(returnState);
                        if ((returnState & (~1)) == 0) {
                            cstart = pos;
                        }
                        state = returnState;
                        reconsume = true;
                        continue stateloop;
                    } else {
                        
                        char[] candidateArr = NamedCharacters.NAMES[candidate];
                        if (candidateArr[candidateArr.length - 1] != ';') {
                            



                            if ((returnState & (~1)) != 0) {
                                




                                char ch;
                                if (strBufMark == strBufLen) {
                                    ch = c;
                                } else {
                                    
                                    
                                    
                                    ch = strBuf[strBufMark];
                                    
                                }
                                if ((ch >= '0' && ch <= '9')
                                        || (ch >= 'A' && ch <= 'Z')
                                        || (ch >= 'a' && ch <= 'z')) {
                                    










                                    errNoNamedCharacterMatch();
                                    appendStrBufToLongStrBuf();
                                    state = returnState;
                                    reconsume = true;
                                    continue stateloop;
                                }
                            }
                            if ((returnState & (~1)) != 0) {
                                errUnescapedAmpersandInterpretedAsCharacterReference();
                            } else {
                                errNotSemicolonTerminated();
                            }
                        }

                        





                        char[] val = NamedCharacters.VALUES[candidate];
                        emitOrAppend(val, returnState);
                        
                        if (strBufMark < strBufLen) {
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            if ((returnState & (~1)) != 0) {
                                for (int i = strBufMark; i < strBufLen; i++) {
                                    appendLongStrBuf(strBuf[i]);
                                }
                            } else {
                                tokenHandler.characters(strBuf, strBufMark,
                                        strBufLen - strBufMark);
                            }
                            
                        }
                        if ((returnState & (~1)) == 0) {
                            cstart = pos;
                        }
                        state = returnState;
                        reconsume = true;
                        continue stateloop;
                        






                    }
                    
                case CONSUME_NCR:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    prevValue = -1;
                    value = 0;
                    seenDigits = false;
                    



                    switch (c) {
                        case 'x':
                        case 'X':

                            













                            appendStrBuf(c);
                            state = Tokenizer.HEX_NCR_LOOP;
                            continue stateloop;
                        default:
                            







                            state = Tokenizer.DECIMAL_NRC_LOOP;
                            reconsume = true;
                            
                    }
                    
                case DECIMAL_NRC_LOOP:
                    decimalloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        
                        if (value < prevValue) {
                            value = 0x110000; 
                            
                            
                        }
                        prevValue = value;
                        



                        if (c >= '0' && c <= '9') {
                            seenDigits = true;
                            value *= 10;
                            value += c - '0';
                            continue;
                        } else if (c == ';') {
                            if (seenDigits) {
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos + 1;
                                }
                                state = Tokenizer.HANDLE_NCR_VALUE;
                                
                                break decimalloop;
                            } else {
                                errNoDigitsInNCR();
                                appendStrBuf(';');
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos + 1;
                                }
                                state = returnState;
                                continue stateloop;
                            }
                        } else {
                            










                            if (!seenDigits) {
                                errNoDigitsInNCR();
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos;
                                }
                                state = returnState;
                                reconsume = true;
                                continue stateloop;
                            } else {
                                errCharRefLacksSemicolon();
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos;
                                }
                                state = Tokenizer.HANDLE_NCR_VALUE;
                                reconsume = true;
                                
                                break decimalloop;
                            }
                        }
                    }
                    
                case HANDLE_NCR_VALUE:
                    
                    
                    handleNcrValue(returnState);
                    state = returnState;
                    continue stateloop;
                    
                case HEX_NCR_LOOP:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        
                        if (value < prevValue) {
                            value = 0x110000; 
                            
                            
                        }
                        prevValue = value;
                        



                        if (c >= '0' && c <= '9') {
                            seenDigits = true;
                            value *= 16;
                            value += c - '0';
                            continue;
                        } else if (c >= 'A' && c <= 'F') {
                            seenDigits = true;
                            value *= 16;
                            value += c - 'A' + 10;
                            continue;
                        } else if (c >= 'a' && c <= 'f') {
                            seenDigits = true;
                            value *= 16;
                            value += c - 'a' + 10;
                            continue;
                        } else if (c == ';') {
                            if (seenDigits) {
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos + 1;
                                }
                                state = Tokenizer.HANDLE_NCR_VALUE;
                                continue stateloop;
                            } else {
                                errNoDigitsInNCR();
                                appendStrBuf(';');
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos + 1;
                                }
                                state = returnState;
                                continue stateloop;
                            }
                        } else {
                            










                            if (!seenDigits) {
                                errNoDigitsInNCR();
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos;
                                }
                                state = returnState;
                                reconsume = true;
                                continue stateloop;
                            } else {
                                errCharRefLacksSemicolon();
                                if ((returnState & (~1)) == 0) {
                                    cstart = pos;
                                }
                                state = Tokenizer.HANDLE_NCR_VALUE;
                                reconsume = true;
                                continue stateloop;
                            }
                        }
                    }
                    
                case PLAINTEXT:
                    plaintextloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        switch (c) {
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                continue;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                



                                


                                continue;
                        }
                    }
                    
                case CDATA:
                    cdataloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        switch (c) {
                            case '<':
                                









                                flushChars(buf, pos);

                                returnState = state;
                                state = Tokenizer.TAG_OPEN_NON_PCDATA;
                                break cdataloop; 
                            
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                continue;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                



                                


                                continue;
                        }
                    }
                    
                case TAG_OPEN_NON_PCDATA:
                    tagopennonpcdataloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '!':
                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                cstart = pos;
                                state = Tokenizer.ESCAPE_EXCLAMATION;
                                break tagopennonpcdataloop; 
                            
                            
                            case '/':
                                




                                if (contentModelElement != null) {
                                    



                                    index = 0;
                                    clearStrBufForNextState();
                                    state = Tokenizer.CLOSE_TAG_OPEN_NOT_PCDATA;
                                    continue stateloop;
                                } 
                            default:
                                



                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                



                                cstart = pos;
                                state = returnState;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case ESCAPE_EXCLAMATION:
                    escapeexclamationloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '-':
                                state = Tokenizer.ESCAPE_EXCLAMATION_HYPHEN;
                                break escapeexclamationloop; 
                            
                            
                            default:
                                state = returnState;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case ESCAPE_EXCLAMATION_HYPHEN:
                    escapeexclamationhyphenloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '-':
                                state = Tokenizer.ESCAPE_HYPHEN_HYPHEN;
                                break escapeexclamationhyphenloop;
                            
                            default:
                                state = returnState;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case ESCAPE_HYPHEN_HYPHEN:
                    escapehyphenhyphenloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '-':
                                continue;
                            case '>':
                                state = returnState;
                                continue stateloop;
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                state = Tokenizer.ESCAPE;
                                break escapehyphenhyphenloop;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = Tokenizer.ESCAPE;
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                state = Tokenizer.ESCAPE;
                                break escapehyphenhyphenloop;
                            
                        }
                    }
                    
                case ESCAPE:
                    escapeloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '-':
                                state = Tokenizer.ESCAPE_HYPHEN;
                                break escapeloop; 
                            
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                continue;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                continue;
                        }
                    }
                    
                case ESCAPE_HYPHEN:
                    escapehyphenloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '-':
                                state = Tokenizer.ESCAPE_HYPHEN_HYPHEN;
                                continue stateloop;
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                state = Tokenizer.ESCAPE;
                                continue stateloop;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = Tokenizer.ESCAPE;
                                continue stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                state = Tokenizer.ESCAPE;
                                continue stateloop;
                        }
                    }
                    
                case CLOSE_TAG_OPEN_NOT_PCDATA:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        
                        
                        
                        

















                        
                        
                        
                        
                        
                        
                        
                        if (index < contentModelElementNameAsArray.length) {
                            char e = contentModelElementNameAsArray[index];
                            char folded = c;
                            if (c >= 'A' && c <= 'Z') {
                                folded += 0x20;
                            }
                            if (folded != e) {
                                
                                errHtml4LtSlashInRcdata(folded);
                                
                                tokenHandler.characters(Tokenizer.LT_SOLIDUS,
                                        0, 2);
                                emitStrBuf();
                                cstart = pos;
                                state = returnState;
                                reconsume = true;
                                continue stateloop;
                            }
                            appendStrBuf(c);
                            index++;
                            continue;
                        } else {
                            endTag = true;
                            
                            
                            tagName = contentModelElement;
                            switch (c) {
                                case '\r':
                                    silentCarriageReturn();
                                    state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                    break stateloop;
                                case '\n':
                                    silentLineFeed();
                                    
                                case ' ':
                                case '\t':
                                case '\u000C':
                                    





                                    state = Tokenizer.BEFORE_ATTRIBUTE_NAME;
                                    continue stateloop;
                                case '>':
                                    



                                    state = emitCurrentTagToken(false, pos);
                                    if (shouldSuspend) {
                                        break stateloop;
                                    }
                                    


                                    continue stateloop;
                                case '/':
                                    



                                    state = Tokenizer.SELF_CLOSING_START_TAG;
                                    continue stateloop;
                                default:
                                    
                                    errWarnLtSlashInRcdata();
                                    
                                    tokenHandler.characters(
                                            Tokenizer.LT_SOLIDUS, 0, 2);
                                    emitStrBuf();
                                    if (c == '\u0000') {
                                        emitReplacementCharacter(buf, pos);
                                    } else {
                                        cstart = pos; 
                                                      
                                    }
                                    state = returnState;
                                    continue stateloop;
                            }
                        }
                    }
                    
                case CLOSE_TAG_OPEN_PCDATA:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    




                    switch (c) {
                        case '>':
                            
                            errLtSlashGt();
                            


                            cstart = pos + 1;
                            state = Tokenizer.DATA;
                            continue stateloop;
                        case '\r':
                            silentCarriageReturn();
                            
                            errGarbageAfterLtSlash();
                            


                            clearLongStrBufAndAppendToComment('\n');
                            state = Tokenizer.BOGUS_COMMENT;
                            break stateloop;
                        case '\n':
                            silentLineFeed();
                            
                            errGarbageAfterLtSlash();
                            


                            clearLongStrBufAndAppendToComment('\n');
                            state = Tokenizer.BOGUS_COMMENT;
                            continue stateloop;
                        case '\u0000':
                            c = '\uFFFD';
                            
                        default:
                            if (c >= 'A' && c <= 'Z') {
                                c += 0x20;
                            }
                            if (c >= 'a' && c <= 'z') {
                                




                                endTag = true;
                                


                                clearStrBufAndAppendCurrentC(c);
                                




                                state = Tokenizer.TAG_NAME;
                                continue stateloop;
                            } else {
                                
                                errGarbageAfterLtSlash();
                                


                                clearLongStrBufAndAppendToComment(c);
                                state = Tokenizer.BOGUS_COMMENT;
                                continue stateloop;
                            }
                    }
                    
                case RCDATA:
                    rcdataloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        switch (c) {
                            case '&':
                                







                                flushChars(buf, pos);
                                clearStrBufAndAppendCurrentC(c);
                                additional = '\u0000';
                                returnState = state;
                                state = Tokenizer.CONSUME_CHARACTER_REFERENCE;
                                continue stateloop;
                            case '<':
                                









                                flushChars(buf, pos);

                                returnState = state;
                                state = Tokenizer.TAG_OPEN_NON_PCDATA;
                                continue stateloop;
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                continue;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                



                                


                                continue;
                        }
                    }

            }
        }
        flushChars(buf, pos);
        


        
        stateSave = state;
        returnStateSave = returnState;
        return pos;
    }

    @Inline private void initDoctypeFields() {
        doctypeName = "";
        systemIdentifier = null;
        publicIdentifier = null;
        forceQuirks = false;
    }

    @Inline private void adjustDoubleHyphenAndAppendToLongStrBufCarriageReturn()
            throws SAXException {
        silentCarriageReturn();
        adjustDoubleHyphenAndAppendToLongStrBufAndErr('\n');
    }

    @Inline private void adjustDoubleHyphenAndAppendToLongStrBufLineFeed()
            throws SAXException {
        silentLineFeed();
        adjustDoubleHyphenAndAppendToLongStrBufAndErr('\n');
    }

    @Inline private void appendLongStrBufLineFeed() {
        silentLineFeed();
        appendLongStrBuf('\n');
    }

    @Inline private void appendLongStrBufCarriageReturn() {
        silentCarriageReturn();
        appendLongStrBuf('\n');
    }

    @Inline protected void silentCarriageReturn() {
        ++line;
        lastCR = true;
    }

    @Inline protected void silentLineFeed() {
        ++line;
    }

    private void emitCarriageReturn(@NoLength char[] buf, int pos)
            throws SAXException {
        silentCarriageReturn();
        flushChars(buf, pos);
        tokenHandler.characters(Tokenizer.LF, 0, 1);
        cstart = Integer.MAX_VALUE;
    }

    private void emitReplacementCharacter(@NoLength char[] buf, int pos)
            throws SAXException {
        silentCarriageReturn();
        flushChars(buf, pos);
        tokenHandler.characters(Tokenizer.REPLACEMENT_CHARACTER, 0, 1);
        cstart = Integer.MAX_VALUE;
    }

    private void rememberAmpersandLocation(char add) {
        additional = add;
        
        ampersandLocation = new LocatorImpl(this);
        
    }

    private void bogusDoctype() throws SAXException {
        errBogusDoctype();
        forceQuirks = true;
    }

    private void bogusDoctypeWithoutQuirks() throws SAXException {
        errBogusDoctype();
        forceQuirks = false;
    }

    private void emitOrAppendStrBuf(int returnState) throws SAXException {
        if ((returnState & (~1)) != 0) {
            appendStrBufToLongStrBuf();
        } else {
            emitStrBuf();
        }
    }

    private void handleNcrValue(int returnState) throws SAXException {
        




        if (value >= 0x80 && value <= 0x9f) {
            



            errNcrInC1Range();
            




            @NoLength char[] val = NamedCharacters.WINDOWS_1252[value - 0x80];
            emitOrAppendOne(val, returnState);
        } else if (value == 0x0D) {
            errRcnCr();
            emitOrAppendOne(Tokenizer.LF, returnState);
            
        } else if (value == 0xC
                && contentSpacePolicy != XmlViolationPolicy.ALLOW) {
            if (contentSpacePolicy == XmlViolationPolicy.ALTER_INFOSET) {
                emitOrAppendOne(Tokenizer.SPACE, returnState);
            } else if (contentSpacePolicy == XmlViolationPolicy.FATAL) {
                fatal("A character reference expanded to a form feed which is not legal XML 1.0 white space.");
            }
            
        } else if ((value >= 0x0000 && value <= 0x0008) || (value == 0x000B)
                || (value >= 0x000E && value <= 0x001F) || value == 0x007F) {
            











            errNcrControlChar();
            emitOrAppendOne(Tokenizer.REPLACEMENT_CHARACTER, returnState);
        } else if ((value & 0xF800) == 0xD800) {
            errNcrSurrogate();
            emitOrAppendOne(Tokenizer.REPLACEMENT_CHARACTER, returnState);
        } else if ((value & 0xFFFE) == 0xFFFE) {
            errNcrNonCharacter();
            emitOrAppendOne(Tokenizer.REPLACEMENT_CHARACTER, returnState);
        } else if (value >= 0xFDD0 && value <= 0xFDEF) {
            errNcrUnassigned();
            emitOrAppendOne(Tokenizer.REPLACEMENT_CHARACTER, returnState);
        } else if (value <= 0xFFFF) {
            



            char ch = (char) value;
            
            maybeWarnPrivateUse(ch);
            
            bmpChar[0] = ch;
            emitOrAppendOne(bmpChar, returnState);
        } else if (value <= 0x10FFFF) {
            
            maybeWarnPrivateUseAstral();
            
            astralChar[0] = (char) (Tokenizer.LEAD_OFFSET + (value >> 10));
            astralChar[1] = (char) (0xDC00 + (value & 0x3FF));
            emitOrAppend(astralChar, returnState);
        } else {
            errNcrOutOfRange();
            emitOrAppendOne(Tokenizer.REPLACEMENT_CHARACTER, returnState);
        }
    }

    public void eof() throws SAXException {
        int state = stateSave;
        int returnState = returnStateSave;

        eofloop: for (;;) {
            switch (state) {
                case TAG_OPEN_NON_PCDATA:
                    


                    tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                    



                    break eofloop;
                case TAG_OPEN:
                    



                    


                    errEofAfterLt();
                    


                    tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                    



                    break eofloop;
                case CLOSE_TAG_OPEN_NOT_PCDATA:
                    if (index < contentModelElementNameAsArray.length) {
                        break eofloop;
                    } else {
                        errEofInEndTag();
                        


                        break eofloop;
                    }
                case CLOSE_TAG_OPEN_PCDATA:
                    
                    errEofAfterLt();
                    



                    tokenHandler.characters(Tokenizer.LT_SOLIDUS, 0, 2);
                    


                    break eofloop;
                case TAG_NAME:
                    


                    errEofInTagName();
                    


                    break eofloop;
                case BEFORE_ATTRIBUTE_NAME:
                case AFTER_ATTRIBUTE_VALUE_QUOTED:
                case SELF_CLOSING_START_TAG:
                    
                    errEofWithoutGt();
                    


                    break eofloop;
                case ATTRIBUTE_NAME:
                    


                    errEofInAttributeName();
                    


                    break eofloop;
                case AFTER_ATTRIBUTE_NAME:
                case BEFORE_ATTRIBUTE_VALUE:
                    
                    errEofWithoutGt();
                    


                    break eofloop;
                case ATTRIBUTE_VALUE_DOUBLE_QUOTED:
                case ATTRIBUTE_VALUE_SINGLE_QUOTED:
                case ATTRIBUTE_VALUE_UNQUOTED:
                    
                    errEofInAttributeValue();
                    


                    break eofloop;
                case BOGUS_COMMENT:
                    emitComment(0, 0);
                    break eofloop;
                case BOGUS_COMMENT_HYPHEN:
                    
                    maybeAppendSpaceToBogusComment();
                    
                    emitComment(0, 0);
                    break eofloop;
                case MARKUP_DECLARATION_OPEN:
                    errBogusComment();
                    clearLongStrBuf();
                    emitComment(0, 0);
                    break eofloop;
                case MARKUP_DECLARATION_HYPHEN:
                    errBogusComment();
                    emitComment(0, 0);
                    break eofloop;
                case MARKUP_DECLARATION_OCTYPE:
                    if (index < 6) {
                        errBogusComment();
                        emitComment(0, 0);
                    } else {
                        
                        errEofInDoctype();
                        



                        doctypeName = "";
                        publicIdentifier = null;
                        systemIdentifier = null;
                        forceQuirks = true;
                        


                        emitDoctypeToken(0);
                        


                        break eofloop;
                    }
                    break eofloop;
                case COMMENT_START:
                case COMMENT:
                case COMMENT_END_SPACE:
                    


                    errEofInComment();
                    
                    emitComment(0, 0);
                    


                    break eofloop;
                case COMMENT_END:
                    errEofInComment();
                    
                    emitComment(2, 0);
                    


                    break eofloop;
                case COMMENT_END_DASH:
                case COMMENT_START_DASH:
                    errEofInComment();
                    
                    emitComment(1, 0);
                    


                    break eofloop;
                case COMMENT_END_BANG:
                    errEofInComment();
                    
                    emitComment(3, 0);
                    


                    break eofloop;
                case DOCTYPE:
                case BEFORE_DOCTYPE_NAME:
                    errEofInDoctype();
                    



                    forceQuirks = true;
                    


                    emitDoctypeToken(0);
                    


                    break eofloop;
                case DOCTYPE_NAME:
                    errEofInDoctype();
                    strBufToDoctypeName();
                    


                    forceQuirks = true;
                    


                    emitDoctypeToken(0);
                    


                    break eofloop;
                case DOCTYPE_UBLIC:
                case DOCTYPE_YSTEM:
                case AFTER_DOCTYPE_NAME:
                case BEFORE_DOCTYPE_PUBLIC_IDENTIFIER:
                    errEofInDoctype();
                    


                    forceQuirks = true;
                    


                    emitDoctypeToken(0);
                    


                    break eofloop;
                case DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED:
                case DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED:
                    
                    errEofInPublicId();
                    


                    forceQuirks = true;
                    


                    publicIdentifier = longStrBufToString();
                    emitDoctypeToken(0);
                    


                    break eofloop;
                case AFTER_DOCTYPE_PUBLIC_IDENTIFIER:
                case BEFORE_DOCTYPE_SYSTEM_IDENTIFIER:
                    errEofInDoctype();
                    


                    forceQuirks = true;
                    


                    emitDoctypeToken(0);
                    


                    break eofloop;
                case DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED:
                case DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED:
                    
                    errEofInSystemId();
                    


                    forceQuirks = true;
                    


                    systemIdentifier = longStrBufToString();
                    emitDoctypeToken(0);
                    


                    break eofloop;
                case AFTER_DOCTYPE_SYSTEM_IDENTIFIER:
                    errEofInDoctype();
                    


                    forceQuirks = true;
                    


                    emitDoctypeToken(0);
                    


                    break eofloop;
                case BOGUS_DOCTYPE:
                    


                    emitDoctypeToken(0);
                    


                    break eofloop;
                case CONSUME_CHARACTER_REFERENCE:
                    







                    









                    emitOrAppendStrBuf(returnState);
                    state = returnState;
                    continue;
                case CHARACTER_REFERENCE_LOOP:
                    outer: for (;;) {
                        char c = '\u0000';
                        entCol++;
                        






                        hiloop: for (;;) {
                            if (hi == -1) {
                                break hiloop;
                            }
                            if (entCol == NamedCharacters.NAMES[hi].length) {
                                break hiloop;
                            }
                            if (entCol > NamedCharacters.NAMES[hi].length) {
                                break outer;
                            } else if (c < NamedCharacters.NAMES[hi][entCol]) {
                                hi--;
                            } else {
                                break hiloop;
                            }
                        }

                        loloop: for (;;) {
                            if (hi < lo) {
                                break outer;
                            }
                            if (entCol == NamedCharacters.NAMES[lo].length) {
                                candidate = lo;
                                strBufMark = strBufLen;
                                lo++;
                            } else if (entCol > NamedCharacters.NAMES[lo].length) {
                                break outer;
                            } else if (c > NamedCharacters.NAMES[lo][entCol]) {
                                lo++;
                            } else {
                                break loloop;
                            }
                        }
                        if (hi < lo) {
                            break outer;
                        }
                        continue;
                    }

                    
                    if (candidate == -1) {
                        


                        errNoNamedCharacterMatch();
                        emitOrAppendStrBuf(returnState);
                        state = returnState;
                        continue eofloop;
                    } else {
                        char[] candidateArr = NamedCharacters.NAMES[candidate];
                        if (candidateArr[candidateArr.length - 1] != ';') {
                            



                            if ((returnState & (~1)) != 0) {
                                




                                char ch;
                                if (strBufMark == strBufLen) {
                                    ch = '\u0000';
                                } else {
                                    ch = strBuf[strBufMark];
                                }
                                if ((ch >= '0' && ch <= '9')
                                        || (ch >= 'A' && ch <= 'Z')
                                        || (ch >= 'a' && ch <= 'z')) {
                                    










                                    errNoNamedCharacterMatch();
                                    appendStrBufToLongStrBuf();
                                    state = returnState;
                                    continue eofloop;
                                }
                            }
                            if ((returnState & (~1)) != 0) {
                                errUnescapedAmpersandInterpretedAsCharacterReference();
                            } else {
                                errNotSemicolonTerminated();
                            }
                        }

                        





                        char[] val = NamedCharacters.VALUES[candidate];
                        emitOrAppend(val, returnState);
                        
                        if (strBufMark < strBufLen) {
                            if ((returnState & (~1)) != 0) {
                                for (int i = strBufMark; i < strBufLen; i++) {
                                    appendLongStrBuf(strBuf[i]);
                                }
                            } else {
                                tokenHandler.characters(strBuf, strBufMark,
                                        strBufLen - strBufMark);
                            }
                        }
                        state = returnState;
                        continue eofloop;
                        






                    }
                case CONSUME_NCR:
                case DECIMAL_NRC_LOOP:
                case HEX_NCR_LOOP:
                    








                    if (!seenDigits) {
                        errNoDigitsInNCR();
                        emitOrAppendStrBuf(returnState);
                        state = returnState;
                        continue;
                    } else {
                        errCharRefLacksSemicolon();
                    }
                    
                    handleNcrValue(returnState);
                    state = returnState;
                    continue;
                case DATA:
                default:
                    break eofloop;
            }
        }
        
        


        tokenHandler.eof();
        return;
    }

    private void emitDoctypeToken(int pos) throws SAXException {
        cstart = pos + 1;
        tokenHandler.doctype(doctypeName, publicIdentifier, systemIdentifier,
                forceQuirks);
        
        
        
        Portability.releaseLocal(doctypeName);
        Portability.releaseString(publicIdentifier);
        Portability.releaseString(systemIdentifier);
    }

    @Inline protected char checkChar(@NoLength char[] buf, int pos)
            throws SAXException {
        return buf[pos];
    }

    

    




    public boolean isAlreadyComplainedAboutNonAscii() {
        return true;
    }

    

    public void internalEncodingDeclaration(String internalCharset)
            throws SAXException {
        if (encodingDeclarationHandler != null) {
            encodingDeclarationHandler.internalEncodingDeclaration(internalCharset);
        }
    }

    



    private void emitOrAppend(char[] val, int returnState) throws SAXException {
        if ((returnState & (~1)) != 0) {
            appendLongStrBuf(val);
        } else {
            tokenHandler.characters(val, 0, val.length);
        }
    }

    private void emitOrAppendOne(@NoLength char[] val, int returnState)
            throws SAXException {
        if ((returnState & (~1)) != 0) {
            appendLongStrBuf(val[0]);
        } else {
            tokenHandler.characters(val, 0, 1);
        }
    }

    public void end() throws SAXException {
        strBuf = null;
        longStrBuf = null;
        systemIdentifier = null;
        publicIdentifier = null;
        doctypeName = null;
        tagName = null;
        attributeName = null;
        tokenHandler.endTokenization();
        if (attributes != null) {
            attributes.clear(mappingLangToXmlLang);
            Portability.delete(attributes);
            attributes = null;
        }
    }

    public void requestSuspension() {
        shouldSuspend = true;
    }

    public void becomeConfident() {
        confident = true;
    }

    




    public boolean isNextCharOnNewLine() {
        return false;
    }

    public boolean isPrevCR() {
        return lastCR;
    }

    




    public int getLine() {
        return -1;
    }

    




    public int getCol() {
        return -1;
    }

    public boolean isInDataState() {
        return (stateSave == DATA);
    }

    protected void errGarbageAfterLtSlash() throws SAXException {
    }

    protected void errLtSlashGt() throws SAXException {
    }

    protected void errWarnLtSlashInRcdata() throws SAXException {
    }

    protected void errHtml4LtSlashInRcdata(char folded) throws SAXException {
    }

    protected void errCharRefLacksSemicolon() throws SAXException {
    }

    protected void errNoDigitsInNCR() throws SAXException {
    }

    protected void errGtInSystemId() throws SAXException {
    }

    protected void errGtInPublicId() throws SAXException {
    }

    protected void errNamelessDoctype() throws SAXException {
    }

    protected void errConsecutiveHyphens() throws SAXException {
    }

    protected void errPrematureEndOfComment() throws SAXException {
    }

    protected void errBogusComment() throws SAXException {
    }

    protected void errUnquotedAttributeValOrNull(char c) throws SAXException {
    }

    protected void errSlashNotFollowedByGt() throws SAXException {
    }

    protected void errHtml4XmlVoidSyntax() throws SAXException {
    }

    protected void errNoSpaceBetweenAttributes() throws SAXException {
    }

    protected void errHtml4NonNameInUnquotedAttribute(char c)
            throws SAXException {
    }

    protected void errLtOrEqualsInUnquotedAttributeOrNull(char c)
            throws SAXException {
    }

    protected void errAttributeValueMissing() throws SAXException {
    }

    protected void errBadCharBeforeAttributeNameOrNull(char c)
            throws SAXException {
    }

    protected void errEqualsSignBeforeAttributeName() throws SAXException {
    }

    protected void errBadCharAfterLt(char c) throws SAXException {
    }

    protected void errLtGt() throws SAXException {
    }

    protected void errProcessingInstruction() throws SAXException {
    }

    protected void errUnescapedAmpersandInterpretedAsCharacterReference()
            throws SAXException {
    }

    protected void errNotSemicolonTerminated() throws SAXException {
    }

    protected void errNoNamedCharacterMatch() throws SAXException {
    }

    protected void errQuoteBeforeAttributeName(char c) throws SAXException {
    }

    protected void errQuoteOrLtInAttributeNameOrNull(char c) throws SAXException {
    }

    protected void errExpectedPublicId() throws SAXException {
    }

    protected void errBogusDoctype() throws SAXException {
    }

    protected void maybeWarnPrivateUseAstral() throws SAXException {
    }

    protected void maybeWarnPrivateUse(char ch) throws SAXException {
    }

    protected void maybeErrAttributesOnEndTag(HtmlAttributes attrs)
            throws SAXException {
    }

    protected void maybeErrSlashInEndTag(boolean selfClosing)
            throws SAXException {
    }

    protected void errNcrNonCharacter() throws SAXException {
    }

    protected void errNcrSurrogate() throws SAXException {
    }

    protected void errNcrControlChar() throws SAXException {
    }

    protected void errRcnCr() throws SAXException {
    }

    protected void errNcrInC1Range() throws SAXException {
    }

    protected void errEofInPublicId() throws SAXException {
    }

    protected void errEofInComment() throws SAXException {
    }

    protected void errEofInDoctype() throws SAXException {
    }

    protected void errEofInAttributeValue() throws SAXException {
    }

    protected void errEofInAttributeName() throws SAXException {
    }

    protected void errEofWithoutGt() throws SAXException {
    }

    protected void errEofInTagName() throws SAXException {
    }

    protected void errEofInEndTag() throws SAXException {
    }

    protected void errEofAfterLt() throws SAXException {
    }

    protected void errNcrOutOfRange() throws SAXException {
    }

    protected void errNcrUnassigned() throws SAXException {
    }

    protected void errDuplicateAttribute() throws SAXException {
    }

    protected void errEofInSystemId() throws SAXException {
    }

    protected void errExpectedSystemId() throws SAXException {
    }

    protected void errMissingSpaceBeforeDoctypeName() throws SAXException {
    }

    protected void errHyphenHyphenBang() throws SAXException {
    }

    





    public void setEncodingDeclarationHandler(
            EncodingDeclarationHandler encodingDeclarationHandler) {
        this.encodingDeclarationHandler = encodingDeclarationHandler;
    }

}
