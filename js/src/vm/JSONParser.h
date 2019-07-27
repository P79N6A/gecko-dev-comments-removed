





#ifndef vm_JSONParser_h
#define vm_JSONParser_h

#include "mozilla/Attributes.h"
#include "mozilla/Range.h"

#include "jspubtd.h"

#include "ds/IdValuePair.h"
#include "vm/String.h"

namespace js {




class MOZ_STACK_CLASS JSONParserBase : private JS::AutoGCRooter
{
  public:
    enum ErrorHandling { RaiseError, NoError };

  private:
    
    Value v;

  protected:
    JSContext * const cx;

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
            MOZ_ASSERT(state == FinishArrayElement);
            return * static_cast<ElementVector *>(vector);
        }

        PropertyVector &properties() {
            MOZ_ASSERT(state == FinishObjectMember);
            return * static_cast<PropertyVector *>(vector);
        }

        explicit StackEntry(ElementVector *elements)
          : state(FinishArrayElement), vector(elements)
        {}

        explicit StackEntry(PropertyVector *properties)
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

    JSONParserBase(JSContext *cx, ErrorHandling errorHandling)
      : JS::AutoGCRooter(cx, JSONPARSER),
        cx(cx),
        errorHandling(errorHandling),
        stack(cx),
        freeElements(cx),
        freeProperties(cx)
#ifdef DEBUG
      , lastToken(Error)
#endif
    {}
    ~JSONParserBase();

    Value numberValue() const {
        MOZ_ASSERT(lastToken == Number);
        MOZ_ASSERT(v.isNumber());
        return v;
    }

    Value stringValue() const {
        MOZ_ASSERT(lastToken == String);
        MOZ_ASSERT(v.isString());
        return v;
    }

    JSAtom *atomValue() const {
        Value strval = stringValue();
        return &strval.toString()->asAtom();
    }

    Token token(Token t) {
        MOZ_ASSERT(t != String);
        MOZ_ASSERT(t != Number);
#ifdef DEBUG
        lastToken = t;
#endif
        return t;
    }

    Token stringToken(JSString *str) {
        this->v = StringValue(str);
#ifdef DEBUG
        lastToken = String;
#endif
        return String;
    }

    Token numberToken(double d) {
        this->v = NumberValue(d);
#ifdef DEBUG
        lastToken = Number;
#endif
        return Number;
    }

    enum StringType { PropertyName, LiteralValue };

    bool errorReturn();

    bool finishObject(MutableHandleValue vp, PropertyVector &properties);
    bool finishArray(MutableHandleValue vp, ElementVector &elements);

  private:
    friend void AutoGCRooter::trace(JSTracer *trc);
    void trace(JSTracer *trc);

    JSObject *createFinishedObject(PropertyVector &properties);

    JSONParserBase(const JSONParserBase &other) MOZ_DELETE;
    void operator=(const JSONParserBase &other) MOZ_DELETE;
};

template <typename CharT>
class MOZ_STACK_CLASS JSONParser : public JSONParserBase
{
  private:
    typedef mozilla::RangedPtr<const CharT> CharPtr;

    CharPtr current;
    const CharPtr begin, end;

  public:
    

    
    JSONParser(JSContext *cx, mozilla::Range<const CharT> data,
               ErrorHandling errorHandling = RaiseError)
      : JSONParserBase(cx, errorHandling),
        current(data.start()),
        begin(current),
        end(data.end())
    {
        MOZ_ASSERT(current <= end);
    }

    









    bool parse(MutableHandleValue vp);

  private:
    template<StringType ST> Token readString();

    Token readNumber();

    Token advance();
    Token advancePropertyName();
    Token advancePropertyColon();
    Token advanceAfterProperty();
    Token advanceAfterObjectOpen();
    Token advanceAfterArrayElement();

    void error(const char *msg);

    void getTextPosition(uint32_t *column, uint32_t *line);

  private:
    JSONParser(const JSONParser &other) MOZ_DELETE;
    void operator=(const JSONParser &other) MOZ_DELETE;
};

} 

#endif 
