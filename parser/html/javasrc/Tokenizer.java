


































package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Auto;
import nu.validator.htmlparser.annotation.CharacterName;
import nu.validator.htmlparser.annotation.Const;
import nu.validator.htmlparser.annotation.Inline;
import nu.validator.htmlparser.annotation.Local;
import nu.validator.htmlparser.annotation.NoLength;
import nu.validator.htmlparser.common.EncodingDeclarationHandler;
import nu.validator.htmlparser.common.Interner;
import nu.validator.htmlparser.common.TokenHandler;
import nu.validator.htmlparser.common.XmlViolationPolicy;

import org.xml.sax.ErrorHandler;
import org.xml.sax.Locator;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
















public class Tokenizer implements Locator {

    private static final int DATA_AND_RCDATA_MASK = ~1;

    public static final int DATA = 0;

    public static final int RCDATA = 1;

    public static final int SCRIPT_DATA = 2;

    public static final int RAWTEXT = 3;

    public static final int SCRIPT_DATA_ESCAPED = 4;

    public static final int ATTRIBUTE_VALUE_DOUBLE_QUOTED = 5;

    public static final int ATTRIBUTE_VALUE_SINGLE_QUOTED = 6;

    public static final int ATTRIBUTE_VALUE_UNQUOTED = 7;

    public static final int PLAINTEXT = 8;

    public static final int TAG_OPEN = 9;

    public static final int CLOSE_TAG_OPEN = 10;

    public static final int TAG_NAME = 11;

    public static final int BEFORE_ATTRIBUTE_NAME = 12;

    public static final int ATTRIBUTE_NAME = 13;

    public static final int AFTER_ATTRIBUTE_NAME = 14;

    public static final int BEFORE_ATTRIBUTE_VALUE = 15;

    public static final int AFTER_ATTRIBUTE_VALUE_QUOTED = 16;

    public static final int BOGUS_COMMENT = 17;

    public static final int MARKUP_DECLARATION_OPEN = 18;

    public static final int DOCTYPE = 19;

    public static final int BEFORE_DOCTYPE_NAME = 20;

    public static final int DOCTYPE_NAME = 21;

    public static final int AFTER_DOCTYPE_NAME = 22;

    public static final int BEFORE_DOCTYPE_PUBLIC_IDENTIFIER = 23;

    public static final int DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED = 24;

    public static final int DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED = 25;

    public static final int AFTER_DOCTYPE_PUBLIC_IDENTIFIER = 26;

    public static final int BEFORE_DOCTYPE_SYSTEM_IDENTIFIER = 27;

    public static final int DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED = 28;

    public static final int DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED = 29;

    public static final int AFTER_DOCTYPE_SYSTEM_IDENTIFIER = 30;

    public static final int BOGUS_DOCTYPE = 31;

    public static final int COMMENT_START = 32;

    public static final int COMMENT_START_DASH = 33;

    public static final int COMMENT = 34;

    public static final int COMMENT_END_DASH = 35;

    public static final int COMMENT_END = 36;

    public static final int COMMENT_END_BANG = 37;

    public static final int NON_DATA_END_TAG_NAME = 38;

    public static final int MARKUP_DECLARATION_HYPHEN = 39;

    public static final int MARKUP_DECLARATION_OCTYPE = 40;

    public static final int DOCTYPE_UBLIC = 41;

    public static final int DOCTYPE_YSTEM = 42;

    public static final int AFTER_DOCTYPE_PUBLIC_KEYWORD = 43;

    public static final int BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS = 44;

    public static final int AFTER_DOCTYPE_SYSTEM_KEYWORD = 45;

    public static final int CONSUME_CHARACTER_REFERENCE = 46;

    public static final int CONSUME_NCR = 47;

    public static final int CHARACTER_REFERENCE_TAIL = 48;

    public static final int HEX_NCR_LOOP = 49;

    public static final int DECIMAL_NRC_LOOP = 50;

    public static final int HANDLE_NCR_VALUE = 51;

    public static final int HANDLE_NCR_VALUE_RECONSUME = 52;

    public static final int CHARACTER_REFERENCE_HILO_LOOKUP = 53;

    public static final int SELF_CLOSING_START_TAG = 54;

    public static final int CDATA_START = 55;

    public static final int CDATA_SECTION = 56;

    public static final int CDATA_RSQB = 57;

    public static final int CDATA_RSQB_RSQB = 58;

    public static final int SCRIPT_DATA_LESS_THAN_SIGN = 59;

    public static final int SCRIPT_DATA_ESCAPE_START = 60;

    public static final int SCRIPT_DATA_ESCAPE_START_DASH = 61;

    public static final int SCRIPT_DATA_ESCAPED_DASH = 62;

    public static final int SCRIPT_DATA_ESCAPED_DASH_DASH = 63;

    public static final int BOGUS_COMMENT_HYPHEN = 64;

    public static final int RAWTEXT_RCDATA_LESS_THAN_SIGN = 65;

    public static final int SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN = 66;

    public static final int SCRIPT_DATA_DOUBLE_ESCAPE_START = 67;

    public static final int SCRIPT_DATA_DOUBLE_ESCAPED = 68;

    public static final int SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN = 69;

    public static final int SCRIPT_DATA_DOUBLE_ESCAPED_DASH = 70;

    public static final int SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH = 71;

    public static final int SCRIPT_DATA_DOUBLE_ESCAPE_END = 72;

    public static final int PROCESSING_INSTRUCTION = 73;

    public static final int PROCESSING_INSTRUCTION_QUESTION_MARK = 74;

    


    private static final int LEAD_OFFSET = (0xD800 - (0x10000 >> 10));

    



    private static final @NoLength char[] LT_GT = { '<', '>' };

    



    private static final @NoLength char[] LT_SOLIDUS = { '<', '/' };

    



    private static final @NoLength char[] RSQB_RSQB = { ']', ']' };

    


    private static final @NoLength char[] REPLACEMENT_CHARACTER = { '\uFFFD' };

    

    


    private static final @NoLength char[] SPACE = { ' ' };

    

    


    private static final @NoLength char[] LF = { '\n' };

    


    private static final int BUFFER_GROW_BY = 1024;

    


    private static final @NoLength char[] CDATA_LSQB = { 'C', 'D', 'A', 'T',
            'A', '[' };

    


    private static final @NoLength char[] OCTYPE = { 'o', 'c', 't', 'y', 'p',
            'e' };

    


    private static final @NoLength char[] UBLIC = { 'u', 'b', 'l', 'i', 'c' };

    


    private static final @NoLength char[] YSTEM = { 'y', 's', 't', 'e', 'm' };

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

    private int firstCharKey;

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

    


    private @Auto char[] strBuf;

    


    private int strBufLen;

    



    
    


    private @Auto char[] longStrBuf;

    


    private int longStrBufLen;

    



    

    


    private final @Auto char[] bmpChar;

    


    private final @Auto char[] astralChar;

    


    protected ElementName endTagExpectation = null;

    private char[] endTagExpectationAsArray; 

    


    protected boolean endTag;

    


    private ElementName tagName = null;

    


    protected AttributeName attributeName = null;

    

    


    private boolean wantsComments = false;

    


    protected boolean html4;

    


    private boolean metaBoundaryPassed;

    

    


    private @Local String doctypeName;

    


    private String publicIdentifier;

    


    private String systemIdentifier;

    


    private HtmlAttributes attributes;

    

    


    private XmlViolationPolicy contentSpacePolicy = XmlViolationPolicy.ALTER_INFOSET;

    


    private XmlViolationPolicy commentPolicy = XmlViolationPolicy.ALTER_INFOSET;

    private XmlViolationPolicy xmlnsPolicy = XmlViolationPolicy.ALTER_INFOSET;

    private XmlViolationPolicy namePolicy = XmlViolationPolicy.ALTER_INFOSET;

    private boolean html4ModeCompatibleWithXhtml1Schemata;

    private int mappingLangToXmlLang;

    

    private final boolean newAttributesEachTime;

    private boolean shouldSuspend;

    protected boolean confident;

    private int line;

    private Interner interner;

    

    

    protected LocatorImpl ampersandLocation;

    public Tokenizer(TokenHandler tokenHandler, boolean newAttributesEachTime) {
        this.tokenHandler = tokenHandler;
        this.encodingDeclarationHandler = null;
        this.newAttributesEachTime = newAttributesEachTime;
        this.bmpChar = new char[1];
        this.astralChar = new char[2];
        this.tagName = null;
        this.attributeName = null;
        this.doctypeName = null;
        this.publicIdentifier = null;
        this.systemIdentifier = null;
        this.attributes = null;
    }

    

    





