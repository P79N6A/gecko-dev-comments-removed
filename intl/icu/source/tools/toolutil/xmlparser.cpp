















#include <stdio.h>
#include "unicode/uchar.h"
#include "unicode/ucnv.h"
#include "unicode/regex.h"
#include "filestrm.h"
#include "xmlparser.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS && !UCONFIG_NO_CONVERSION


enum {
    x_QUOT=0x22,
    x_AMP=0x26,
    x_APOS=0x27,
    x_LT=0x3c,
    x_GT=0x3e,
    x_l=0x6c
};

#define  XML_SPACES "[ \\u0009\\u000d\\u000a]"


#define  XML_NAMESTARTCHAR "[[A-Z]:_[a-z][\\u00c0-\\u00d6][\\u00d8-\\u00f6]" \
                    "[\\u00f8-\\u02ff][\\u0370-\\u037d][\\u037F-\\u1FFF][\\u200C-\\u200D]" \
                    "[\\u2070-\\u218F][\\u2C00-\\u2FEF][\\u3001-\\uD7FF][\\uF900-\\uFDCF]" \
                    "[\\uFDF0-\\uFFFD][\\U00010000-\\U000EFFFF]]"


#define  XML_NAMECHAR "[" XML_NAMESTARTCHAR "\\-.[0-9]\\u00b7[\\u0300-\\u036f][\\u203f-\\u2040]]"


#define  XML_NAME    XML_NAMESTARTCHAR "(?:" XML_NAMECHAR ")*"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UXMLParser)
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UXMLElement)





UXMLParser::UXMLParser(UErrorCode &status) :
      
      
      
      
      mXMLDecl(UnicodeString("(?s)\\uFEFF?<\\?xml.+?\\?>", -1, US_INV), 0, status),
      
      
      
      
      mXMLComment(UnicodeString("(?s)<!--.+?-->", -1, US_INV), 0, status),
      
      
      
      mXMLSP(UnicodeString(XML_SPACES "+", -1, US_INV), 0, status),
      
      
      
      
      
      
      
      
      
      mXMLDoctype(UnicodeString(
           "(?s)<!DOCTYPE.*?(>|\\[.*?\\].*?>)", -1, US_INV
           ), 0, status),
      
      
      
      mXMLPI(UnicodeString("(?s)<\\?.+?\\?>", -1, US_INV), 0, status),
      
      
      
      
      
      mXMLElemStart (UnicodeString("(?s)<(" XML_NAME ")"                                 
          "(?:" 
                XML_SPACES "+" XML_NAME XML_SPACES "*=" XML_SPACES "*"     
                "(?:(?:\\\'[^<\\\']*?\\\')|(?:\\\"[^<\\\"]*?\\\"))"        
          ")*"                                                             
          XML_SPACES "*?>", -1, US_INV), 0, status),                               
      
      
      
      mXMLElemEnd (UnicodeString("</(" XML_NAME ")" XML_SPACES "*>", -1, US_INV), 0, status),
      
      
      
      mXMLElemEmpty (UnicodeString("(?s)<(" XML_NAME ")"                                 
          "(?:" 
                XML_SPACES "+" XML_NAME XML_SPACES "*=" XML_SPACES "*"     
                "(?:(?:\\\'[^<\\\']*?\\\')|(?:\\\"[^<\\\"]*?\\\"))"        
          ")*"                                                             
          XML_SPACES "*?/>", -1, US_INV), 0, status),                              
      

      
      mXMLCharData(UnicodeString("(?s)[^<]*", -1, US_INV), 0, status),

      
      
      
      
      
      
      
      
      
      mAttrValue(UnicodeString(XML_SPACES "+("  XML_NAME ")"  XML_SPACES "*=" XML_SPACES "*"
         "((?:\\\'[^<\\\']*?\\\')|(?:\\\"[^<\\\"]*?\\\"))", -1, US_INV), 0, status),


      mAttrNormalizer(UnicodeString(XML_SPACES, -1, US_INV), 0, status),

      
      
      mNewLineNormalizer(UnicodeString("\\u000d\\u000a|\\u000d\\u0085|\\u000a|\\u000d|\\u0085|\\u2028", -1, US_INV), 0, status),

      
      
      
      
      mAmps(UnicodeString("&(?:(amp;)|(lt;)|(gt;)|(apos;)|(quot;)|#x([0-9A-Fa-f]{1,8});|#([0-9]{1,8});|(.))"),
                0, status),

      fNames(status),
      fElementStack(status),
      fOneLF((UChar)0x0a)        
      {
      }

