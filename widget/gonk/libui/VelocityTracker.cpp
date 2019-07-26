















#define LOG_TAG "VelocityTracker"

#include "cutils_log.h"


#define DEBUG_VELOCITY 0


#define DEBUG_STRATEGY 0

#include <math.h>
#include <limits.h>

#include "VelocityTracker.h"
#include <utils/BitSet.h>
#include <utils/String8.h>
#include <utils/Timers.h>

#include <cutils/properties.h>

namespace android {


static const nsecs_t NANOS_PER_MS = 1000000;





static const nsecs_t ASSUME_POINTER_STOPPED_TIME = 40 * NANOS_PER_MS;


static float vectorDot(const float* a, const float* b, uint32_t m) {
    float r = 0;
    while (m--) {
        r += *(a++) * *(b++);
    }
    return r;
}

static float vectorNorm(const float* a, uint32_t m) {
    float r = 0;
    while (m--) {
        float t = *(a++);
        r += t * t;
    }
    return sqrtf(r);
}

#if DEBUG_STRATEGY || DEBUG_VELOCITY
static String8 vectorToString(const float* a, uint32_t m) {
    String8 str;
    str.append("[");
    while (m--) {
        str.appendFormat(" %f", *(a++));
        if (m) {
            str.append(",");
        }
    }
    str.append(" ]");
    return str;
}

static String8 matrixToString(const float* a, uint32_t m, uint32_t n, bool rowMajor) {
    String8 str;
    str.append("[");
    for (size_t i = 0; i < m; i++) {
        if (i) {
            str.append(",");
        }
        str.append(" [");
        for (size_t j = 0; j < n; j++) {
            if (j) {
                str.append(",");
            }
            str.appendFormat(" %f", a[rowMajor ? i * n + j : j * m + i]);
        }
        str.append(" ]");
    }
    str.append(" ]");
    return str;
}
#endif









const char* VelocityTracker::DEFAULT_STRATEGY = "lsq2";

VelocityTracker::VelocityTracker(const char* strategy) :
        mLastEventTime(0), mCurrentPointerIdBits(0), mActivePointerId(-1) {
    char value[PROPERTY_VALUE_MAX];

    
    if (!strategy) {
        int length = property_get("debug.velocitytracker.strategy", value, NULL);
        if (length > 0) {
            strategy = value;
        } else {
            strategy = DEFAULT_STRATEGY;
        }
    }

    
    if (!configureStrategy(strategy)) {
        ALOGD("Unrecognized velocity tracker strategy name '%s'.", strategy);
        if (!configureStrategy(DEFAULT_STRATEGY)) {
            LOG_ALWAYS_FATAL("Could not create the default velocity tracker strategy '%s'!",
                    strategy);
        }
    }
}

VelocityTracker::~VelocityTracker() {
    delete mStrategy;
}

bool VelocityTracker::configureStrategy(const char* strategy) {
    mStrategy = createStrategy(strategy);
    return mStrategy != NULL;
}

VelocityTrackerStrategy* VelocityTracker::createStrategy(const char* strategy) {
    if (!strcmp("lsq1", strategy)) {
        
        
        
        
        return new LeastSquaresVelocityTrackerStrategy(1);
    }
    if (!strcmp("lsq2", strategy)) {
        
        
        
        
        return new LeastSquaresVelocityTrackerStrategy(2);
    }
    if (!strcmp("lsq3", strategy)) {
        
        
        
        return new LeastSquaresVelocityTrackerStrategy(3);
    }
    if (!strcmp("wlsq2-delta", strategy)) {
        
        return new LeastSquaresVelocityTrackerStrategy(2,
                LeastSquaresVelocityTrackerStrategy::WEIGHTING_DELTA);
    }
    if (!strcmp("wlsq2-central", strategy)) {
        
        return new LeastSquaresVelocityTrackerStrategy(2,
                LeastSquaresVelocityTrackerStrategy::WEIGHTING_CENTRAL);
    }
    if (!strcmp("wlsq2-recent", strategy)) {
        
        return new LeastSquaresVelocityTrackerStrategy(2,
                LeastSquaresVelocityTrackerStrategy::WEIGHTING_RECENT);
    }
    if (!strcmp("int1", strategy)) {
        
        
        
        
        
        return new IntegratingVelocityTrackerStrategy(1);
    }
    if (!strcmp("int2", strategy)) {
        
        
        
        return new IntegratingVelocityTrackerStrategy(2);
    }
    if (!strcmp("legacy", strategy)) {
        
        
        
        
        return new LegacyVelocityTrackerStrategy();
    }
    return NULL;
}

void VelocityTracker::clear() {
    mCurrentPointerIdBits.clear();
    mActivePointerId = -1;

    mStrategy->clear();
}

void VelocityTracker::clearPointers(BitSet32 idBits) {
    BitSet32 remainingIdBits(mCurrentPointerIdBits.value & ~idBits.value);
    mCurrentPointerIdBits = remainingIdBits;

    if (mActivePointerId >= 0 && idBits.hasBit(mActivePointerId)) {
        mActivePointerId = !remainingIdBits.isEmpty() ? remainingIdBits.firstMarkedBit() : -1;
    }

    mStrategy->clearPointers(idBits);
}

void VelocityTracker::addMovement(nsecs_t eventTime, BitSet32 idBits, const Position* positions) {
    while (idBits.count() > MAX_POINTERS) {
        idBits.clearLastMarkedBit();
    }

    if ((mCurrentPointerIdBits.value & idBits.value)
            && eventTime >= mLastEventTime + ASSUME_POINTER_STOPPED_TIME) {
#if DEBUG_VELOCITY
        ALOGD("VelocityTracker: stopped for %0.3f ms, clearing state.",
                (eventTime - mLastEventTime) * 0.000001f);
#endif
        
        
        mStrategy->clear();
    }
    mLastEventTime = eventTime;

    mCurrentPointerIdBits = idBits;
    if (mActivePointerId < 0 || !idBits.hasBit(mActivePointerId)) {
        mActivePointerId = idBits.isEmpty() ? -1 : idBits.firstMarkedBit();
    }

    mStrategy->addMovement(eventTime, idBits, positions);

#if DEBUG_VELOCITY
    ALOGD("VelocityTracker: addMovement eventTime=%lld, idBits=0x%08x, activePointerId=%d",
            eventTime, idBits.value, mActivePointerId);
    for (BitSet32 iterBits(idBits); !iterBits.isEmpty(); ) {
        uint32_t id = iterBits.firstMarkedBit();
        uint32_t index = idBits.getIndexOfBit(id);
        iterBits.clearBit(id);
        Estimator estimator;
        getEstimator(id, &estimator);
        ALOGD("  %d: position (%0.3f, %0.3f), "
                "estimator (degree=%d, xCoeff=%s, yCoeff=%s, confidence=%f)",
                id, positions[index].x, positions[index].y,
                int(estimator.degree),
                vectorToString(estimator.xCoeff, estimator.degree + 1).string(),
                vectorToString(estimator.yCoeff, estimator.degree + 1).string(),
                estimator.confidence);
    }
#endif
}

void VelocityTracker::addMovement(const MotionEvent* event) {
    int32_t actionMasked = event->getActionMasked();

    switch (actionMasked) {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_HOVER_ENTER:
        
        clear();
        break;
    case AMOTION_EVENT_ACTION_POINTER_DOWN: {
        
        
        
        BitSet32 downIdBits;
        downIdBits.markBit(event->getPointerId(event->getActionIndex()));
        clearPointers(downIdBits);
        break;
    }
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
        break;
    default:
        
        
        
        
        
        
        
        
        return;
    }

    size_t pointerCount = event->getPointerCount();
    if (pointerCount > MAX_POINTERS) {
        pointerCount = MAX_POINTERS;
    }

    BitSet32 idBits;
    for (size_t i = 0; i < pointerCount; i++) {
        idBits.markBit(event->getPointerId(i));
    }

    uint32_t pointerIndex[MAX_POINTERS];
    for (size_t i = 0; i < pointerCount; i++) {
        pointerIndex[i] = idBits.getIndexOfBit(event->getPointerId(i));
    }

    nsecs_t eventTime;
    Position positions[pointerCount];

    size_t historySize = event->getHistorySize();
    for (size_t h = 0; h < historySize; h++) {
        eventTime = event->getHistoricalEventTime(h);
        for (size_t i = 0; i < pointerCount; i++) {
            uint32_t index = pointerIndex[i];
            positions[index].x = event->getHistoricalX(i, h);
            positions[index].y = event->getHistoricalY(i, h);
        }
        addMovement(eventTime, idBits, positions);
    }

    eventTime = event->getEventTime();
    for (size_t i = 0; i < pointerCount; i++) {
        uint32_t index = pointerIndex[i];
        positions[index].x = event->getX(i);
        positions[index].y = event->getY(i);
    }
    addMovement(eventTime, idBits, positions);
}

bool VelocityTracker::getVelocity(uint32_t id, float* outVx, float* outVy) const {
    Estimator estimator;
    if (getEstimator(id, &estimator) && estimator.degree >= 1) {
        *outVx = estimator.xCoeff[1];
        *outVy = estimator.yCoeff[1];
        return true;
    }
    *outVx = 0;
    *outVy = 0;
    return false;
}

bool VelocityTracker::getEstimator(uint32_t id, Estimator* outEstimator) const {
    return mStrategy->getEstimator(id, outEstimator);
}




const nsecs_t LeastSquaresVelocityTrackerStrategy::HORIZON;
const uint32_t LeastSquaresVelocityTrackerStrategy::HISTORY_SIZE;

LeastSquaresVelocityTrackerStrategy::LeastSquaresVelocityTrackerStrategy(
        uint32_t degree, Weighting weighting) :
        mDegree(degree), mWeighting(weighting) {
    clear();
}

LeastSquaresVelocityTrackerStrategy::~LeastSquaresVelocityTrackerStrategy() {
}

void LeastSquaresVelocityTrackerStrategy::clear() {
    mIndex = 0;
    mMovements[0].idBits.clear();
}

void LeastSquaresVelocityTrackerStrategy::clearPointers(BitSet32 idBits) {
    BitSet32 remainingIdBits(mMovements[mIndex].idBits.value & ~idBits.value);
    mMovements[mIndex].idBits = remainingIdBits;
}

void LeastSquaresVelocityTrackerStrategy::addMovement(nsecs_t eventTime, BitSet32 idBits,
        const VelocityTracker::Position* positions) {
    if (++mIndex == HISTORY_SIZE) {
        mIndex = 0;
    }

    Movement& movement = mMovements[mIndex];
    movement.eventTime = eventTime;
    movement.idBits = idBits;
    uint32_t count = idBits.count();
    for (uint32_t i = 0; i < count; i++) {
        movement.positions[i] = positions[i];
    }
}


















































static bool solveLeastSquares(const float* x, const float* y,
        const float* w, uint32_t m, uint32_t n, float* outB, float* outDet) {
#if DEBUG_STRATEGY
    ALOGD("solveLeastSquares: m=%d, n=%d, x=%s, y=%s, w=%s", int(m), int(n),
            vectorToString(x, m).string(), vectorToString(y, m).string(),
            vectorToString(w, m).string());
#endif

    
    float a[n][m]; 
    for (uint32_t h = 0; h < m; h++) {
        a[0][h] = w[h];
        for (uint32_t i = 1; i < n; i++) {
            a[i][h] = a[i - 1][h] * x[h];
        }
    }
#if DEBUG_STRATEGY
    ALOGD("  - a=%s", matrixToString(&a[0][0], m, n, false ).string());
#endif

    
    float q[n][m]; 
    float r[n][n]; 
    for (uint32_t j = 0; j < n; j++) {
        for (uint32_t h = 0; h < m; h++) {
            q[j][h] = a[j][h];
        }
        for (uint32_t i = 0; i < j; i++) {
            float dot = vectorDot(&q[j][0], &q[i][0], m);
            for (uint32_t h = 0; h < m; h++) {
                q[j][h] -= dot * q[i][h];
            }
        }

        float norm = vectorNorm(&q[j][0], m);
        if (norm < 0.000001f) {
            
#if DEBUG_STRATEGY
            ALOGD("  - no solution, norm=%f", norm);
#endif
            return false;
        }

        float invNorm = 1.0f / norm;
        for (uint32_t h = 0; h < m; h++) {
            q[j][h] *= invNorm;
        }
        for (uint32_t i = 0; i < n; i++) {
            r[j][i] = i < j ? 0 : vectorDot(&q[j][0], &a[i][0], m);
        }
    }
#if DEBUG_STRATEGY
    ALOGD("  - q=%s", matrixToString(&q[0][0], m, n, false ).string());
    ALOGD("  - r=%s", matrixToString(&r[0][0], n, n, true ).string());

    
    float qr[n][m];
    for (uint32_t h = 0; h < m; h++) {
        for (uint32_t i = 0; i < n; i++) {
            qr[i][h] = 0;
            for (uint32_t j = 0; j < n; j++) {
                qr[i][h] += q[j][h] * r[j][i];
            }
        }
    }
    ALOGD("  - qr=%s", matrixToString(&qr[0][0], m, n, false ).string());
#endif

    
    
    float wy[m];
    for (uint32_t h = 0; h < m; h++) {
        wy[h] = y[h] * w[h];
    }
    for (uint32_t i = n; i-- != 0; ) {
        outB[i] = vectorDot(&q[i][0], wy, m);
        for (uint32_t j = n - 1; j > i; j--) {
            outB[i] -= r[i][j] * outB[j];
        }
        outB[i] /= r[i][i];
    }
#if DEBUG_STRATEGY
    ALOGD("  - b=%s", vectorToString(outB, n).string());
#endif

    
    
    
    
    float ymean = 0;
    for (uint32_t h = 0; h < m; h++) {
        ymean += y[h];
    }
    ymean /= m;

    float sserr = 0;
    float sstot = 0;
    for (uint32_t h = 0; h < m; h++) {
        float err = y[h] - outB[0];
        float term = 1;
        for (uint32_t i = 1; i < n; i++) {
            term *= x[h];
            err -= term * outB[i];
        }
        sserr += w[h] * w[h] * err * err;
        float var = y[h] - ymean;
        sstot += w[h] * w[h] * var * var;
    }
    *outDet = sstot > 0.000001f ? 1.0f - (sserr / sstot) : 1;
#if DEBUG_STRATEGY
    ALOGD("  - sserr=%f", sserr);
    ALOGD("  - sstot=%f", sstot);
    ALOGD("  - det=%f", *outDet);
#endif
    return true;
}

bool LeastSquaresVelocityTrackerStrategy::getEstimator(uint32_t id,
        VelocityTracker::Estimator* outEstimator) const {
    outEstimator->clear();

    
    float x[HISTORY_SIZE];
    float y[HISTORY_SIZE];
    float w[HISTORY_SIZE];
    float time[HISTORY_SIZE];
    uint32_t m = 0;
    uint32_t index = mIndex;
    const Movement& newestMovement = mMovements[mIndex];
    do {
        const Movement& movement = mMovements[index];
        if (!movement.idBits.hasBit(id)) {
            break;
        }

        nsecs_t age = newestMovement.eventTime - movement.eventTime;
        if (age > HORIZON) {
            break;
        }

        const VelocityTracker::Position& position = movement.getPosition(id);
        x[m] = position.x;
        y[m] = position.y;
        w[m] = chooseWeight(index);
        time[m] = -age * 0.000000001f;
        index = (index == 0 ? HISTORY_SIZE : index) - 1;
    } while (++m < HISTORY_SIZE);

    if (m == 0) {
        return false; 
    }

    
    uint32_t degree = mDegree;
    if (degree > m - 1) {
        degree = m - 1;
    }
    if (degree >= 1) {
        float xdet, ydet;
        uint32_t n = degree + 1;
        if (solveLeastSquares(time, x, w, m, n, outEstimator->xCoeff, &xdet)
                && solveLeastSquares(time, y, w, m, n, outEstimator->yCoeff, &ydet)) {
            outEstimator->time = newestMovement.eventTime;
            outEstimator->degree = degree;
            outEstimator->confidence = xdet * ydet;
#if DEBUG_STRATEGY
            ALOGD("estimate: degree=%d, xCoeff=%s, yCoeff=%s, confidence=%f",
                    int(outEstimator->degree),
                    vectorToString(outEstimator->xCoeff, n).string(),
                    vectorToString(outEstimator->yCoeff, n).string(),
                    outEstimator->confidence);
#endif
            return true;
        }
    }

    
    outEstimator->xCoeff[0] = x[0];
    outEstimator->yCoeff[0] = y[0];
    outEstimator->time = newestMovement.eventTime;
    outEstimator->degree = 0;
    outEstimator->confidence = 1;
    return true;
}

float LeastSquaresVelocityTrackerStrategy::chooseWeight(uint32_t index) const {
    switch (mWeighting) {
    case WEIGHTING_DELTA: {
        
        
        
        
        if (index == mIndex) {
            return 1.0f;
        }
        uint32_t nextIndex = (index + 1) % HISTORY_SIZE;
        float deltaMillis = (mMovements[nextIndex].eventTime- mMovements[index].eventTime)
                * 0.000001f;
        if (deltaMillis < 0) {
            return 0.5f;
        }
        if (deltaMillis < 10) {
            return 0.5f + deltaMillis * 0.05;
        }
        return 1.0f;
    }

    case WEIGHTING_CENTRAL: {
        
        
        
        
        
        float ageMillis = (mMovements[mIndex].eventTime - mMovements[index].eventTime)
                * 0.000001f;
        if (ageMillis < 0) {
            return 0.5f;
        }
        if (ageMillis < 10) {
            return 0.5f + ageMillis * 0.05;
        }
        if (ageMillis < 50) {
            return 1.0f;
        }
        if (ageMillis < 60) {
            return 0.5f + (60 - ageMillis) * 0.05;
        }
        return 0.5f;
    }

    case WEIGHTING_RECENT: {
        
        
        
        
        float ageMillis = (mMovements[mIndex].eventTime - mMovements[index].eventTime)
                * 0.000001f;
        if (ageMillis < 50) {
            return 1.0f;
        }
        if (ageMillis < 100) {
            return 0.5f + (100 - ageMillis) * 0.01f;
        }
        return 0.5f;
    }

    case WEIGHTING_NONE:
    default:
        return 1.0f;
    }
}




IntegratingVelocityTrackerStrategy::IntegratingVelocityTrackerStrategy(uint32_t degree) :
        mDegree(degree) {
}

IntegratingVelocityTrackerStrategy::~IntegratingVelocityTrackerStrategy() {
}

void IntegratingVelocityTrackerStrategy::clear() {
    mPointerIdBits.clear();
}

void IntegratingVelocityTrackerStrategy::clearPointers(BitSet32 idBits) {
    mPointerIdBits.value &= ~idBits.value;
}

void IntegratingVelocityTrackerStrategy::addMovement(nsecs_t eventTime, BitSet32 idBits,
        const VelocityTracker::Position* positions) {
    uint32_t index = 0;
    for (BitSet32 iterIdBits(idBits); !iterIdBits.isEmpty();) {
        uint32_t id = iterIdBits.clearFirstMarkedBit();
        State& state = mPointerState[id];
        const VelocityTracker::Position& position = positions[index++];
        if (mPointerIdBits.hasBit(id)) {
            updateState(state, eventTime, position.x, position.y);
        } else {
            initState(state, eventTime, position.x, position.y);
        }
    }

    mPointerIdBits = idBits;
}

bool IntegratingVelocityTrackerStrategy::getEstimator(uint32_t id,
        VelocityTracker::Estimator* outEstimator) const {
    outEstimator->clear();

    if (mPointerIdBits.hasBit(id)) {
        const State& state = mPointerState[id];
        populateEstimator(state, outEstimator);
        return true;
    }

    return false;
}

void IntegratingVelocityTrackerStrategy::initState(State& state,
        nsecs_t eventTime, float xpos, float ypos) const {
    state.updateTime = eventTime;
    state.degree = 0;

    state.xpos = xpos;
    state.xvel = 0;
    state.xaccel = 0;
    state.ypos = ypos;
    state.yvel = 0;
    state.yaccel = 0;
}

void IntegratingVelocityTrackerStrategy::updateState(State& state,
        nsecs_t eventTime, float xpos, float ypos) const {
    const nsecs_t MIN_TIME_DELTA = 2 * NANOS_PER_MS;
    const float FILTER_TIME_CONSTANT = 0.010f; 

    if (eventTime <= state.updateTime + MIN_TIME_DELTA) {
        return;
    }

    float dt = (eventTime - state.updateTime) * 0.000000001f;
    state.updateTime = eventTime;

    float xvel = (xpos - state.xpos) / dt;
    float yvel = (ypos - state.ypos) / dt;
    if (state.degree == 0) {
        state.xvel = xvel;
        state.yvel = yvel;
        state.degree = 1;
    } else {
        float alpha = dt / (FILTER_TIME_CONSTANT + dt);
        if (mDegree == 1) {
            state.xvel += (xvel - state.xvel) * alpha;
            state.yvel += (yvel - state.yvel) * alpha;
        } else {
            float xaccel = (xvel - state.xvel) / dt;
            float yaccel = (yvel - state.yvel) / dt;
            if (state.degree == 1) {
                state.xaccel = xaccel;
                state.yaccel = yaccel;
                state.degree = 2;
            } else {
                state.xaccel += (xaccel - state.xaccel) * alpha;
                state.yaccel += (yaccel - state.yaccel) * alpha;
            }
            state.xvel += (state.xaccel * dt) * alpha;
            state.yvel += (state.yaccel * dt) * alpha;
        }
    }
    state.xpos = xpos;
    state.ypos = ypos;
}

void IntegratingVelocityTrackerStrategy::populateEstimator(const State& state,
        VelocityTracker::Estimator* outEstimator) const {
    outEstimator->time = state.updateTime;
    outEstimator->confidence = 1.0f;
    outEstimator->degree = state.degree;
    outEstimator->xCoeff[0] = state.xpos;
    outEstimator->xCoeff[1] = state.xvel;
    outEstimator->xCoeff[2] = state.xaccel / 2;
    outEstimator->yCoeff[0] = state.ypos;
    outEstimator->yCoeff[1] = state.yvel;
    outEstimator->yCoeff[2] = state.yaccel / 2;
}




const nsecs_t LegacyVelocityTrackerStrategy::HORIZON;
const uint32_t LegacyVelocityTrackerStrategy::HISTORY_SIZE;
const nsecs_t LegacyVelocityTrackerStrategy::MIN_DURATION;

LegacyVelocityTrackerStrategy::LegacyVelocityTrackerStrategy() {
    clear();
}

LegacyVelocityTrackerStrategy::~LegacyVelocityTrackerStrategy() {
}

void LegacyVelocityTrackerStrategy::clear() {
    mIndex = 0;
    mMovements[0].idBits.clear();
}

void LegacyVelocityTrackerStrategy::clearPointers(BitSet32 idBits) {
    BitSet32 remainingIdBits(mMovements[mIndex].idBits.value & ~idBits.value);
    mMovements[mIndex].idBits = remainingIdBits;
}

void LegacyVelocityTrackerStrategy::addMovement(nsecs_t eventTime, BitSet32 idBits,
        const VelocityTracker::Position* positions) {
    if (++mIndex == HISTORY_SIZE) {
        mIndex = 0;
    }

    Movement& movement = mMovements[mIndex];
    movement.eventTime = eventTime;
    movement.idBits = idBits;
    uint32_t count = idBits.count();
    for (uint32_t i = 0; i < count; i++) {
        movement.positions[i] = positions[i];
    }
}

bool LegacyVelocityTrackerStrategy::getEstimator(uint32_t id,
        VelocityTracker::Estimator* outEstimator) const {
    outEstimator->clear();

    const Movement& newestMovement = mMovements[mIndex];
    if (!newestMovement.idBits.hasBit(id)) {
        return false; 
    }

    
    nsecs_t minTime = newestMovement.eventTime - HORIZON;
    uint32_t oldestIndex = mIndex;
    uint32_t numTouches = 1;
    do {
        uint32_t nextOldestIndex = (oldestIndex == 0 ? HISTORY_SIZE : oldestIndex) - 1;
        const Movement& nextOldestMovement = mMovements[nextOldestIndex];
        if (!nextOldestMovement.idBits.hasBit(id)
                || nextOldestMovement.eventTime < minTime) {
            break;
        }
        oldestIndex = nextOldestIndex;
    } while (++numTouches < HISTORY_SIZE);

    
    
    
    
    
    
    
    
    
    
    
    float accumVx = 0;
    float accumVy = 0;
    uint32_t index = oldestIndex;
    uint32_t samplesUsed = 0;
    const Movement& oldestMovement = mMovements[oldestIndex];
    const VelocityTracker::Position& oldestPosition = oldestMovement.getPosition(id);
    nsecs_t lastDuration = 0;

    while (numTouches-- > 1) {
        if (++index == HISTORY_SIZE) {
            index = 0;
        }
        const Movement& movement = mMovements[index];
        nsecs_t duration = movement.eventTime - oldestMovement.eventTime;

        
        
        
        if (duration >= MIN_DURATION) {
            const VelocityTracker::Position& position = movement.getPosition(id);
            float scale = 1000000000.0f / duration; 
            float vx = (position.x - oldestPosition.x) * scale;
            float vy = (position.y - oldestPosition.y) * scale;
            accumVx = (accumVx * lastDuration + vx * duration) / (duration + lastDuration);
            accumVy = (accumVy * lastDuration + vy * duration) / (duration + lastDuration);
            lastDuration = duration;
            samplesUsed += 1;
        }
    }

    
    const VelocityTracker::Position& newestPosition = newestMovement.getPosition(id);
    outEstimator->time = newestMovement.eventTime;
    outEstimator->confidence = 1;
    outEstimator->xCoeff[0] = newestPosition.x;
    outEstimator->yCoeff[0] = newestPosition.y;
    if (samplesUsed) {
        outEstimator->xCoeff[1] = accumVx;
        outEstimator->yCoeff[1] = accumVy;
        outEstimator->degree = 1;
    } else {
        outEstimator->degree = 0;
    }
    return true;
}

} 
