







































#ifndef jsonparser_h___
#define jsonparser_h___

#include "mozilla/Attributes.h"
#include "mozilla/RangedPtr.h"

#include "jscntxt.h"
#include "jsstr.h"




class JSONParser
{
    JSONParser(const JSONParser &other) MOZ_DELETE;
    void operator=(const JSONParser &other) MOZ_DELETE;

  public:
    enum ErrorHandling { RaiseError, NoError };
    enum ParsingMode { StrictJSON, LegacyJSON };

  private:
    

    JSContext * const cx;
    mozilla::RangedPtr<const jschar> current;
    const mozilla::RangedPtr<const jschar> end;

    
    js::SkipRoot root;

    js::Value v;

    const ParsingMode parsingMode;
    const ErrorHandling errorHandling;

    enum Token { String, Number, True, False, Null,
                 ArrayOpen, ArrayClose,
                 ObjectOpen, ObjectClose,
                 Colon, Comma,
                 OOM, Error };
#ifdef DEBUG
    Token lastToken;
#endif

  public:
    

    





    JSONParser(JSContext *cx, const jschar *data, size_t length,
               ParsingMode parsingMode = StrictJSON,
               ErrorHandling errorHandling = RaiseError)
      : cx(cx),
        current(data, length),
        end(data + length, data, length),
        root(cx, this),
        parsingMode(parsingMode),
        errorHandling(errorHandling)
#ifdef DEBUG
      , lastToken(Error)
#endif
    {
        JS_ASSERT(current <= end);
    }

    









    bool parse(js::Value *vp);

  private:
    js::Value numberValue() const {
        JS_ASSERT(lastToken == Number);
        JS_ASSERT(v.isNumber());
        return v;
    }

    js::Value stringValue() const {
        JS_ASSERT(lastToken == String);
        JS_ASSERT(v.isString());
        return v;
    }

    js::Value atomValue() const {
        js::Value strval = stringValue();
        JS_ASSERT(strval.toString()->isAtom());
        return strval;
    }

    Token token(Token t) {
        JS_ASSERT(t != String);
        JS_ASSERT(t != Number);
#ifdef DEBUG
        lastToken = t;
#endif
        return t;
    }

    Token stringToken(JSString *str) {
        this->v = js::StringValue(str);
#ifdef DEBUG
        lastToken = String;
#endif
        return String;
    }

    Token numberToken(double d) {
        this->v = js::NumberValue(d);
#ifdef DEBUG
        lastToken = Number;
#endif
        return Number;
    }

    enum StringType { PropertyName, LiteralValue };
    template<StringType ST> Token readString();

    Token readNumber();

    Token advance();
    Token advancePropertyName();
    Token advancePropertyColon();
    Token advanceAfterProperty();
    Token advanceAfterObjectOpen();
    Token advanceAfterArrayElement();

    void error(const char *msg);
    bool errorReturn();
};

#endif 