UXMLParser *
UXMLParser::createParser(UErrorCode &errorCode) {
    if (U_FAILURE(errorCode)) {
        return NULL;
    } else {
        return new UXMLParser(errorCode);
    }
}

UXMLParser::~UXMLParser() {}

UXMLElement *
UXMLParser::parseFile(const char *filename, UErrorCode &errorCode) {
    char bytes[4096], charsetBuffer[100];
    FileStream *f;
    const char *charset, *pb;
    UnicodeString src;
    UConverter *cnv;
    UChar *buffer, *pu;
    int32_t fileLength, bytesLength, length, capacity;
    UBool flush;

    if(U_FAILURE(errorCode)) {
        return NULL;
    }

    f=T_FileStream_open(filename, "rb");
    if(f==NULL) {
        errorCode=U_FILE_ACCESS_ERROR;
        return NULL;
    }

    bytesLength=T_FileStream_read(f, bytes, (int32_t)sizeof(bytes));
    if(bytesLength<(int32_t)sizeof(bytes)) {
        
        fileLength=bytesLength;
    } else {
        
        fileLength=T_FileStream_size(f);
    }

    





    charset=ucnv_detectUnicodeSignature(bytes, bytesLength, NULL, &errorCode);
    if(U_SUCCESS(errorCode) && charset!=NULL) {
        
        cnv=ucnv_open(charset, &errorCode);
    } else {
        
        cnv=ucnv_open("ISO-8859-1", &errorCode);
        if(U_FAILURE(errorCode)) {
            
            goto exit;
        }

        buffer=src.getBuffer(bytesLength);
        if(buffer==NULL) {
            
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            goto exit;
        }
        pb=bytes;
        pu=buffer;
        ucnv_toUnicode(
            cnv,
            &pu, buffer+src.getCapacity(),
            &pb, bytes+bytesLength,
            NULL, TRUE, &errorCode);
        src.releaseBuffer(U_SUCCESS(errorCode) ? (int32_t)(pu-buffer) : 0);
        ucnv_close(cnv);
        cnv=NULL;
        if(U_FAILURE(errorCode)) {
            
            src.remove();
            goto exit;
        }

        
        if(mXMLDecl.reset(src).lookingAt(0, errorCode)) {
            int32_t declEnd=mXMLDecl.end(errorCode);
            
            int32_t pos=src.indexOf((UChar)x_l)+1;

            mAttrValue.reset(src);
            while(pos<declEnd && mAttrValue.lookingAt(pos, errorCode)) {  
                UnicodeString attName  = mAttrValue.group(1, errorCode);
                UnicodeString attValue = mAttrValue.group(2, errorCode);

                
                
                attValue.remove(0,1);                    
                attValue.truncate(attValue.length()-1);  

                if(attName==UNICODE_STRING("encoding", 8)) {
                    length=attValue.extract(0, 0x7fffffff, charsetBuffer, (int32_t)sizeof(charsetBuffer));
                    charset=charsetBuffer;
                    break;
                }
                pos = mAttrValue.end(2, errorCode);
            }

            if(charset==NULL) {
                
                charset="UTF-8";
            }
            cnv=ucnv_open(charset, &errorCode);
        }
    }

    if(U_FAILURE(errorCode)) {
        
        goto exit;
    }

    
    capacity=fileLength;        
    src.getBuffer(capacity);
    src.releaseBuffer(0);       
    flush=FALSE;
    for(;;) {
        
        pb=bytes;
        for(;;) {
            length=src.length();
            buffer=src.getBuffer(capacity);
            if(buffer==NULL) {
                
                errorCode=U_MEMORY_ALLOCATION_ERROR;
                goto exit;
            }

            pu=buffer+length;
            ucnv_toUnicode(
                cnv, &pu, buffer+src.getCapacity(),
                &pb, bytes+bytesLength,
                NULL, FALSE, &errorCode);
            src.releaseBuffer(U_SUCCESS(errorCode) ? (int32_t)(pu-buffer) : 0);
            if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
                errorCode=U_ZERO_ERROR;
                capacity=(3*src.getCapacity())/2; 
            } else {
                break;
            }
        }

        if(U_FAILURE(errorCode)) {
            break; 
        }

        if(flush) {
            break; 
        }

        
        bytesLength=T_FileStream_read(f, bytes, (int32_t)sizeof(bytes));
        if(bytesLength==0) {
            
            flush=TRUE;
        }
    };