    public Tokenizer(TokenHandler tokenHandler
    
    ) {
        this.tokenHandler = tokenHandler;
        this.encodingDeclarationHandler = null;
        
        this.newAttributesEachTime = false;
        
        this.bmpChar = new char[1];
        this.astralChar = new char[2];
        this.tagName = null;
        this.attributeName = null;
        this.doctypeName = null;
        this.publicIdentifier = null;
        this.systemIdentifier = null;
        
        this.attributes = null;
        
        
        
        
    }

    public void setInterner(Interner interner) {
        this.interner = interner;
    }

    public void initLocation(String newPublicId, String newSystemId) {
        this.systemId = newSystemId;
        this.publicId = newPublicId;

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

    

    
    









    public void setStateAndEndTagExpectation(int specialTokenizerState,
            @Local String endTagExpectation) {
        this.stateSave = specialTokenizerState;
        if (specialTokenizerState == Tokenizer.DATA) {
            return;
        }
        @Auto char[] asArray = Portability.newCharArrayFromLocal(endTagExpectation);
        this.endTagExpectation = ElementName.elementNameByBuffer(asArray, 0,
                asArray.length, interner);
        endTagExpectationToArray();
    }

    









    public void setStateAndEndTagExpectation(int specialTokenizerState,
            ElementName endTagExpectation) {
        this.stateSave = specialTokenizerState;
        this.endTagExpectation = endTagExpectation;
        endTagExpectationToArray();
    }

    private void endTagExpectationToArray() {
        switch (endTagExpectation.getGroup()) {
            case TreeBuilder.TITLE:
                endTagExpectationAsArray = TITLE_ARR;
                return;
            case TreeBuilder.SCRIPT:
                endTagExpectationAsArray = SCRIPT_ARR;
                return;
            case TreeBuilder.STYLE:
                endTagExpectationAsArray = STYLE_ARR;
                return;
            case TreeBuilder.PLAINTEXT:
                endTagExpectationAsArray = PLAINTEXT_ARR;
                return;
            case TreeBuilder.XMP:
                endTagExpectationAsArray = XMP_ARR;
                return;
            case TreeBuilder.TEXTAREA:
                endTagExpectationAsArray = TEXTAREA_ARR;
                return;
            case TreeBuilder.IFRAME:
                endTagExpectationAsArray = IFRAME_ARR;
                return;
            case TreeBuilder.NOEMBED:
                endTagExpectationAsArray = NOEMBED_ARR;
                return;
            case TreeBuilder.NOSCRIPT:
                endTagExpectationAsArray = NOSCRIPT_ARR;
                return;
            case TreeBuilder.NOFRAMES:
                endTagExpectationAsArray = NOFRAMES_ARR;
                return;
            default:
                assert false: "Bad end tag expectation.";
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

    @Inline private void clearStrBufAndAppend(char c) {
        strBuf[0] = c;
        strBufLen = 1;
    }

    @Inline private void clearStrBuf() {
        strBufLen = 0;
    }

    





    private void appendStrBuf(char c) {
        if (strBufLen == strBuf.length) {
            char[] newBuf = new char[strBuf.length + Tokenizer.BUFFER_GROW_BY];
            System.arraycopy(strBuf, 0, newBuf, 0, strBuf.length);
            strBuf = newBuf;
        }
        strBuf[strBufLen++] = c;
    }

    







    protected String strBufToString() {
        return Portability.newStringFromBuffer(strBuf, 0, strBufLen);
    }

    





    private void strBufToDoctypeName() {
        doctypeName = Portability.newLocalNameFromBuffer(strBuf, 0, strBufLen,
                interner);
    }

    





    private void emitStrBuf() throws SAXException {
        if (strBufLen > 0) {
            tokenHandler.characters(strBuf, 0, strBufLen);
        }
    }

    @Inline private void clearLongStrBuf() {
        longStrBufLen = 0;
    }

    @Inline private void clearLongStrBufAndAppend(char c) {
        longStrBuf[0] = c;
        longStrBufLen = 1;
    }

    





    private void appendLongStrBuf(char c) {
        if (longStrBufLen == longStrBuf.length) {
            char[] newBuf = new char[longStrBufLen + (longStrBufLen >> 1)];
            System.arraycopy(longStrBuf, 0, newBuf, 0, longStrBuf.length);
            longStrBuf = newBuf;
        }
        longStrBuf[longStrBufLen++] = c;
    }

    @Inline private void appendSecondHyphenToBogusComment() throws SAXException {
        
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

    

    @Inline private void adjustDoubleHyphenAndAppendToLongStrBufAndErr(char c)
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

    private void appendLongStrBuf(@NoLength char[] buffer, int offset, int length) {
        int reqLen = longStrBufLen + length;
        if (longStrBuf.length < reqLen) {
            char[] newBuf = new char[reqLen + (reqLen >> 1)];
            System.arraycopy(longStrBuf, 0, newBuf, 0, longStrBuf.length);
            longStrBuf = newBuf;
        }
        System.arraycopy(buffer, offset, longStrBuf, longStrBufLen, length);
        longStrBufLen = reqLen;
    }

    


    @Inline private void appendStrBufToLongStrBuf() {
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
        cstart = Integer.MAX_VALUE;
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

    private void strBufToElementNameString() {
        
        
        
        tagName = ElementName.elementNameByBuffer(strBuf, 0, strBufLen,
                interner);
        
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
        tagName.release();
        tagName = null;
        if (newAttributesEachTime) {
            attributes = null;
        } else {
            attributes.clear(mappingLangToXmlLang);
        }
        



        return stateSave;
    }

    private void attributeNameComplete() throws SAXException {
        
        
        
        
        attributeName = AttributeName.nameByBuffer(strBuf, 0, strBufLen
        
                , namePolicy != XmlViolationPolicy.ALLOW
                
                , interner);
        

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
        noteAttributeWithoutValue();

        
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
                    if (AttributeName.BORDER != attributeName) {
                        err("Attribute value omitted for a non-boolean attribute. (HTML4-only error.)");
                        attributes.addAttribute(attributeName, "", xmlnsPolicy);
                    }
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
            
            attributeName = null; 
            
        }
    }

    private void addAttributeWithValue() throws SAXException {
        
        if (metaBoundaryPassed && ElementName.META == tagName
                && AttributeName.CHARSET == attributeName) {
            err("A \u201Ccharset\u201D attribute on a \u201Cmeta\u201D element found after the first 512 bytes.");
        }
        
        if (attributeName != null) {
            String val = longStrBufToString(); 
            
            
            
            
            
            if (!endTag && html4 && html4ModeCompatibleWithXhtml1Schemata
                    && attributeName.isCaseFolded()) {
                val = newAsciiLowerCaseStringFromString(val);
            }
            
            attributes.addAttribute(attributeName, val
            
                    , xmlnsPolicy
            
            );
            attributeName = null; 
            
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
        initializeWithoutStarting();
        tokenHandler.startTokenization(this);
        
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
            case SCRIPT_DATA:
            case PLAINTEXT:
            case RAWTEXT:
            case CDATA_SECTION:
            case SCRIPT_DATA_ESCAPED:
            case SCRIPT_DATA_ESCAPE_START:
            case SCRIPT_DATA_ESCAPE_START_DASH:
            case SCRIPT_DATA_ESCAPED_DASH:
            case SCRIPT_DATA_ESCAPED_DASH_DASH:
            case SCRIPT_DATA_DOUBLE_ESCAPE_START:
            case SCRIPT_DATA_DOUBLE_ESCAPED:
            case SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN:
            case SCRIPT_DATA_DOUBLE_ESCAPED_DASH:
            case SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH:
            case SCRIPT_DATA_DOUBLE_ESCAPE_END:
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

    @SuppressWarnings("unused") private int stateLoop(int state, char c,
            int pos, @NoLength char[] buf, boolean reconsume, int returnState,
            int endPos) throws SAXException {
        



































































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
                                clearStrBufAndAppend(c);
                                setAdditionalAndRememberAmpersandLocation('\u0000');
                                returnState = state;
                                state = transition(state, Tokenizer.CONSUME_CHARACTER_REFERENCE, reconsume, pos);
                                continue stateloop;
                            case '<':
                                



                                flushChars(buf, pos);

                                state = transition(state, Tokenizer.TAG_OPEN, reconsume, pos);
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
                            




                            clearStrBufAndAppend((char) (c + 0x20));
                            
                            state = transition(state, Tokenizer.TAG_NAME, reconsume, pos);
                            



                            break tagopenloop;
                            
                        } else if (c >= 'a' && c <= 'z') {
                            




                            endTag = false;
                            


                            clearStrBufAndAppend(c);
                            
                            state = transition(state, Tokenizer.TAG_NAME, reconsume, pos);
                            



                            break tagopenloop;
                            
                        }
                        switch (c) {
                            case '!':
                                



                                state = transition(state, Tokenizer.MARKUP_DECLARATION_OPEN, reconsume, pos);
                                continue stateloop;
                            case '/':
                                



                                state = transition(state, Tokenizer.CLOSE_TAG_OPEN, reconsume, pos);
                                continue stateloop;
                            case '?':
                                
                                
                                
                                
                                
                                
                                
                                


                                errProcessingInstruction();
                                


                                clearLongStrBufAndAppend(c);
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                                continue stateloop;
                            case '>':
                                


                                errLtGt();
                                




                                tokenHandler.characters(Tokenizer.LT_GT, 0, 2);
                                
                                cstart = pos + 1;
                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            default:
                                


                                errBadCharAfterLt(c);
                                


                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                



                                cstart = pos;
                                reconsume = true;
                                state = transition(state, Tokenizer.DATA, reconsume, pos);
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
                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                strBufToElementNameString();
                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                break tagnameloop;
                            
                            case '/':
                                



                                strBufToElementNameString();
                                state = transition(state, Tokenizer.SELF_CLOSING_START_TAG, reconsume, pos);
                                continue stateloop;
                            case '>':
                                



                                strBufToElementNameString();
                                state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
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
                                



                                state = transition(state, Tokenizer.SELF_CLOSING_START_TAG, reconsume, pos);
                                continue stateloop;
                            case '>':
                                



                                state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
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
                                



                                clearStrBufAndAppend(c);
                                


                                
                                


                                state = transition(state, Tokenizer.ATTRIBUTE_NAME, reconsume, pos);
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
                                state = transition(state, Tokenizer.AFTER_ATTRIBUTE_NAME, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                attributeNameComplete();
                                state = transition(state, Tokenizer.AFTER_ATTRIBUTE_NAME, reconsume, pos);
                                continue stateloop;
                            case '/':
                                



                                attributeNameComplete();
                                addAttributeWithoutValue();
                                state = transition(state, Tokenizer.SELF_CLOSING_START_TAG, reconsume, pos);
                                continue stateloop;
                            case '=':
                                



                                attributeNameComplete();
                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_VALUE, reconsume, pos);
                                break attributenameloop;
                            
                            case '>':
                                



                                attributeNameComplete();
                                addAttributeWithoutValue();
                                state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
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
                                



                                clearLongStrBuf();
                                state = transition(state, Tokenizer.ATTRIBUTE_VALUE_DOUBLE_QUOTED, reconsume, pos);
                                break beforeattributevalueloop;
                            
                            case '&':
                                




                                clearLongStrBuf();
                                reconsume = true;
                                state = transition(state, Tokenizer.ATTRIBUTE_VALUE_UNQUOTED, reconsume, pos);
                                noteUnquotedAttributeValue();
                                continue stateloop;
                            case '\'':
                                



                                clearLongStrBuf();
                                state = transition(state, Tokenizer.ATTRIBUTE_VALUE_SINGLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '>':
                                


                                errAttributeValueMissing();
                                


                                addAttributeWithoutValue();
                                state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            case '<':
                            case '=':
                            case '`':
                                



                                errLtOrEqualsOrGraveInUnquotedAttributeOrNull(c);
                                



                            default:
                                
                                errHtml4NonNameInUnquotedAttribute(c);
                                
                                



                                clearLongStrBufAndAppend(c);
                                




                                state = transition(state, Tokenizer.ATTRIBUTE_VALUE_UNQUOTED, reconsume, pos);
                                noteUnquotedAttributeValue();
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

                                state = transition(state, Tokenizer.AFTER_ATTRIBUTE_VALUE_QUOTED, reconsume, pos);
                                break attributevaluedoublequotedloop;
                            
                            case '&':
                                





                                clearStrBufAndAppend(c);
                                setAdditionalAndRememberAmpersandLocation('\"');
                                returnState = state;
                                state = transition(state, Tokenizer.CONSUME_CHARACTER_REFERENCE, reconsume, pos);
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
                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                continue stateloop;
                            case '/':
                                



                                state = transition(state, Tokenizer.SELF_CLOSING_START_TAG, reconsume, pos);
                                break afterattributevaluequotedloop;
                            
                            case '>':
                                



                                state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
                                if (shouldSuspend) {
                                    break stateloop;
                                }
                                


                                continue stateloop;
                            default:
                                


                                errNoSpaceBetweenAttributes();
                                



                                reconsume = true;
                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
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
                            
                            state = transition(state, emitCurrentTagToken(true, pos), reconsume, pos);
                            if (shouldSuspend) {
                                break stateloop;
                            }
                            


                            continue stateloop;
                        default:
                            
                            errSlashNotFollowedByGt();
                            



                            reconsume = true;
                            state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
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
                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                addAttributeWithValue();
                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                continue stateloop;
                            case '&':
                                





                                clearStrBufAndAppend(c);
                                setAdditionalAndRememberAmpersandLocation('>');
                                returnState = state;
                                state = transition(state, Tokenizer.CONSUME_CHARACTER_REFERENCE, reconsume, pos);
                                continue stateloop;
                            case '>':
                                



                                addAttributeWithValue();
                                state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
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
                            case '`':
                                




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
                                state = transition(state, Tokenizer.SELF_CLOSING_START_TAG, reconsume, pos);
                                continue stateloop;
                            case '=':
                                



                                state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_VALUE, reconsume, pos);
                                continue stateloop;
                            case '>':
                                



                                addAttributeWithoutValue();
                                state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
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
                                



                                clearStrBufAndAppend(c);
                                


                                
                                


                                state = transition(state, Tokenizer.ATTRIBUTE_NAME, reconsume, pos);
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
                                clearLongStrBufAndAppend(c);
                                state = transition(state, Tokenizer.MARKUP_DECLARATION_HYPHEN, reconsume, pos);
                                break markupdeclarationopenloop;
                            
                            case 'd':
                            case 'D':
                                clearLongStrBufAndAppend(c);
                                index = 0;
                                state = transition(state, Tokenizer.MARKUP_DECLARATION_OCTYPE, reconsume, pos);
                                continue stateloop;
                            case '[':
                                if (tokenHandler.cdataSectionAllowed()) {
                                    clearLongStrBufAndAppend(c);
                                    index = 0;
                                    state = transition(state, Tokenizer.CDATA_START, reconsume, pos);
                                    continue stateloop;
                                }
                                
                            default:
                                errBogusComment();
                                clearLongStrBuf();
                                reconsume = true;
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
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
                                clearLongStrBuf();
                                state = transition(state, Tokenizer.COMMENT_START, reconsume, pos);
                                break markupdeclarationhyphenloop;
                            
                            default:
                                errBogusComment();
                                reconsume = true;
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
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
                                state = transition(state, Tokenizer.COMMENT_START_DASH, reconsume, pos);
                                continue stateloop;
                            case '>':
                                


                                errPrematureEndOfComment();
                                
                                emitComment(0, pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                                break commentstartloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                



                                appendLongStrBuf(c);
                                


                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
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
                                state = transition(state, Tokenizer.COMMENT_END_DASH, reconsume, pos);
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
                                state = transition(state, Tokenizer.COMMENT_END, reconsume, pos);
                                break commentenddashloop;
                            
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                appendLongStrBuf(c);
                                


                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
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
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '-':
                                
                                



                                adjustDoubleHyphenAndAppendToLongStrBufAndErr(c);
                                


                                continue;
                            case '\r':
                                adjustDoubleHyphenAndAppendToLongStrBufCarriageReturn();
                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                                break stateloop;
                            case '\n':
                                adjustDoubleHyphenAndAppendToLongStrBufLineFeed();
                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                                continue stateloop;
                            case '!':
                                errHyphenHyphenBang();
                                appendLongStrBuf(c);
                                state = transition(state, Tokenizer.COMMENT_END_BANG, reconsume, pos);
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                




                                adjustDoubleHyphenAndAppendToLongStrBufAndErr(c);
                                


                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
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
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '-':
                                




                                appendLongStrBuf(c);
                                


                                state = transition(state, Tokenizer.COMMENT_END_DASH, reconsume, pos);
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
                                


                                state = transition(state, Tokenizer.COMMENT, reconsume, pos);
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
                            state = transition(state, Tokenizer.COMMENT_END, reconsume, pos);
                            continue stateloop;
                        case '>':
                            errPrematureEndOfComment();
                            
                            emitComment(1, pos);
                            


                            state = transition(state, Tokenizer.DATA, reconsume, pos);
                            continue stateloop;
                        case '\r':
                            appendLongStrBufCarriageReturn();
                            state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                            break stateloop;
                        case '\n':
                            appendLongStrBufLineFeed();
                            state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                            continue stateloop;
                        case '\u0000':
                            c = '\uFFFD';
                            
                        default:
                            




                            appendLongStrBuf(c);
                            


                            state = transition(state, Tokenizer.COMMENT, reconsume, pos);
                            continue stateloop;
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
                                reconsume = true;
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                                continue stateloop;
                            }
                            index++;
                            continue;
                        } else {
                            cstart = pos; 
                            reconsume = true;
                            state = transition(state, Tokenizer.CDATA_SECTION, reconsume, pos);
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
                                state = transition(state, Tokenizer.CDATA_RSQB, reconsume, pos);
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
                                state = transition(state, Tokenizer.CDATA_RSQB_RSQB, reconsume, pos);
                                break cdatarsqb;
                            default:
                                tokenHandler.characters(Tokenizer.RSQB_RSQB, 0,
                                        1);
                                cstart = pos;
                                reconsume = true;
                                state = transition(state, Tokenizer.CDATA_SECTION, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case CDATA_RSQB_RSQB:
                    cdatarsqbrsqb: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case ']':
                                
                                
                                
                                
                                tokenHandler.characters(Tokenizer.RSQB_RSQB, 0, 1);                                
                                continue;
                            case '>':
                                cstart = pos + 1;
                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            default:
                                tokenHandler.characters(Tokenizer.RSQB_RSQB, 0, 2);
                                cstart = pos;
                                reconsume = true;
                                state = transition(state, Tokenizer.CDATA_SECTION, reconsume, pos);
                                continue stateloop;
                        }
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

                                state = transition(state, Tokenizer.AFTER_ATTRIBUTE_VALUE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '&':
                                





                                clearStrBufAndAppend(c);
                                setAdditionalAndRememberAmpersandLocation('\'');
                                returnState = state;
                                state = transition(state, Tokenizer.CONSUME_CHARACTER_REFERENCE, reconsume, pos);
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
                            if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                cstart = pos;
                            }
                            reconsume = true;
                            state = transition(state, returnState, reconsume, pos);
                            continue stateloop;
                        case '#':
                            



                            appendStrBuf('#');
                            state = transition(state, Tokenizer.CONSUME_NCR, reconsume, pos);
                            continue stateloop;
                        default:
                            if (c == additional) {
                                emitOrAppendStrBuf(returnState);
                                reconsume = true;
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                            }
                            if (c >= 'a' && c <= 'z') {
                                firstCharKey = c - 'a' + 26;
                            } else if (c >= 'A' && c <= 'Z') {
                                firstCharKey = c - 'A';
                            } else {
                                
                                



                                errNoNamedCharacterMatch();
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos;
                                }
                                reconsume = true;
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                            }
                            
                            appendStrBuf(c);
                            state = transition(state, Tokenizer.CHARACTER_REFERENCE_HILO_LOOKUP, reconsume, pos);
                            
                    }
                    
                case CHARACTER_REFERENCE_HILO_LOOKUP:
                    {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        if (c == '\u0000') {
                            break stateloop;
                        }
                        





































                        int hilo = 0;
                        if (c <= 'z') {
                            @Const @NoLength int[] row = NamedCharactersAccel.HILO_ACCEL[c];
                            if (row != null) {
                                hilo = row[firstCharKey];
                            }
                        }
                        if (hilo == 0) {
                            



                            errNoNamedCharacterMatch();
                            emitOrAppendStrBuf(returnState);
                            if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                cstart = pos;
                            }
                            reconsume = true;
                            state = transition(state, returnState, reconsume, pos);
                            continue stateloop;
                        }
                        
                        appendStrBuf(c);
                        lo = hilo & 0xFFFF;
                        hi = hilo >> 16;
                        entCol = -1;
                        candidate = -1;
                        strBufMark = 0;
                        state = transition(state, Tokenizer.CHARACTER_REFERENCE_TAIL, reconsume, pos);
                        
                    }
                case CHARACTER_REFERENCE_TAIL:
                    outer: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        if (c == '\u0000') {
                            break stateloop;
                        }
                        entCol++;
                        






                        loloop: for (;;) {
                            if (hi < lo) {
                                break outer;
                            }
                            if (entCol == NamedCharacters.NAMES[lo].length()) {
                                candidate = lo;
                                strBufMark = strBufLen;
                                lo++;
                            } else if (entCol > NamedCharacters.NAMES[lo].length()) {
                                break outer;
                            } else if (c > NamedCharacters.NAMES[lo].charAt(entCol)) {
                                lo++;
                            } else {
                                break loloop;
                            }
                        }

                        hiloop: for (;;) {
                            if (hi < lo) {
                                break outer;
                            }
                            if (entCol == NamedCharacters.NAMES[hi].length()) {
                                break hiloop;
                            }
                            if (entCol > NamedCharacters.NAMES[hi].length()) {
                                break outer;
                            } else if (c < NamedCharacters.NAMES[hi].charAt(entCol)) {
                                hi--;
                            } else {
                                break hiloop;
                            }
                        }

                        if (c == ';') {
                            
                            
                            
                            
                            
                            if (entCol + 1 == NamedCharacters.NAMES[lo].length()) {
                                candidate = lo;
                                strBufMark = strBufLen;
                            }                            
                            break outer;
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
                        if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                            cstart = pos;
                        }
                        reconsume = true;
                        state = transition(state, returnState, reconsume, pos);
                        continue stateloop;
                    } else {
                        
                        @Const @CharacterName String candidateName = NamedCharacters.NAMES[candidate];
                        if (candidateName.length() == 0
                                || candidateName.charAt(candidateName.length() - 1) != ';') {
                            



                            if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
                                




                                char ch;
                                if (strBufMark == strBufLen) {
                                    ch = c;
                                } else {
                                    
                                    
                                    
                                    ch = strBuf[strBufMark];
                                    
                                }
                                if (ch == '=' || (ch >= '0' && ch <= '9')
                                        || (ch >= 'A' && ch <= 'Z')
                                        || (ch >= 'a' && ch <= 'z')) {
                                    











                                    errNoNamedCharacterMatch();
                                    appendStrBufToLongStrBuf();
                                    reconsume = true;
                                    state = transition(state, returnState, reconsume, pos);
                                    continue stateloop;
                                }
                            }
                            if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
                                errUnescapedAmpersandInterpretedAsCharacterReference();
                            } else {
                                errNotSemicolonTerminated();
                            }
                        }

                        





                        
                        @Const @NoLength char[] val = NamedCharacters.VALUES[candidate];
                        if (
                        
                        val.length == 1
                        
                        
                        ) {
                            emitOrAppendOne(val, returnState);
                        } else {
                            emitOrAppendTwo(val, returnState);
                        }
                        
                        if (strBufMark < strBufLen) {
                            if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
                                for (int i = strBufMark; i < strBufLen; i++) {
                                    appendLongStrBuf(strBuf[i]);
                                }
                            } else {
                                tokenHandler.characters(strBuf, strBufMark,
                                        strBufLen - strBufMark);
                            }
                        }
                        
                        
                        
                        
                        
                        
                        boolean earlyBreak = (c == ';' && strBufMark == strBufLen);
                        if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                            cstart = earlyBreak ? pos + 1 : pos;
                        }
                        reconsume = !earlyBreak;
                        state = transition(state, returnState, reconsume, pos);
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
                            state = transition(state, Tokenizer.HEX_NCR_LOOP, reconsume, pos);
                            continue stateloop;
                        default:
                            







