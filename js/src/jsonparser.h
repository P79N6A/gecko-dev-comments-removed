






#ifndef jsonparser_h___
#define jsonparser_h___

#include "mozilla/Attributes.h"
#include "mozilla/RangedPtr.h"

#include "jscntxt.h"
#include "jsstr.h"

namespace js {




class JSONParser : private AutoGCRooter
{
  public:
    enum ErrorHandling { RaiseError, NoError };
    enum ParsingMode { StrictJSON, LegacyJSON };

  private:
    

    JSContext * const cx;
    StableCharPtr current;
    const StableCharPtr end;

    Value v;

    const ParsingMode parsingMode;
    const ErrorHandling errorHandling;

    enum Token { String, Number, True, False, Null,
                 ArrayOpen, ArrayClose,
                 ObjectOpen, ObjectClose,
                 Colon, Comma,
                 OOM, Error };

    
    
    
    
    
    

    
    
    typedef Vector<Value, 20> ElementVector;

    
    
    typedef Vector<IdValuePair, 10> PropertyVector;

    
    enum ParserState {
        
        FinishArrayElement,

        
        FinishObjectMember,

        
        JSONValue
    };

    
    struct StackEntry {
        ElementVector &elements() {
            JS_ASSERT(state == FinishArrayElement);
            return * static_cast<ElementVector *>(vector);
        }

        PropertyVector &properties() {
            JS_ASSERT(state == FinishObjectMember);
            return * static_cast<PropertyVector *>(vector);
        }

        StackEntry(ElementVector *elements)
          : state(FinishArrayElement), vector(elements)
        {}

        StackEntry(PropertyVector *properties)
          : state(FinishObjectMember), vector(properties)
        {}

        ParserState state;

      private:
        void *vector;
    };

    
    
    Vector<StackEntry, 10> stack;

    
    
    
    Vector<ElementVector*, 5> freeElements;
    Vector<PropertyVector*, 5> freeProperties;

#ifdef DEBUG
    Token lastToken;
#endif

    JSONParser *thisDuringConstruction() { return this; }

  public:
    

    





    JSONParser(JSContext *cx, JS::StableCharPtr data, size_t length,
               ParsingMode parsingMode = StrictJSON,
               ErrorHandling errorHandling = RaiseError)
      : AutoGCRooter(cx, JSONPARSER),
        cx(cx),
        current(data),
        end((data + length).get(), data.get(), length),
        parsingMode(parsingMode),
        errorHandling(errorHandling),
        stack(cx),
        freeElements(cx),
        freeProperties(cx)
#ifdef DEBUG
      , lastToken(Error)
#endif
    {
        JS_ASSERT(current <= end);
    }

    ~JSONParser();

    









    bool parse(js::MutableHandleValue vp);

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

    JSAtom *atomValue() const {
        js::Value strval = stringValue();
        return &strval.toString()->asAtom();
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

    JSObject *createFinishedObject(PropertyVector &properties);
    bool finishObject(MutableHandleValue vp, PropertyVector &properties);
    bool finishArray(MutableHandleValue vp, ElementVector &elements);

    friend void AutoGCRooter::trace(JSTracer *trc);
    void trace(JSTracer *trc);

  private:
    JSONParser(const JSONParser &other) MOZ_DELETE;
    void operator=(const JSONParser &other) MOZ_DELETE;
};

} 

#endif 