exit:
    ucnv_close(cnv);
    T_FileStream_close(f);

    if(U_SUCCESS(errorCode)) {
        return parse(src, errorCode);
    } else {
        return NULL;
    }
}

UXMLElement *
UXMLParser::parse(const UnicodeString &src, UErrorCode &status) {
    if(U_FAILURE(status)) {
        return NULL;
    }

    UXMLElement   *root = NULL;
    fPos = 0; 
              

    
    mXMLDecl.reset(src);
    mXMLComment.reset(src);
    mXMLSP.reset(src);
    mXMLDoctype.reset(src);
    mXMLPI.reset(src);
    mXMLElemStart.reset(src);
    mXMLElemEnd.reset(src);
    mXMLElemEmpty.reset(src);
    mXMLCharData.reset(src);
    mAttrValue.reset(src);
    mAttrNormalizer.reset(src);
    mNewLineNormalizer.reset(src);
    mAmps.reset(src);

    
    if (mXMLDecl.lookingAt(fPos, status)) {
        fPos = mXMLDecl.end(status);
    }

    
    parseMisc(status);

    
    if (mXMLDoctype.lookingAt(fPos, status)) {
        fPos = mXMLDoctype.end(status);
    }

    
    parseMisc(status);

    
    if (mXMLElemEmpty.lookingAt(fPos, status)) {
        
        root = createElement(mXMLElemEmpty, status);
        fPos = mXMLElemEmpty.end(status);
    } else {
        if (mXMLElemStart.lookingAt(fPos, status) == FALSE) {
            error("Root Element expected", status);
            goto errorExit;
        }
        root = createElement(mXMLElemStart, status);
        UXMLElement  *el = root;

        
        
        
        
        
        
        
        for (;;) {
            
            if (mXMLElemStart.lookingAt(fPos, status)) {
                UXMLElement *t = createElement(mXMLElemStart, status);
                el->fChildren.addElement(t, status);
                t->fParent = el;
                fElementStack.push(el, status);
                el = t;
                continue;
            }

            
            
            UnicodeString s = scanContent(status);
            if (s.length() > 0) {
                mXMLSP.reset(s);
                if (mXMLSP.matches(status) == FALSE) {
                    
                    
                    replaceCharRefs(s, status);
                    el->fChildren.addElement(s.clone(), status);
                }
                mXMLSP.reset(src);    
                continue;
            }

            
            if (mXMLComment.lookingAt(fPos, status)) {
                fPos = mXMLComment.end(status);
                continue;
            }

            
            if (mXMLPI.lookingAt(fPos, status)) {
                fPos = mXMLPI.end(status);
                continue;
            }

            
            if (mXMLElemEnd.lookingAt(fPos, status)) {
                fPos = mXMLElemEnd.end(0, status);
                const UnicodeString name = mXMLElemEnd.group(1, status);
                if (name != *el->fName) {
                    error("Element start / end tag mismatch", status);
                    goto errorExit;
                }
                if (fElementStack.empty()) {
                    
                    el = NULL;
                    break;
                }
                el = (UXMLElement *)fElementStack.pop();
                continue;
            }

            
            if (mXMLElemEmpty.lookingAt(fPos, status)) {
                UXMLElement *t = createElement(mXMLElemEmpty, status);
                el->fChildren.addElement(t, status);
                continue;
            }

            
            
            error("Unrecognized markup", status);
            break;
        }

        if (el != NULL || !fElementStack.empty()) {
            
            error("Root element not closed.", status);
            goto errorExit;
        }
    }

    
    
    parseMisc(status);

    
    if (fPos != src.length()) {
        error("Extra content at the end of the document", status);
        goto errorExit;
    }

    
    return root;

errorExit:
    delete root;
    return NULL;
}






