





#ifndef vm_Printer_h
#define vm_Printer_h

#include <stdarg.h>
#include <stdio.h>

class JSString;

namespace js {

class ExclusiveContext;
class LifoAlloc;






class GenericPrinter
{
  protected:
    bool                    reportedOOM_;   

    GenericPrinter();

  public:
    
    
    virtual int put(const char* s, size_t len) = 0;
    virtual int put(const char* s);

    
    virtual int printf(const char* fmt, ...);
    virtual int vprintf(const char* fmt, va_list ap);

    
    
    
    virtual void reportOutOfMemory();

    
    virtual bool hadOutOfMemory() const;
};


class Sprinter final : public GenericPrinter
{
  public:
    struct InvariantChecker
    {
        const Sprinter* parent;

        explicit InvariantChecker(const Sprinter* p) : parent(p) {
            parent->checkInvariants();
        }

        ~InvariantChecker() {
            parent->checkInvariants();
        }
    };

    ExclusiveContext*       context;        

  private:
    static const size_t     DefaultSize;
#ifdef DEBUG
    bool                    initialized;    
#endif
    char*                   base;           
    size_t                  size;           
    ptrdiff_t               offset;         

    bool realloc_(size_t newSize);

  public:
    explicit Sprinter(ExclusiveContext* cx);
    ~Sprinter();

    
    bool init();

    void checkInvariants() const;

    const char* string() const;
    const char* stringEnd() const;
    
    char* stringAt(ptrdiff_t off) const;
    
    char& operator[](size_t off);

    
    
    
    char* reserve(size_t len);

    
    
    using GenericPrinter::put;
    virtual int put(const char* s, size_t len) override;

    
    virtual int vprintf(const char* fmt, va_list ap) override;

    int putString(JSString* str);

    ptrdiff_t getOffset() const;

    
    
    
    virtual void reportOutOfMemory() override;
};


class Fprinter final : public GenericPrinter
{
  private:
    FILE*                   file_;
    bool                    init_;

  public:
    explicit Fprinter(FILE* fp);
    Fprinter();
    ~Fprinter();

    
    bool init(const char* path);
    void init(FILE* fp);
    bool isInitialized() {
        return file_ != nullptr;
    }
    void flush();
    void finish();

    
    
    virtual int put(const char* s, size_t len) override;
    virtual int put(const char* s) override;

    
    virtual int printf(const char* fmt, ...) override;
    virtual int vprintf(const char* fmt, va_list ap) override;
};




class LSprinter final : public GenericPrinter
{
  private:
    struct Chunk
    {
        Chunk* next;
        size_t length;

        char* chars() {
            return reinterpret_cast<char*>(this + 1);
        }
        char* end() {
            return chars() + length;
        }
    };

  private:
    LifoAlloc*              alloc_;          
    Chunk*                  head_;
    Chunk*                  tail_;
    size_t                  unused_;

  public:
    explicit LSprinter(LifoAlloc* lifoAlloc);
    ~LSprinter();

    
    
    void exportInto(GenericPrinter& out) const;

    
    void clear();

    
    
    virtual int put(const char* s, size_t len) override;
    virtual int put(const char* s) override;

    
    virtual int printf(const char* fmt, ...) override;
    virtual int vprintf(const char* fmt, va_list ap) override;

    
    
    
    virtual void reportOutOfMemory() override;

    
    virtual bool hadOutOfMemory() const override;
};

extern ptrdiff_t
Sprint(Sprinter* sp, const char* format, ...);


extern const char       js_EscapeMap[];




extern JSString*
QuoteString(ExclusiveContext* cx, JSString* str, char16_t quote);

extern char*
QuoteString(Sprinter* sp, JSString* str, char16_t quote);


} 

#endif 