                            reconsume = true;
                            state = transition(state, Tokenizer.DECIMAL_NRC_LOOP, reconsume, pos);
                            
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
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos + 1;
                                }
                                state = transition(state, Tokenizer.HANDLE_NCR_VALUE, reconsume, pos);
                                
                                break decimalloop;
                            } else {
                                errNoDigitsInNCR();
                                appendStrBuf(';');
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos + 1;
                                }
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                            }
                        } else {
                            










                            if (!seenDigits) {
                                errNoDigitsInNCR();
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos;
                                }
                                reconsume = true;
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                            } else {
                                errCharRefLacksSemicolon();
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos;
                                }
                                reconsume = true;
                                state = transition(state, Tokenizer.HANDLE_NCR_VALUE, reconsume, pos);
                                
                                break decimalloop;
                            }
                        }
                    }
                    
                case HANDLE_NCR_VALUE:
                    
                    
                    handleNcrValue(returnState);
                    state = transition(state, returnState, reconsume, pos);
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
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos + 1;
                                }
                                state = transition(state, Tokenizer.HANDLE_NCR_VALUE, reconsume, pos);
                                continue stateloop;
                            } else {
                                errNoDigitsInNCR();
                                appendStrBuf(';');
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos + 1;
                                }
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                            }
                        } else {
                            










                            if (!seenDigits) {
                                errNoDigitsInNCR();
                                emitOrAppendStrBuf(returnState);
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos;
                                }
                                reconsume = true;
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                            } else {
                                errCharRefLacksSemicolon();
                                if ((returnState & DATA_AND_RCDATA_MASK) == 0) {
                                    cstart = pos;
                                }
                                reconsume = true;
                                state = transition(state, Tokenizer.HANDLE_NCR_VALUE, reconsume, pos);
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
                                emitPlaintextReplacementCharacter(buf, pos);
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
                    
                case CLOSE_TAG_OPEN:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    




                    switch (c) {
                        case '>':
                            
                            errLtSlashGt();
                            


                            cstart = pos + 1;
                            state = transition(state, Tokenizer.DATA, reconsume, pos);
                            continue stateloop;
                        case '\r':
                            silentCarriageReturn();
                            
                            errGarbageAfterLtSlash();
                            


                            clearLongStrBufAndAppend('\n');
                            state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                            break stateloop;
                        case '\n':
                            silentLineFeed();
                            
                            errGarbageAfterLtSlash();
                            


                            clearLongStrBufAndAppend('\n');
                            state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                            continue stateloop;
                        case '\u0000':
                            c = '\uFFFD';
                            
                        default:
                            if (c >= 'A' && c <= 'Z') {
                                c += 0x20;
                            }
                            if (c >= 'a' && c <= 'z') {
                                




                                endTag = true;
                                


                                clearStrBufAndAppend(c);
                                




                                state = transition(state, Tokenizer.TAG_NAME, reconsume, pos);
                                continue stateloop;
                            } else {
                                
                                errGarbageAfterLtSlash();
                                


                                clearLongStrBufAndAppend(c);
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
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
                                clearStrBufAndAppend(c);
                                setAdditionalAndRememberAmpersandLocation('\u0000');
                                returnState = state;
                                state = transition(state, Tokenizer.CONSUME_CHARACTER_REFERENCE, reconsume, pos);
                                continue stateloop;
                            case '<':
                                



                                flushChars(buf, pos);

                                returnState = state;
                                state = transition(state, Tokenizer.RAWTEXT_RCDATA_LESS_THAN_SIGN, reconsume, pos);
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
                    
                case RAWTEXT:
                    rawtextloop: for (;;) {
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
                                state = transition(state, Tokenizer.RAWTEXT_RCDATA_LESS_THAN_SIGN, reconsume, pos);
                                break rawtextloop;
                            
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
                    
                case RAWTEXT_RCDATA_LESS_THAN_SIGN:
                    rawtextrcdatalessthansignloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '/':
                                




                                index = 0;
                                clearStrBuf();
                                state = transition(state, Tokenizer.NON_DATA_END_TAG_NAME, reconsume, pos);
                                break rawtextrcdatalessthansignloop;
                            
                            default:
                                



                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                



                                cstart = pos;
                                reconsume = true;
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case NON_DATA_END_TAG_NAME:
                    for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        





                        if (index < endTagExpectationAsArray.length) {
                            char e = endTagExpectationAsArray[index];
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
                                reconsume = true;
                                state = transition(state, returnState, reconsume, pos);
                                continue stateloop;
                            }
                            appendStrBuf(c);
                            index++;
                            continue;
                        } else {
                            endTag = true;
                            
                            
                            tagName = endTagExpectation;
                            switch (c) {
                                case '\r':
                                    silentCarriageReturn();
                                    state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                    break stateloop;
                                case '\n':
                                    silentLineFeed();
                                    
                                case ' ':
                                case '\t':
                                case '\u000C':
                                    






                                    state = transition(state, Tokenizer.BEFORE_ATTRIBUTE_NAME, reconsume, pos);
                                    continue stateloop;
                                case '/':
                                    





                                    state = transition(state, Tokenizer.SELF_CLOSING_START_TAG, reconsume, pos);
                                    continue stateloop;
                                case '>':
                                    





                                    state = transition(state, emitCurrentTagToken(false, pos), reconsume, pos);
                                    if (shouldSuspend) {
                                        break stateloop;
                                    }
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
                                    state = transition(state, returnState, reconsume, pos);
                                    continue stateloop;
                            }
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
                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '-':
                                appendLongStrBuf(c);
                                state = transition(state, Tokenizer.BOGUS_COMMENT_HYPHEN, reconsume, pos);
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
                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '-':
                                appendSecondHyphenToBogusComment();
                                continue boguscommenthyphenloop;
                            case '\r':
                                appendLongStrBufCarriageReturn();
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                                break stateloop;
                            case '\n':
                                appendLongStrBufLineFeed();
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                appendLongStrBuf(c);
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA:
                    scriptdataloop: for (;;) {
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
                                state = transition(state, Tokenizer.SCRIPT_DATA_LESS_THAN_SIGN, reconsume, pos);
                                break scriptdataloop; 
                            
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
                    
                case SCRIPT_DATA_LESS_THAN_SIGN:
                    scriptdatalessthansignloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '/':
                                




                                index = 0;
                                clearStrBuf();
                                state = transition(state, Tokenizer.NON_DATA_END_TAG_NAME, reconsume, pos);
                                continue stateloop;
                            case '!':
                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                cstart = pos;
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPE_START, reconsume, pos);
                                break scriptdatalessthansignloop; 
                            
                            
                            default:
                                



                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                



                                cstart = pos;
                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_ESCAPE_START:
                    scriptdataescapestartloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '-':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPE_START_DASH, reconsume, pos);
                                break scriptdataescapestartloop; 
                            
                            
                            default:
                                



                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_ESCAPE_START_DASH:
                    scriptdataescapestartdashloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '-':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED_DASH_DASH, reconsume, pos);
                                break scriptdataescapestartdashloop;
                            
                            default:
                                



                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_ESCAPED_DASH_DASH:
                    scriptdataescapeddashdashloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '-':
                                




                                continue;
                            case '<':
                                



                                flushChars(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN, reconsume, pos);
                                continue stateloop;
                            case '>':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA, reconsume, pos);
                                continue stateloop;
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                break scriptdataescapeddashdashloop;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                break scriptdataescapeddashdashloop;
                            
                        }
                    }
                    
                case SCRIPT_DATA_ESCAPED:
                    scriptdataescapedloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '-':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED_DASH, reconsume, pos);
                                break scriptdataescapedloop; 
                            
                            
                            case '<':
                                



                                flushChars(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN, reconsume, pos);
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
                    
                case SCRIPT_DATA_ESCAPED_DASH:
                    scriptdataescapeddashloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '-':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED_DASH_DASH, reconsume, pos);
                                continue stateloop;
                            case '<':
                                



                                flushChars(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN, reconsume, pos);
                                break scriptdataescapeddashloop;
                            
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                continue stateloop;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN:
                    scriptdataescapedlessthanloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '/':
                                




                                index = 0;
                                clearStrBuf();
                                returnState = Tokenizer.SCRIPT_DATA_ESCAPED;
                                state = transition(state, Tokenizer.NON_DATA_END_TAG_NAME, reconsume, pos);
                                continue stateloop;
                            case 'S':
                            case 's':
                                





                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                cstart = pos;
                                index = 1;
                                







                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPE_START, reconsume, pos);
                                break scriptdataescapedlessthanloop;
                            
                            default:
                                





                                tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                                cstart = pos;
                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_DOUBLE_ESCAPE_START:
                    scriptdatadoubleescapestartloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        assert index > 0;
                        if (index < 6) { 
                            char folded = c;
                            if (c >= 'A' && c <= 'Z') {
                                folded += 0x20;
                            }
                            if (folded != Tokenizer.SCRIPT_ARR[index]) {
                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                continue stateloop;
                            }
                            index++;
                            continue;
                        }
                        switch (c) {
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            case ' ':
                            case '\t':
                            case '\u000C':
                            case '/':
                            case '>':
                                








                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                break scriptdatadoubleescapestartloop;
                            
                            default:
                                



                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_DOUBLE_ESCAPED:
                    scriptdatadoubleescapedloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            if (++pos == endPos) {
                                break stateloop;
                            }
                            c = checkChar(buf, pos);
                        }
                        


                        switch (c) {
                            case '-':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED_DASH, reconsume, pos);
                                break scriptdatadoubleescapedloop; 
                            
                            
                            case '<':
                                





                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN, reconsume, pos);
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
                    
                case SCRIPT_DATA_DOUBLE_ESCAPED_DASH:
                    scriptdatadoubleescapeddashloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '-':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH, reconsume, pos);
                                break scriptdatadoubleescapeddashloop;
                            
                            case '<':
                                





                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN, reconsume, pos);
                                continue stateloop;
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                continue stateloop;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH:
                    scriptdatadoubleescapeddashdashloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '-':
                                




                                continue;
                            case '<':
                                





                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN, reconsume, pos);
                                break scriptdatadoubleescapeddashdashloop;
                            case '>':
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA, reconsume, pos);
                                continue stateloop;
                            case '\u0000':
                                emitReplacementCharacter(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                continue stateloop;
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            default:
                                




                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN:
                    scriptdatadoubleescapedlessthanloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        


                        switch (c) {
                            case '/':
                                





                                index = 0;
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPE_END, reconsume, pos);
                                break scriptdatadoubleescapedlessthanloop;
                            default:
                                




                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case SCRIPT_DATA_DOUBLE_ESCAPE_END:
                    scriptdatadoubleescapeendloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        if (index < 6) { 
                            char folded = c;
                            if (c >= 'A' && c <= 'Z') {
                                folded += 0x20;
                            }
                            if (folded != Tokenizer.SCRIPT_ARR[index]) {
                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                continue stateloop;
                            }
                            index++;
                            continue;
                        }
                        switch (c) {
                            case '\r':
                                emitCarriageReturn(buf, pos);
                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                            case ' ':
                            case '\t':
                            case '\u000C':
                            case '/':
                            case '>':
                                








                                state = transition(state, Tokenizer.SCRIPT_DATA_ESCAPED, reconsume, pos);
                                continue stateloop;
                            default:
                                



                                reconsume = true;
                                state = transition(state, Tokenizer.SCRIPT_DATA_DOUBLE_ESCAPED, reconsume, pos);
                                continue stateloop;
                        }
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
                                reconsume = true;
                                state = transition(state, Tokenizer.BOGUS_COMMENT, reconsume, pos);
                                continue stateloop;
                            }
                            index++;
                            continue;
                        } else {
                            reconsume = true;
                            state = transition(state, Tokenizer.DOCTYPE, reconsume, pos);
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
                                state = transition(state, Tokenizer.BEFORE_DOCTYPE_NAME, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                state = transition(state, Tokenizer.BEFORE_DOCTYPE_NAME, reconsume, pos);
                                break doctypeloop;
                            
                            default:
                                


                                errMissingSpaceBeforeDoctypeName();
                                



                                reconsume = true;
                                state = transition(state, Tokenizer.BEFORE_DOCTYPE_NAME, reconsume, pos);
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
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '\u0000':
                                c = '\uFFFD';
                                
                            default:
                                if (c >= 'A' && c <= 'Z') {
                                    







                                    c += 0x20;
                                }
                                
                                



                                clearStrBufAndAppend(c);
                                


                                state = transition(state, Tokenizer.DOCTYPE_NAME, reconsume, pos);
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
                                state = transition(state, Tokenizer.AFTER_DOCTYPE_NAME, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                




                                strBufToDoctypeName();
                                state = transition(state, Tokenizer.AFTER_DOCTYPE_NAME, reconsume, pos);
                                break doctypenameloop;
                            
                            case '>':
                                



                                strBufToDoctypeName();
                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
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
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case 'p':
                            case 'P':
                                index = 0;
                                state = transition(state, Tokenizer.DOCTYPE_UBLIC, reconsume, pos);
                                break afterdoctypenameloop;
                            
                            case 's':
                            case 'S':
                                index = 0;
                                state = transition(state, Tokenizer.DOCTYPE_YSTEM, reconsume, pos);
                                continue stateloop;
                            default:
                                


                                bogusDoctype();

                                



                                
                                


                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
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
                                
                                reconsume = true;
                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
                                continue stateloop;
                            }
                            index++;
                            continue;
                        } else {
                            reconsume = true;
                            state = transition(state, Tokenizer.AFTER_DOCTYPE_PUBLIC_KEYWORD, reconsume, pos);
                            break doctypeublicloop;
                            
                        }
                    }
                    
                case AFTER_DOCTYPE_PUBLIC_KEYWORD:
                    afterdoctypepublickeywordloop: for (;;) {
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
                                state = transition(state, Tokenizer.BEFORE_DOCTYPE_PUBLIC_IDENTIFIER, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                





                                state = transition(state, Tokenizer.BEFORE_DOCTYPE_PUBLIC_IDENTIFIER, reconsume, pos);
                                break afterdoctypepublickeywordloop;
                            
                            case '"':
                                


                                errNoSpaceBetweenDoctypePublicKeywordAndQuote();
                                



                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '\'':
                                


                                errNoSpaceBetweenDoctypePublicKeywordAndQuote();
                                



                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '>':
                                
                                errExpectedPublicId();
                                



                                forceQuirks = true;
                                


                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case BEFORE_DOCTYPE_PUBLIC_IDENTIFIER:
                    beforedoctypepublicidentifierloop: for (;;) {
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
                                




                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED, reconsume, pos);
                                break beforedoctypepublicidentifierloop;
                            
                            case '\'':
                                




                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '>':
                                
                                errExpectedPublicId();
                                



                                forceQuirks = true;
                                


                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
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
                                state = transition(state, Tokenizer.AFTER_DOCTYPE_PUBLIC_IDENTIFIER, reconsume, pos);
                                break doctypepublicidentifierdoublequotedloop;
                            
                            case '>':
                                


                                errGtInPublicId();
                                



                                forceQuirks = true;
                                


                                publicIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
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
                                state = transition(state, Tokenizer.BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                





                                state = transition(state, Tokenizer.BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS, reconsume, pos);
                                break afterdoctypepublicidentifierloop;
                            
                            case '>':
                                



                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '"':
                                


                                errNoSpaceBetweenPublicAndSystemIds();
                                



                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '\'':
                                


                                errNoSpaceBetweenPublicAndSystemIds();
                                



                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS:
                    betweendoctypepublicandsystemidentifiersloop: for (;;) {
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
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            case '"':
                                




                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED, reconsume, pos);
                                break betweendoctypepublicandsystemidentifiersloop;
                            
                            case '\'':
                                




                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
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
                                state = transition(state, Tokenizer.AFTER_DOCTYPE_SYSTEM_IDENTIFIER, reconsume, pos);
                                continue stateloop;
                            case '>':
                                


                                errGtInSystemId();
                                



                                forceQuirks = true;
                                


                                systemIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
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
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            default:
                                




                                bogusDoctypeWithoutQuirks();
                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
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
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
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
                                reconsume = true;
                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
                                continue stateloop;
                            }
                            index++;
                            continue stateloop;
                        } else {
                            reconsume = true;
                            state = transition(state, Tokenizer.AFTER_DOCTYPE_SYSTEM_KEYWORD, reconsume, pos);
                            break doctypeystemloop;
                            
                        }
                    }
                    
                case AFTER_DOCTYPE_SYSTEM_KEYWORD:
                    afterdoctypesystemkeywordloop: for (;;) {
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
                                state = transition(state, Tokenizer.BEFORE_DOCTYPE_SYSTEM_IDENTIFIER, reconsume, pos);
                                break stateloop;
                            case '\n':
                                silentLineFeed();
                                
                            case ' ':
                            case '\t':
                            case '\u000C':
                                





                                state = transition(state, Tokenizer.BEFORE_DOCTYPE_SYSTEM_IDENTIFIER, reconsume, pos);
                                break afterdoctypesystemkeywordloop;
                            
                            case '"':
                                


                                errNoSpaceBetweenDoctypeSystemKeywordAndQuote();
                                



                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '\'':
                                


                                errNoSpaceBetweenDoctypeSystemKeywordAndQuote();
                                



                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '>':
                                
                                errExpectedPublicId();
                                



                                forceQuirks = true;
                                


                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
                                continue stateloop;
                        }
                    }
                    
                case BEFORE_DOCTYPE_SYSTEM_IDENTIFIER:
                    beforedoctypesystemidentifierloop: for (;;) {
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
                                




                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED, reconsume, pos);
                                continue stateloop;
                            case '\'':
                                




                                clearLongStrBuf();
                                



                                state = transition(state, Tokenizer.DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED, reconsume, pos);
                                break beforedoctypesystemidentifierloop;
                            
                            case '>':
                                
                                errExpectedSystemId();
                                



                                forceQuirks = true;
                                


                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
                                continue stateloop;
                            default:
                                bogusDoctype();
                                



                                
                                


                                state = transition(state, Tokenizer.BOGUS_DOCTYPE, reconsume, pos);
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
                                state = transition(state, Tokenizer.AFTER_DOCTYPE_SYSTEM_IDENTIFIER, reconsume, pos);
                                continue stateloop;
                            case '>':
                                errGtInSystemId();
                                



                                forceQuirks = true;
                                


                                systemIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
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
                                state = transition(state, Tokenizer.AFTER_DOCTYPE_PUBLIC_IDENTIFIER, reconsume, pos);
                                continue stateloop;
                            case '>':
                                errGtInPublicId();
                                



                                forceQuirks = true;
                                


                                publicIdentifier = longStrBufToString();
                                emitDoctypeToken(pos);
                                


                                state = transition(state, Tokenizer.DATA, reconsume, pos);
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
                    
                case PROCESSING_INSTRUCTION:
                    processinginstructionloop: for (;;) {
                        if (++pos == endPos) {
                            break stateloop;
                        }
                        c = checkChar(buf, pos);
                        switch (c) {
                            case '?':
                                state = transition(
                                        state,
                                        Tokenizer.PROCESSING_INSTRUCTION_QUESTION_MARK,
                                        reconsume, pos);
                                break processinginstructionloop;
                            
                            default:
                                continue;
                        }
                    }
                case PROCESSING_INSTRUCTION_QUESTION_MARK:
                    if (++pos == endPos) {
                        break stateloop;
                    }
                    c = checkChar(buf, pos);
                    switch (c) {
                        case '>':
                            state = transition(state, Tokenizer.DATA,
                                    reconsume, pos);
                            continue stateloop;
                        default:
                            state = transition(state,
                                    Tokenizer.PROCESSING_INSTRUCTION,
                                    reconsume, pos);
                            continue stateloop;
                    }
                    
            }
        }
        flushChars(buf, pos);
        


        
        stateSave = state;
        returnStateSave = returnState;
        return pos;
    }
    
    
    
    
    
    protected int transition(int from, int to, boolean reconsume, int pos) throws SAXException {
        return to;
    }

    
    
    private void initDoctypeFields() {
        doctypeName = "";
        if (systemIdentifier != null) {
            Portability.releaseString(systemIdentifier);
            systemIdentifier = null;
        }
        if (publicIdentifier != null) {
            Portability.releaseString(publicIdentifier);
            publicIdentifier = null;
        }
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
        flushChars(buf, pos);
        tokenHandler.zeroOriginatingReplacementCharacter();
        cstart = pos + 1;
    }

    private void emitPlaintextReplacementCharacter(@NoLength char[] buf, int pos)
            throws SAXException {
        flushChars(buf, pos);
        tokenHandler.characters(REPLACEMENT_CHARACTER, 0, 1);
        cstart = pos + 1;
    }

    private void setAdditionalAndRememberAmpersandLocation(char add) {
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
        if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
            appendStrBufToLongStrBuf();
        } else {
            emitStrBuf();
        }
    }

    private void handleNcrValue(int returnState) throws SAXException {
        




        if (value <= 0xFFFF) {
            if (value >= 0x80 && value <= 0x9f) {
                



                errNcrInC1Range();
                




                @NoLength char[] val = NamedCharacters.WINDOWS_1252[value - 0x80];
                emitOrAppendOne(val, returnState);
                
            } else if (value == 0xC
                    && contentSpacePolicy != XmlViolationPolicy.ALLOW) {
                if (contentSpacePolicy == XmlViolationPolicy.ALTER_INFOSET) {
                    emitOrAppendOne(Tokenizer.SPACE, returnState);
                } else if (contentSpacePolicy == XmlViolationPolicy.FATAL) {
                    fatal("A character reference expanded to a form feed which is not legal XML 1.0 white space.");
                }
                
            } else if (value == 0x0) {
                errNcrZero();
                emitOrAppendOne(Tokenizer.REPLACEMENT_CHARACTER, returnState);
            } else if ((value & 0xF800) == 0xD800) {
                errNcrSurrogate();
                emitOrAppendOne(Tokenizer.REPLACEMENT_CHARACTER, returnState);
            } else {
                



                char ch = (char) value;
                
                if (value == 0x0D) {
                    errNcrCr();
                } else if ((value <= 0x0008) || (value == 0x000B)
                        || (value >= 0x000E && value <= 0x001F)) {
                    ch = errNcrControlChar(ch);
                } else if (value >= 0xFDD0 && value <= 0xFDEF) {
                    errNcrUnassigned();
                } else if ((value & 0xFFFE) == 0xFFFE) {
                    ch = errNcrNonCharacter(ch);
                } else if (value >= 0x007F && value <= 0x009F) {
                    errNcrControlChar();
                } else {
                    maybeWarnPrivateUse(ch);
                }
                
                bmpChar[0] = ch;
                emitOrAppendOne(bmpChar, returnState);
            }
        } else if (value <= 0x10FFFF) {
            
            maybeWarnPrivateUseAstral();
            if ((value & 0xFFFE) == 0xFFFE) {
                errAstralNonCharacter(value);
            }
            
            astralChar[0] = (char) (Tokenizer.LEAD_OFFSET + (value >> 10));
            astralChar[1] = (char) (0xDC00 + (value & 0x3FF));
            emitOrAppendTwo(astralChar, returnState);
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
                case SCRIPT_DATA_LESS_THAN_SIGN:
                case SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN:
                    


                    tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                    



                    break eofloop;
                case TAG_OPEN:
                    



                    


                    errEofAfterLt();
                    


                    tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                    



                    break eofloop;
                case RAWTEXT_RCDATA_LESS_THAN_SIGN:
                    


                    tokenHandler.characters(Tokenizer.LT_GT, 0, 1);
                    



                    break eofloop;
                case NON_DATA_END_TAG_NAME:
                    



                    tokenHandler.characters(Tokenizer.LT_SOLIDUS, 0, 2);
                    




                    emitStrBuf();
                    



                    break eofloop;
                case CLOSE_TAG_OPEN:
                    
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
                        if (systemIdentifier != null) {
                            Portability.releaseString(systemIdentifier);
                            systemIdentifier = null;
                        }
                        if (publicIdentifier != null) {
                            Portability.releaseString(publicIdentifier);
                            publicIdentifier = null;
                        }
                        forceQuirks = true;
                        


                        emitDoctypeToken(0);
                        


                        break eofloop;
                    }
                    break eofloop;
                case COMMENT_START:
                case COMMENT:
                    


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
                case AFTER_DOCTYPE_PUBLIC_KEYWORD:
                case AFTER_DOCTYPE_SYSTEM_KEYWORD:
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
                case BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS:
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
                case CHARACTER_REFERENCE_HILO_LOOKUP:
                    errNoNamedCharacterMatch();
                    emitOrAppendStrBuf(returnState);
                    state = returnState;
                    continue;
                case CHARACTER_REFERENCE_TAIL:
                    outer: for (;;) {
                        char c = '\u0000';
                        entCol++;
                        






                        hiloop: for (;;) {
                            if (hi == -1) {
                                break hiloop;
                            }
                            if (entCol == NamedCharacters.NAMES[hi].length()) {
                                break hiloop;
                            }
                            if (entCol > NamedCharacters.NAMES[hi].length()) {
                                break outer;
                            } else if (c < NamedCharacters.NAMES[hi].charAt(entCol)) {
                                hi--;
                            } else {
                                break hiloop;
                            }
                        }

                        loloop: for (;;) {
                            if (hi < lo) {
                                break outer;
                            }
                            if (entCol == NamedCharacters.NAMES[lo].length()) {
                                candidate = lo;
                                strBufMark = strBufLen;
                                lo++;
                            } else if (entCol > NamedCharacters.NAMES[lo].length()) {
                                break outer;
                            } else if (c > NamedCharacters.NAMES[lo].charAt(entCol)) {
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
                        @Const @CharacterName String candidateName = NamedCharacters.NAMES[candidate];
                        if (candidateName.length() == 0
                                || candidateName.charAt(candidateName.length() - 1) != ';') {
                            



                            if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
                                




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
                            if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
                                errUnescapedAmpersandInterpretedAsCharacterReference();
                            } else {
                                errNotSemicolonTerminated();
                            }
                        }

                        





                        @Const @NoLength char[] val = NamedCharacters.VALUES[candidate];
                        if (
                        
                        val.length == 1
                        
                        
                        ) {
                            emitOrAppendOne(val, returnState);
                        } else {
                            emitOrAppendTwo(val, returnState);
                        }
                        
                        if (strBufMark < strBufLen) {
                            if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
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
                case CDATA_RSQB:
                    tokenHandler.characters(Tokenizer.RSQB_RSQB, 0, 1);
                    break eofloop;
                case CDATA_RSQB_RSQB:
                    tokenHandler.characters(Tokenizer.RSQB_RSQB, 0, 2);
                    break eofloop;
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
        
        
        
        doctypeName = null;
        Portability.releaseString(publicIdentifier);
        publicIdentifier = null;
        Portability.releaseString(systemIdentifier);
        systemIdentifier = null;
    }

    @Inline protected char checkChar(@NoLength char[] buf, int pos)
            throws SAXException {
        return buf[pos];
    }

    public boolean internalEncodingDeclaration(String internalCharset)
            throws SAXException {
        if (encodingDeclarationHandler != null) {
            return encodingDeclarationHandler.internalEncodingDeclaration(internalCharset);
        }
        return false;
    }

    



    private void emitOrAppendTwo(@Const @NoLength char[] val, int returnState)
            throws SAXException {
        if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
            appendLongStrBuf(val[0]);
            appendLongStrBuf(val[1]);
        } else {
            tokenHandler.characters(val, 0, 2);
        }
    }

    private void emitOrAppendOne(@Const @NoLength char[] val, int returnState)
            throws SAXException {
        if ((returnState & DATA_AND_RCDATA_MASK) != 0) {
            appendLongStrBuf(val[0]);
        } else {
            tokenHandler.characters(val, 0, 1);
        }
    }

    public void end() throws SAXException {
        strBuf = null;
        longStrBuf = null;
        doctypeName = null;
        if (systemIdentifier != null) {
            Portability.releaseString(systemIdentifier);
            systemIdentifier = null;
        }
        if (publicIdentifier != null) {
            Portability.releaseString(publicIdentifier);
            publicIdentifier = null;
        }
        if (tagName != null) {
            tagName.release();
            tagName = null;
        }
        if (attributeName != null) {
            attributeName.release();
            attributeName = null;
        }
        tokenHandler.endTokenization();
        if (attributes != null) {
            
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

    public void resetToDataState() {
        strBufLen = 0;
        longStrBufLen = 0;
        stateSave = Tokenizer.DATA;
        
        lastCR = false;
        index = 0;
        forceQuirks = false;
        additional = '\u0000';
        entCol = -1;
        firstCharKey = -1;
        lo = 0;
        hi = 0; 
        candidate = -1;
        strBufMark = 0;
        prevValue = -1;
        value = 0;
        seenDigits = false;
        endTag = false;
        shouldSuspend = false;
        initDoctypeFields();
        if (tagName != null) {
            tagName.release();
            tagName = null;
        }
        if (attributeName != null) {
            attributeName.release();
            attributeName = null;
        }
        if (newAttributesEachTime) {
            if (attributes != null) {
                Portability.delete(attributes);
                attributes = null;
            }
        }
    }

    public void loadState(Tokenizer other) throws SAXException {
        strBufLen = other.strBufLen;
        if (strBufLen > strBuf.length) {
            strBuf = new char[strBufLen];
        }
        System.arraycopy(other.strBuf, 0, strBuf, 0, strBufLen);

        longStrBufLen = other.longStrBufLen;
        if (longStrBufLen > longStrBuf.length) {
            longStrBuf = new char[longStrBufLen];
        }
        System.arraycopy(other.longStrBuf, 0, longStrBuf, 0, longStrBufLen);

        stateSave = other.stateSave;
        returnStateSave = other.returnStateSave;
        endTagExpectation = other.endTagExpectation;
        endTagExpectationAsArray = other.endTagExpectationAsArray;
        
        lastCR = other.lastCR;
        index = other.index;
        forceQuirks = other.forceQuirks;
        additional = other.additional;
        entCol = other.entCol;
        firstCharKey = other.firstCharKey;
        lo = other.lo;
        hi = other.hi;
        candidate = other.candidate;
        strBufMark = other.strBufMark;
        prevValue = other.prevValue;
        value = other.value;
        seenDigits = other.seenDigits;
        endTag = other.endTag;
        shouldSuspend = false;

        if (other.doctypeName == null) {
            doctypeName = null;
        } else {
            doctypeName = Portability.newLocalFromLocal(other.doctypeName,
                    interner);
        }

        Portability.releaseString(systemIdentifier);
        if (other.systemIdentifier == null) {
            systemIdentifier = null;
        } else {
            systemIdentifier = Portability.newStringFromString(other.systemIdentifier);
        }

        Portability.releaseString(publicIdentifier);
        if (other.publicIdentifier == null) {
            publicIdentifier = null;
        } else {
            publicIdentifier = Portability.newStringFromString(other.publicIdentifier);
        }

        if (tagName != null) {
            tagName.release();
        }
        if (other.tagName == null) {
            tagName = null;
        } else {
            tagName = other.tagName.cloneElementName(interner);
        }

        if (attributeName != null) {
            attributeName.release();
        }
        if (other.attributeName == null) {
            attributeName = null;
        } else {
            attributeName = other.attributeName.cloneAttributeName(interner);
        }

        Portability.delete(attributes);
        if (other.attributes == null) {
            attributes = null;
        } else {
            attributes = other.attributes.cloneAttributes(interner);
        }
    }

    public void initializeWithoutStarting() throws SAXException {
        confident = false;
        strBuf = new char[64];
        longStrBuf = new char[1024];
        line = 1;
        
        html4 = false;
        metaBoundaryPassed = false;
        wantsComments = tokenHandler.wantsComments();
        if (!newAttributesEachTime) {
            attributes = new HtmlAttributes(mappingLangToXmlLang);
        }
        
        resetToDataState();
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

    protected void errLtOrEqualsOrGraveInUnquotedAttributeOrNull(char c)
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

    protected void errQuoteOrLtInAttributeNameOrNull(char c)
            throws SAXException {
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

    protected char errNcrNonCharacter(char ch) throws SAXException {
        return ch;
    }

    protected void errAstralNonCharacter(int ch) throws SAXException {
    }

    protected void errNcrSurrogate() throws SAXException {
    }

    protected char errNcrControlChar(char ch) throws SAXException {
        return ch;
    }

    protected void errNcrCr() throws SAXException {
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

    protected void errNcrControlChar() throws SAXException {
    }

    protected void errNcrZero() throws SAXException {
    }

    protected void errNoSpaceBetweenDoctypeSystemKeywordAndQuote()
            throws SAXException {
    }

    protected void errNoSpaceBetweenPublicAndSystemIds() throws SAXException {
    }

    protected void errNoSpaceBetweenDoctypePublicKeywordAndQuote()
            throws SAXException {
    }

    protected void noteAttributeWithoutValue() throws SAXException {
    }

    protected void noteUnquotedAttributeValue() throws SAXException {
    }

    





    public void setEncodingDeclarationHandler(
            EncodingDeclarationHandler encodingDeclarationHandler) {
        this.encodingDeclarationHandler = encodingDeclarationHandler;
    }
    
    void destructor() {
        
        Portability.delete(attributes);
        attributes = null;
    }
    
    
    
    





    public void setTransitionBaseOffset(int offset) {
        
    }
    
    

}