UXMLElement *
UXMLParser::createElement(RegexMatcher  &mEl, UErrorCode &status) {
    
    UXMLElement *el = new UXMLElement(this, intern(mEl.group(1, status), status), status);

    
    int32_t   pos = mEl.end(1, status);  

    while (mAttrValue.lookingAt(pos, status)) {  
        UnicodeString attName  = mAttrValue.group(1, status);
        UnicodeString attValue = mAttrValue.group(2, status);

        
        
        attValue.remove(0,1);                    
        attValue.truncate(attValue.length()-1);  
        
        
        
        
        
        

        
        mNewLineNormalizer.reset(attValue);
        attValue = mNewLineNormalizer.replaceAll(fOneLF, status);

        
        mAttrNormalizer.reset(attValue);
        UnicodeString oneSpace((UChar)0x0020);
        attValue = mAttrNormalizer.replaceAll(oneSpace, status);

        
        replaceCharRefs(attValue, status);

        
        el->fAttNames.addElement((void *)intern(attName, status), status);
        el->fAttValues.addElement(attValue.clone(), status);
        pos = mAttrValue.end(2, status);
    }
    fPos = mEl.end(0, status);
    return el;
}








void
UXMLParser::parseMisc(UErrorCode &status)  {
    for (;;) {
        if (fPos >= mXMLPI.input().length()) {
            break;
        }
        if (mXMLPI.lookingAt(fPos, status)) {
            fPos = mXMLPI.end(status);
            continue;
        }
        if (mXMLSP.lookingAt(fPos, status)) {
            fPos = mXMLSP.end(status);
            continue;
        }
        if (mXMLComment.lookingAt(fPos, status)) {
            fPos = mXMLComment.end(status);
            continue;
        }
        break;
    }
}




UnicodeString
UXMLParser::scanContent(UErrorCode &status) {
    UnicodeString  result;
    if (mXMLCharData.lookingAt(fPos, status)) {
        result = mXMLCharData.group((int32_t)0, status);
        
        mNewLineNormalizer.reset(result);
        result = mNewLineNormalizer.replaceAll(fOneLF, status);
        
        
        fPos = mXMLCharData.end(0, status);
    }

    return result;
}







void
UXMLParser::replaceCharRefs(UnicodeString &s, UErrorCode &status) {
    UnicodeString result;
    UnicodeString replacement;
    int     i;

    mAmps.reset(s);
    
    
    
    while (mAmps.find()) {
        if (mAmps.start(1, status) != -1) {
            replacement.setTo((UChar)x_AMP);
        } else if (mAmps.start(2, status) != -1) {
            replacement.setTo((UChar)x_LT);
        } else if (mAmps.start(3, status) != -1) {
            replacement.setTo((UChar)x_GT);
        } else if (mAmps.start(4, status) != -1) {
            replacement.setTo((UChar)x_APOS);
        } else if (mAmps.start(5, status) != -1) {
            replacement.setTo((UChar)x_QUOT);
        } else if (mAmps.start(6, status) != -1) {
            UnicodeString hexString = mAmps.group(6, status);
            UChar32 val = 0;
            for (i=0; i<hexString.length(); i++) {
                val = (val << 4) + u_digit(hexString.charAt(i), 16);
            }
            
            replacement.setTo(val);
        } else if (mAmps.start(7, status) != -1) {
            UnicodeString decimalString = mAmps.group(7, status);
            UChar32 val = 0;
            for (i=0; i<decimalString.length(); i++) {
                val = val*10 + u_digit(decimalString.charAt(i), 10);
            }
            
            replacement.setTo(val);
        } else {
            
            
            
            replacement = mAmps.group((int32_t)0, status);
        }
        mAmps.appendReplacement(result, replacement, status);
    }
    mAmps.appendTail(result);
    s = result;
}

void
UXMLParser::error(const char *message, UErrorCode &status) {
    
    const UnicodeString &src=mXMLDecl.input();
    int  line = 0;
    int  ci = 0;
    while (ci < fPos && ci>=0) {
        ci = src.indexOf((UChar)0x0a, ci+1);
        line++;
    }
    fprintf(stderr, "Error: %s at line %d\n", message, line);
    if (U_SUCCESS(status)) {
        status = U_PARSE_ERROR;
    }
}



const UnicodeString *
UXMLParser::intern(const UnicodeString &s, UErrorCode &errorCode) {
    const UHashElement *he=fNames.find(s);
    if(he!=NULL) {
        
        return (const UnicodeString *)he->key.pointer;
    } else {
        
        fNames.puti(s, 0, errorCode);
        he=fNames.find(s);
        return (const UnicodeString *)he->key.pointer;
    }
}

