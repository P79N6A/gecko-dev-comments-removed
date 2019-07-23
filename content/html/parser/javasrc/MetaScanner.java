






















package nu.validator.htmlparser.impl;

import java.io.IOException;

import nu.validator.htmlparser.annotation.NoLength;
import nu.validator.htmlparser.common.ByteReadable;

import org.xml.sax.SAXException;

public abstract class MetaScanner {

    private static final @NoLength char[] CHARSET = "charset".toCharArray();
    
    private static final @NoLength char[] CONTENT = "content".toCharArray();

    private static final int NO = 0;

    private static final int M = 1;
    
    private static final int E = 2;
    
    private static final int T = 3;

    private static final int A = 4;
    
    private static final int DATA = 0;

    private static final int TAG_OPEN = 1;

    private static final int SCAN_UNTIL_GT = 2;

    private static final int TAG_NAME = 3;

    private static final int BEFORE_ATTRIBUTE_NAME = 4;

    private static final int ATTRIBUTE_NAME = 5;

    private static final int AFTER_ATTRIBUTE_NAME = 6;

    private static final int BEFORE_ATTRIBUTE_VALUE = 7;

    private static final int ATTRIBUTE_VALUE_DOUBLE_QUOTED = 8;

    private static final int ATTRIBUTE_VALUE_SINGLE_QUOTED = 9;

    private static final int ATTRIBUTE_VALUE_UNQUOTED = 10;

    private static final int AFTER_ATTRIBUTE_VALUE_QUOTED = 11;

    private static final int MARKUP_DECLARATION_OPEN = 13;
    
    private static final int MARKUP_DECLARATION_HYPHEN = 14;

    private static final int COMMENT_START = 15;

    private static final int COMMENT_START_DASH = 16;

    private static final int COMMENT = 17;

    private static final int COMMENT_END_DASH = 18;

    private static final int COMMENT_END = 19;
    
    private static final int SELF_CLOSING_START_TAG = 20;
    
    protected ByteReadable readable;
    
    private int metaState = NO;

    private int contentIndex = -1;
    
    private int charsetIndex = -1;

    protected int stateSave = DATA;

    private int strBufLen;

    private char[] strBuf;
    
    
    
    





    public MetaScanner() {
        this.readable = null;
        this.metaState = NO;
        this.contentIndex = -1;
        this.charsetIndex = -1;
        this.stateSave = DATA;
        strBufLen = 0;
        strBuf = new char[36];
    }
    
    




    protected int read() throws IOException {
        return readable.readByte();
    }

    

    
    
    protected final void stateLoop(int state)
            throws SAXException, IOException {
        int c = -1;
        boolean reconsume = false;
        stateloop: for (;;) {
            switch (state) {
                case DATA:
                    dataloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            c = read();
                        }
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '<':
                                state = MetaScanner.TAG_OPEN;
                                break dataloop; 
                            
                            default:
                                continue;
                        }
                    }
                    
                case TAG_OPEN:
                    tagopenloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case 'm':
                            case 'M':
                                metaState = M;
                                state = MetaScanner.TAG_NAME;
                                break tagopenloop;
                                
