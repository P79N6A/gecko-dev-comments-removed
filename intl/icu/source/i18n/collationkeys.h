










#ifndef __COLLATIONKEYS_H__
#define __COLLATIONKEYS_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/bytestream.h"
#include "unicode/ucol.h"
#include "charstr.h"
#include "collation.h"

U_NAMESPACE_BEGIN

class CollationIterator;
struct CollationDataReader;
struct CollationSettings;

class SortKeyByteSink : public ByteSink {
public:
    SortKeyByteSink(char *dest, int32_t destCapacity)
            : buffer_(dest), capacity_(destCapacity),
              appended_(0), ignore_(0) {}
    virtual ~SortKeyByteSink();

    void IgnoreBytes(int32_t numIgnore) { ignore_ = numIgnore; }

    virtual void Append(const char *bytes, int32_t n);
    void Append(uint32_t b) {
        if (ignore_ > 0) {
            --ignore_;
        } else {
            if (appended_ < capacity_ || Resize(1, appended_)) {
                buffer_[appended_] = (char)b;
            }
            ++appended_;
        }
    }
    virtual char *GetAppendBuffer(int32_t min_capacity,
                                  int32_t desired_capacity_hint,
                                  char *scratch, int32_t scratch_capacity,
                                  int32_t *result_capacity);
    int32_t NumberOfBytesAppended() const { return appended_; }

    



    int32_t GetRemainingCapacity() const {
        
        return ignore_ + capacity_ - appended_;
    }

    UBool Overflowed() const { return appended_ > capacity_; }
    
    UBool IsOk() const { return buffer_ != NULL; }

protected:
    virtual void AppendBeyondCapacity(const char *bytes, int32_t n, int32_t length) = 0;
    virtual UBool Resize(int32_t appendCapacity, int32_t length) = 0;

    void SetNotOk() {
        buffer_ = NULL;
        capacity_ = 0;
    }

    char *buffer_;
    int32_t capacity_;
    int32_t appended_;
    int32_t ignore_;

private:
    SortKeyByteSink(const SortKeyByteSink &); 
    SortKeyByteSink &operator=(const SortKeyByteSink &); 
};

class U_I18N_API CollationKeys  {
public:
    class LevelCallback : public UMemory {
    public:
        virtual ~LevelCallback();
        




        virtual UBool needToWrite(Collation::Level level);
    };

    






    static void writeSortKeyUpToQuaternary(CollationIterator &iter,
                                           const UBool *compressibleBytes,
                                           const CollationSettings &settings,
                                           SortKeyByteSink &sink,
                                           Collation::Level minLevel, LevelCallback &callback,
                                           UBool preflight, UErrorCode &errorCode);
private:
    friend struct CollationDataReader;

    CollationKeys();  

    
    static const uint32_t SEC_COMMON_LOW = Collation::COMMON_BYTE;
    static const uint32_t SEC_COMMON_MIDDLE = SEC_COMMON_LOW + 0x20;
    static const uint32_t SEC_COMMON_HIGH = SEC_COMMON_LOW + 0x40;
    static const int32_t SEC_COMMON_MAX_COUNT = 0x21;

    
    static const uint32_t CASE_LOWER_FIRST_COMMON_LOW = 1;
    static const uint32_t CASE_LOWER_FIRST_COMMON_MIDDLE = 7;
    static const uint32_t CASE_LOWER_FIRST_COMMON_HIGH = 13;
    static const int32_t CASE_LOWER_FIRST_COMMON_MAX_COUNT = 7;

    
    static const uint32_t CASE_UPPER_FIRST_COMMON_LOW = 3;
    static const uint32_t CASE_UPPER_FIRST_COMMON_HIGH = 15;
    static const int32_t CASE_UPPER_FIRST_COMMON_MAX_COUNT = 13;

    
    static const uint32_t TER_ONLY_COMMON_LOW = Collation::COMMON_BYTE;
    static const uint32_t TER_ONLY_COMMON_MIDDLE = TER_ONLY_COMMON_LOW + 0x60;
    static const uint32_t TER_ONLY_COMMON_HIGH = TER_ONLY_COMMON_LOW + 0xc0;
    static const int32_t TER_ONLY_COMMON_MAX_COUNT = 0x61;

    
    static const uint32_t TER_LOWER_FIRST_COMMON_LOW = Collation::COMMON_BYTE;
    static const uint32_t TER_LOWER_FIRST_COMMON_MIDDLE = TER_LOWER_FIRST_COMMON_LOW + 0x20;
    static const uint32_t TER_LOWER_FIRST_COMMON_HIGH = TER_LOWER_FIRST_COMMON_LOW + 0x40;
    static const int32_t TER_LOWER_FIRST_COMMON_MAX_COUNT = 0x21;

    
    static const uint32_t TER_UPPER_FIRST_COMMON_LOW = Collation::COMMON_BYTE + 0x80;
    static const uint32_t TER_UPPER_FIRST_COMMON_MIDDLE = TER_UPPER_FIRST_COMMON_LOW + 0x20;
    static const uint32_t TER_UPPER_FIRST_COMMON_HIGH = TER_UPPER_FIRST_COMMON_LOW + 0x40;
    static const int32_t TER_UPPER_FIRST_COMMON_MAX_COUNT = 0x21;

    
    static const uint32_t QUAT_COMMON_LOW = 0x1c;
    static const uint32_t QUAT_COMMON_MIDDLE = QUAT_COMMON_LOW + 0x70;
    static const uint32_t QUAT_COMMON_HIGH = QUAT_COMMON_LOW + 0xE0;
    static const int32_t QUAT_COMMON_MAX_COUNT = 0x71;
    
    
    static const uint32_t QUAT_SHIFTED_LIMIT_BYTE = QUAT_COMMON_LOW - 1;  
};

U_NAMESPACE_END

#endif  
#endif  