const UnicodeString *
UXMLParser::findName(const UnicodeString &s) const {
    const UHashElement *he=fNames.find(s);
    if(he!=NULL) {
        
        return (const UnicodeString *)he->key.pointer;
    } else {
        
        return NULL;
    }
}



UXMLElement::UXMLElement(const UXMLParser *parser, const UnicodeString *name, UErrorCode &errorCode) :
   fParser(parser),
   fName(name),
   fAttNames(errorCode),
   fAttValues(errorCode),
   fChildren(errorCode),
   fParent(NULL)
{
}

UXMLElement::~UXMLElement() {
    int   i;
    
    for (i=fAttValues.size()-1; i>=0; i--) {
        delete (UObject *)fAttValues.elementAt(i);
    }
    for (i=fChildren.size()-1; i>=0; i--) {
        delete (UObject *)fChildren.elementAt(i);
    }
}

const UnicodeString &
UXMLElement::getTagName() const {
    return *fName;
}

UnicodeString
UXMLElement::getText(UBool recurse) const {
    UnicodeString text;
    appendText(text, recurse);
    return text;
}

void
UXMLElement::appendText(UnicodeString &text, UBool recurse) const {
    const UObject *node;
    int32_t i, count=fChildren.size();
    for(i=0; i<count; ++i) {
        node=(const UObject *)fChildren.elementAt(i);
        const UnicodeString *s=dynamic_cast<const UnicodeString *>(node);
        if(s!=NULL) {
            text.append(*s);
        } else if(recurse)  {
            ((const UXMLElement *)node)->appendText(text, recurse);
        }
    }
}

int32_t
UXMLElement::countAttributes() const {
    return fAttNames.size();
}

const UnicodeString *
UXMLElement::getAttribute(int32_t i, UnicodeString &name, UnicodeString &value) const {
    if(0<=i && i<fAttNames.size()) {
        name.setTo(*(const UnicodeString *)fAttNames.elementAt(i));
        value.setTo(*(const UnicodeString *)fAttValues.elementAt(i));
        return &value; 
    } else {
        return NULL;
    }
}

const UnicodeString *
UXMLElement::getAttribute(const UnicodeString &name) const {
    
    
    const UnicodeString *p=fParser->findName(name);
    if(p==NULL) {
        return NULL; 
    }

    int32_t i, count=fAttNames.size();
    for(i=0; i<count; ++i) {
        if(p==(const UnicodeString *)fAttNames.elementAt(i)) {
            return (const UnicodeString *)fAttValues.elementAt(i);
        }
    }
    return NULL;
}

int32_t
UXMLElement::countChildren() const {
    return fChildren.size();
}

const UObject *
UXMLElement::getChild(int32_t i, UXMLNodeType &type) const {
    if(0<=i && i<fChildren.size()) {
        const UObject *node=(const UObject *)fChildren.elementAt(i);
        if(dynamic_cast<const UXMLElement *>(node)!=NULL) {
            type=UXML_NODE_TYPE_ELEMENT;
        } else {
            type=UXML_NODE_TYPE_STRING;
        }
        return node;
    } else {
        return NULL;
    }
}

const UXMLElement *
UXMLElement::nextChildElement(int32_t &i) const {
    if(i<0) {
        return NULL;
    }

    const UObject *node;
    int32_t count=fChildren.size();
    while(i<count) {
        node=(const UObject *)fChildren.elementAt(i++);
        const UXMLElement *elem=dynamic_cast<const UXMLElement *>(node);
        if(elem!=NULL) {
            return elem;
        }
    }
    return NULL;
}

const UXMLElement *
UXMLElement::getChildElement(const UnicodeString &name) const {
    
    
    const UnicodeString *p=fParser->findName(name);
    if(p==NULL) {
        return NULL; 
    }

    const UObject *node;
    int32_t i, count=fChildren.size();
    for(i=0; i<count; ++i) {
        node=(const UObject *)fChildren.elementAt(i);
        const UXMLElement *elem=dynamic_cast<const UXMLElement *>(node);
        if(elem!=NULL) {
            if(p==elem->fName) {
                return elem;
            }
        }
    }
    return NULL;
}

U_NAMESPACE_END

#endif 

