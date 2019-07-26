






#ifndef SkTFitsIn_DEFINED
#define SkTFitsIn_DEFINED

#include "SkTypes.h"
#include "SkTLogic.h"
#include <limits>

namespace sktfitsin {
namespace Private {


template<typename A, typename B> struct SkTHasMoreDigits {
    typedef SkTBool<std::numeric_limits<A>::digits >= std::numeric_limits<B>::digits> type;
};




template <typename S> struct SkTOutOfRange_False {
    typedef SkFalse can_be_true;
    typedef S source_type;
    static bool apply(S s) {
        return false;
    }
};




template <typename D, typename S> struct SkTOutOfRange_LT_MinD {
    typedef SkTrue can_be_true;
    typedef S source_type;
    static bool apply(S s) {
        typedef typename SkTHasMoreDigits<S, D>::type precondition;
        SK_COMPILE_ASSERT(precondition::value, SkTOutOfRange_LT_MinD__minS_gt_minD);

        return s < static_cast<S>((std::numeric_limits<D>::min)());
    }
};


template <typename D, typename S> struct SkTOutOfRange_LT_Zero {
    typedef SkTrue can_be_true;
    typedef S source_type;
    static bool apply(S s) {
        return s < static_cast<S>(0);
    }
};




template <typename D, typename S> struct SkTOutOfRange_GT_MaxD {
    typedef SkTrue can_be_true;
    typedef S source_type;
    static bool apply(S s) {
        typedef typename SkTHasMoreDigits<S, D>::type precondition;
        SK_COMPILE_ASSERT(precondition::value, SkTOutOfRange_GT_MaxD__maxS_lt_maxD);

        return s > static_cast<S>((std::numeric_limits<D>::max)());
    }
};




template<class OutOfRange_Low, class OutOfRange_High> struct SkTOutOfRange_Either {
    typedef SkTrue can_be_true;
    typedef typename OutOfRange_Low::source_type source_type;
    static bool apply(source_type s) {
        bool outOfRange = OutOfRange_Low::apply(s);
        if (!outOfRange) {
            outOfRange = OutOfRange_High::apply(s);
        }
        return outOfRange;
    }
};




template<class OutOfRange_Low, class OutOfRange_High> struct SkTCombineOutOfRange {
    typedef SkTOutOfRange_Either<OutOfRange_Low, OutOfRange_High> Both;
    typedef SkTOutOfRange_False<typename OutOfRange_Low::source_type> Neither;

    typedef typename OutOfRange_Low::can_be_true apply_low;
    typedef typename OutOfRange_High::can_be_true apply_high;

    typedef typename SkTMux<apply_low, apply_high,
                            Both, OutOfRange_Low, OutOfRange_High, Neither>::type type;
};

template<typename D, typename S, class OutOfRange_Low, class OutOfRange_High>
struct SkTRangeChecker {
    
    static bool OutOfRange(S s) {
        typedef typename SkTCombineOutOfRange<OutOfRange_Low, OutOfRange_High>::type Combined;
        return Combined::apply(s);
    }
};





template<typename D, typename S> struct SkTFitsIn_Unsigned2Unsiged {
    typedef SkTOutOfRange_False<S> OutOfRange_Low;
    typedef SkTOutOfRange_GT_MaxD<D, S> OutOfRange_High;

    typedef SkTRangeChecker<D, S, OutOfRange_Low, OutOfRange_High> HighSideOnlyCheck;
    typedef SkTRangeChecker<D, S, SkTOutOfRange_False<S>, SkTOutOfRange_False<S> > NoCheck;

    
    
    typedef typename SkTHasMoreDigits<D, S>::type sourceFitsInDesitination;
    typedef typename SkTIf<sourceFitsInDesitination, NoCheck, HighSideOnlyCheck>::type type;
};





template<typename D, typename S> struct SkTFitsIn_Signed2Signed {
    typedef SkTOutOfRange_LT_MinD<D, S> OutOfRange_Low;
    typedef SkTOutOfRange_GT_MaxD<D, S> OutOfRange_High;

    typedef SkTRangeChecker<D, S, OutOfRange_Low, OutOfRange_High> FullCheck;
    typedef SkTRangeChecker<D, S, SkTOutOfRange_False<S>, SkTOutOfRange_False<S> > NoCheck;

    
    
    typedef typename SkTHasMoreDigits<D, S>::type sourceFitsInDesitination;
    typedef typename SkTIf<sourceFitsInDesitination, NoCheck, FullCheck>::type type;
};





template<typename D, typename S> struct SkTFitsIn_Signed2Unsigned {
    typedef SkTOutOfRange_LT_Zero<D, S> OutOfRange_Low;
    typedef SkTOutOfRange_GT_MaxD<D, S> OutOfRange_High;

    typedef SkTRangeChecker<D, S, OutOfRange_Low, OutOfRange_High> FullCheck;
    typedef SkTRangeChecker<D, S, OutOfRange_Low, SkTOutOfRange_False<S> > LowSideOnlyCheck;

    
    
    
    typedef typename SkTHasMoreDigits<D, S>::type sourceCannotExceedDesitination;
    typedef typename SkTIf<sourceCannotExceedDesitination, LowSideOnlyCheck, FullCheck>::type type;
};





template<typename D, typename S> struct SkTFitsIn_Unsigned2Signed {
    typedef SkTOutOfRange_False<S> OutOfRange_Low;
    typedef SkTOutOfRange_GT_MaxD<D, S> OutOfRange_High;

    typedef SkTRangeChecker<D, S, OutOfRange_Low, OutOfRange_High> HighSideOnlyCheck;
    typedef SkTRangeChecker<D, S, SkTOutOfRange_False<S>, SkTOutOfRange_False<S> > NoCheck;

    
    
    
    typedef typename SkTHasMoreDigits<D, S>::type sourceCannotExceedDesitination;
    typedef typename SkTIf<sourceCannotExceedDesitination, NoCheck, HighSideOnlyCheck>::type type;
};





template<typename D, typename S> struct SkTFitsIn {
    
    typedef SkTFitsIn_Signed2Signed<D, S> S2S;
    typedef SkTFitsIn_Signed2Unsigned<D, S> S2U;
    typedef SkTFitsIn_Unsigned2Signed<D, S> U2S;
    typedef SkTFitsIn_Unsigned2Unsiged<D, S> U2U;

    typedef SkTBool<std::numeric_limits<S>::is_signed> S_is_signed;
    typedef SkTBool<std::numeric_limits<D>::is_signed> D_is_signed;

    typedef typename SkTMux<S_is_signed, D_is_signed, S2S, S2U, U2S, U2U>::type selector;
    
    typedef typename selector::type type;
};

} 
} 


template <typename D, typename S> inline bool SkTFitsIn(S s) {
    SK_COMPILE_ASSERT(std::numeric_limits<S>::is_integer, SkTFitsIn_source_must_be_integer);
    SK_COMPILE_ASSERT(std::numeric_limits<D>::is_integer, SkTFitsIn_destination_must_be_integer);

    return !sktfitsin::Private::SkTFitsIn<D, S>::type::OutOfRange(s);
}

#endif
