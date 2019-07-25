






#ifndef SkJSON_DEFINED
#define SkJSON_DEFINED

#include "SkTypes.h"

class SkStream;
class SkString;

class SkJSON {
public:
    enum Type {
        kObject,
        kArray,
        kString,
        kInt,
        kFloat,
        kBool,
    };
    
    class Array;
    
    class Object {
    private:
        struct Slot;

    public:
        Object();
        Object(const Object&);
        ~Object();

        





        void addObject(const char name[], Object* value);
        
        





        void addArray(const char name[], Array* value);
        
        




        void addString(const char name[], const char value[]);
        
        



        void addInt(const char name[], int32_t value);
        
        



        void addFloat(const char name[], float value);
        
        



        void addBool(const char name[], bool value);

        



        int count() const;

        


        bool find(const char name[], Type) const;
        bool findObject(const char name[], Object** = NULL) const;
        bool findArray(const char name[], Array** = NULL) const;
        bool findString(const char name[], SkString* = NULL) const;
        bool findInt(const char name[], int32_t* = NULL) const;
        bool findFloat(const char name[], float* = NULL) const;
        bool findBool(const char name[], bool* = NULL) const;

        



        bool remove(const char name[], Type);

        void toDebugf() const;

        



        class Iter {
        public:
            Iter(const Object&);
            
            



            bool done() const;

            



            void next();

            



            Type type() const;
            
            



            const char* name() const;
            
            



            Object* objectValue() const;
            
            



            Array* arrayValue() const;
            
            



            const char* stringValue() const;
            
            



            int32_t intValue() const;
            
            



            float floatValue() const;
            
            



            bool boolValue() const;

        private:
            Slot* fSlot;
        };

    private:
        Slot* fHead;
        Slot* fTail;
        
        const Slot* findSlot(const char name[], Type) const;
        Slot* addSlot(Slot*);
        void dumpLevel(int level) const;
        
        friend class Array;
    };
    
    class Array {
    public:
        



        Array(Type, int count);

        



        Array(const int32_t values[], int count);
        
        



        Array(const float values[], int count);
        
        



        Array(const bool values[], int count);
        
        Array(const Array&);
        ~Array();
        
        int count() const { return fCount; }
        Type type() const { return fType; }

        




        void setObject(int index, Object*);
        
        




        void setArray(int index, Array*);

        




        void setString(int index, const char str[]);

        Object* const* objects() const {
            SkASSERT(kObject == fType);
            return fArray.fObjects;
        }
        Array* const* arrays() const {
            SkASSERT(kObject == fType);
            return fArray.fArrays;
        }
        const char* const* strings() const {
            SkASSERT(kString == fType);
            return fArray.fStrings;
        }
        int32_t* ints() const {
            SkASSERT(kInt == fType);
            return fArray.fInts;
        }
        float* floats() const {
            SkASSERT(kFloat == fType);
            return fArray.fFloats;
        }
        bool* bools() const {
            SkASSERT(kBool == fType);
            return fArray.fBools;
        }

    private:
        int fCount;
        Type fType;
        union {
            void*    fVoids;
            Object** fObjects;
            Array**  fArrays;
            char**   fStrings;
            int32_t* fInts;
            float*   fFloats;
            bool*    fBools;
        } fArray;
        
        void init(Type, int count, const void* src);
        void dumpLevel(int level) const;
        
        friend class Object;
    };
};

#endif