                            case '!':
                                state = MetaScanner.MARKUP_DECLARATION_OPEN;
                                continue stateloop;
                            case '?':
                            case '/':
                                state = MetaScanner.SCAN_UNTIL_GT;
                                continue stateloop;
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            default:
                                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                                    metaState = NO;
                                    state = MetaScanner.TAG_NAME;
                                    break tagopenloop;
                                    
                                }
                                state = MetaScanner.DATA;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case TAG_NAME:
                    tagnameloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case ' ':
                            case '\t':
                            case '\n':
                            case '\u000C':
                                state = MetaScanner.BEFORE_ATTRIBUTE_NAME;
                                break tagnameloop;
                            
                            case '/':
                                state = MetaScanner.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            case 'e':
                            case 'E':
                                if (metaState == M) {
                                    metaState = E;
                                } else {
                                    metaState = NO;
                                }
                                continue;
                            case 't':
                            case 'T':
                                if (metaState == E) {
                                    metaState = T;
                                } else {
                                    metaState = NO;
                                }
                                continue;
                            case 'a':
                            case 'A':
                                if (metaState == T) {
                                    metaState = A;
                                } else {
                                    metaState = NO;
                                }
                                continue;
                            default:
                                metaState = NO;
                                continue;
                        }
                    }
                    
                case BEFORE_ATTRIBUTE_NAME:
                    beforeattributenameloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            c = read();
                        }
                        


                        switch (c) {
                            case -1:
                                break stateloop;
                            case ' ':
                            case '\t':
                            case '\n':
                            case '\u000C':
                                continue;
                            case '/':
                                state = MetaScanner.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '>':
                                state = DATA;
                                continue stateloop;
                            case 'c':
                            case 'C':
                                contentIndex = 0;
                                charsetIndex = 0;
                                state = MetaScanner.ATTRIBUTE_NAME;
                                break beforeattributenameloop;                                
                            default:
                                contentIndex = -1;
                                charsetIndex = -1;
                                state = MetaScanner.ATTRIBUTE_NAME;
                                break beforeattributenameloop;
                            
                        }
                    }
                    
                case ATTRIBUTE_NAME:
                    attributenameloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case ' ':
                            case '\t':
                            case '\n':
                            case '\u000C':
                                state = MetaScanner.AFTER_ATTRIBUTE_NAME;
                                continue stateloop;
                            case '/':
                                state = MetaScanner.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '=':
                                strBufLen = 0;
                                state = MetaScanner.BEFORE_ATTRIBUTE_VALUE;
                                break attributenameloop;
                            
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            default:
                                if (metaState == A) {
                                    if (c >= 'A' && c <= 'Z') {
                                        c += 0x20;
                                    }
                                    if (contentIndex == 6) {
                                        contentIndex = -1;
                                    } else if (contentIndex > -1
                                            && contentIndex < 6
                                            && (c == CONTENT[contentIndex + 1])) {
                                        contentIndex++;
                                    }
                                    if (charsetIndex == 6) {
                                        charsetIndex = -1;
                                    } else if (charsetIndex > -1
                                            && charsetIndex < 6
                                            && (c == CHARSET[charsetIndex + 1])) {
                                        charsetIndex++;
                                    }
                                }
                                continue;
                        }
                    }
                    
                case BEFORE_ATTRIBUTE_VALUE:
                    beforeattributevalueloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case ' ':
                            case '\t':
                            case '\n':
                            case '\u000C':
                                continue;
                            case '"':
                                state = MetaScanner.ATTRIBUTE_VALUE_DOUBLE_QUOTED;
                                break beforeattributevalueloop;
                            
                            case '\'':
                                state = MetaScanner.ATTRIBUTE_VALUE_SINGLE_QUOTED;
                                continue stateloop;
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            default:
                                if (charsetIndex == 6 || contentIndex == 6) {
                                    addToBuffer(c);
                                }
                                state = MetaScanner.ATTRIBUTE_VALUE_UNQUOTED;
                                continue stateloop;
                        }
                    }
                    
                case ATTRIBUTE_VALUE_DOUBLE_QUOTED:
                    attributevaluedoublequotedloop: for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            c = read();
                        }
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '"':
                                if (tryCharset()) {
                                    break stateloop;
                                }
                                state = MetaScanner.AFTER_ATTRIBUTE_VALUE_QUOTED;
                                break attributevaluedoublequotedloop;
                            
                            default:
                                if (metaState == A && (contentIndex == 6 || charsetIndex == 6)) {
                                    addToBuffer(c);
                                }
                                continue;
                        }
                    }
                    
                case AFTER_ATTRIBUTE_VALUE_QUOTED:
                    afterattributevaluequotedloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case ' ':
                            case '\t':
                            case '\n':
                            case '\u000C':
                                state = MetaScanner.BEFORE_ATTRIBUTE_NAME;
                                continue stateloop;
                            case '/':
                                state = MetaScanner.SELF_CLOSING_START_TAG;
                                break afterattributevaluequotedloop;
                            
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            default:
                                state = MetaScanner.BEFORE_ATTRIBUTE_NAME;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case SELF_CLOSING_START_TAG:
                    c = read();
                    switch (c) {
                        case -1:
                            break stateloop;
                        case '>':
                            state = MetaScanner.DATA;
                            continue stateloop;
                        default:
                            state = MetaScanner.BEFORE_ATTRIBUTE_NAME;
                            reconsume = true;
                            continue stateloop;
                    }
                    
                case ATTRIBUTE_VALUE_UNQUOTED:
                    for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            c = read();
                        }
                        switch (c) {
                            case -1:
                                break stateloop;
                            case ' ':
                            case '\t':
                            case '\n':

                            case '\u000C':
                                if (tryCharset()) {
                                    break stateloop;
                                }
                                state = MetaScanner.BEFORE_ATTRIBUTE_NAME;
                                continue stateloop;
                            case '>':
                                if (tryCharset()) {
                                    break stateloop;
                                }
                                state = MetaScanner.DATA;
                                continue stateloop;
                            default:
                                if (metaState == A && (contentIndex == 6 || charsetIndex == 6)) {
                                    addToBuffer(c);
                                }
                                continue;
                        }
                    }
                    
                case AFTER_ATTRIBUTE_NAME:
                    for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case ' ':
                            case '\t':
                            case '\n':
                            case '\u000C':
                                continue;
                            case '/':
                                if (tryCharset()) {
                                    break stateloop;
                                }
                                state = MetaScanner.SELF_CLOSING_START_TAG;
                                continue stateloop;
                            case '=':
                                state = MetaScanner.BEFORE_ATTRIBUTE_VALUE;
                                continue stateloop;
                            case '>':
                                if (tryCharset()) {
                                    break stateloop;
                                }
                                state = MetaScanner.DATA;
                                continue stateloop;
                            case 'c':
                            case 'C':
                                contentIndex = 0;
                                charsetIndex = 0;
                                state = MetaScanner.ATTRIBUTE_NAME;
                                continue stateloop;
                            default:
                                contentIndex = -1;
                                charsetIndex = -1;
                                state = MetaScanner.ATTRIBUTE_NAME;
                                continue stateloop;
                        }
                    }
                    
                case MARKUP_DECLARATION_OPEN:
                    markupdeclarationopenloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '-':
                                state = MetaScanner.MARKUP_DECLARATION_HYPHEN;
                                break markupdeclarationopenloop;
                            
                            default:
                                state = MetaScanner.SCAN_UNTIL_GT;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case MARKUP_DECLARATION_HYPHEN:
                    markupdeclarationhyphenloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '-':
                                state = MetaScanner.COMMENT_START;
                                break markupdeclarationhyphenloop;
                            
                            default:
                                state = MetaScanner.SCAN_UNTIL_GT;
                                reconsume = true;
                                continue stateloop;
                        }
                    }
                    
                case COMMENT_START:
                    commentstartloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '-':
                                state = MetaScanner.COMMENT_START_DASH;
                                continue stateloop;
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            default:
                                state = MetaScanner.COMMENT;
                                break commentstartloop;
                            
                        }
                    }
                    
                case COMMENT:
                    commentloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '-':
                                state = MetaScanner.COMMENT_END_DASH;
                                break commentloop;
                            
                            default:
                                continue;
                        }
                    }
                    
                case COMMENT_END_DASH:
                    commentenddashloop: for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '-':
                                state = MetaScanner.COMMENT_END;
                                break commentenddashloop;
                            
                            default:
                                state = MetaScanner.COMMENT;
                                continue stateloop;
                        }
                    }
                    
                case COMMENT_END:
                    for (;;) {
                        c = read();
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            case '-':
                                continue;
                            default:
                                state = MetaScanner.COMMENT;
                                continue stateloop;
                        }
                    }
                    
                case COMMENT_START_DASH:
                    c = read();
                    switch (c) {
                        case -1:
                            break stateloop;
                        case '-':
                            state = MetaScanner.COMMENT_END;
                            continue stateloop;
                        case '>':
                            state = MetaScanner.DATA;
                            continue stateloop;
                        default:
                            state = MetaScanner.COMMENT;
                            continue stateloop;
                    }
                    
                case ATTRIBUTE_VALUE_SINGLE_QUOTED:
                    for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            c = read();
                        }
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '\'':
                                if (tryCharset()) {
                                    break stateloop;
                                }
                                state = MetaScanner.AFTER_ATTRIBUTE_VALUE_QUOTED;
                                continue stateloop;
                            default:
                                if (metaState == A && (contentIndex == 6 || charsetIndex == 6)) {
                                    addToBuffer(c);
                                }
                                continue;
                        }
                    }
                    
                case SCAN_UNTIL_GT:
                    for (;;) {
                        if (reconsume) {
                            reconsume = false;
                        } else {
                            c = read();
                        }
                        switch (c) {
                            case -1:
                                break stateloop;
                            case '>':
                                state = MetaScanner.DATA;
                                continue stateloop;
                            default:
                                continue;
                        }
                    }
            }
        }
        stateSave  = state;
    }

    private void addToBuffer(int c) {
        if (strBufLen == strBuf.length) {
            char[] newBuf = new char[strBuf.length + (strBuf.length << 1)];
            System.arraycopy(strBuf, 0, newBuf, 0, strBuf.length);
            Portability.releaseArray(strBuf);
            strBuf = newBuf;
        }
        strBuf[strBufLen++] = (char)c;
    }

    private boolean tryCharset() throws SAXException {
        if (metaState != A || !(contentIndex == 6 || charsetIndex == 6)) {
            return false;
        }
        String attVal = Portability.newStringFromBuffer(strBuf, 0, strBufLen);
        String candidateEncoding;
        if (contentIndex == 6) {
            candidateEncoding = TreeBuilder.extractCharsetFromContent(attVal);
            Portability.releaseString(attVal);
        } else {
            candidateEncoding = attVal;
        }
        if (candidateEncoding == null) {
            return false;
        }
        boolean success = tryCharset(candidateEncoding);
        Portability.releaseString(candidateEncoding);
        contentIndex = -1;
        charsetIndex = -1;
        return success;
    }
    
    protected abstract boolean tryCharset(String encoding) throws SAXException;

    
}
