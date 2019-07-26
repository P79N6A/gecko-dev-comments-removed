















#ifndef _ANDROIDFW_VELOCITY_TRACKER_H
#define _ANDROIDFW_VELOCITY_TRACKER_H

#include "Input.h"
#include <utils/Timers.h>
#include <utils/BitSet.h>

namespace android {

class VelocityTrackerStrategy;




class VelocityTracker {
public:
    struct Position {
        float x, y;
    };

    struct Estimator {
        static const size_t MAX_DEGREE = 4;

        
        nsecs_t time;

        
        float xCoeff[MAX_DEGREE + 1], yCoeff[MAX_DEGREE + 1];

        
        
        uint32_t degree;

        
        float confidence;

        inline void clear() {
            time = 0;
            degree = 0;
            confidence = 0;
            for (size_t i = 0; i <= MAX_DEGREE; i++) {
                xCoeff[i] = 0;
                yCoeff[i] = 0;
            }
        }
    };

    
    
    VelocityTracker(const char* strategy = NULL);

    ~VelocityTracker();

    
    void clear();

    
    
    
    void clearPointers(BitSet32 idBits);

    
    
    
    
    
    void addMovement(nsecs_t eventTime, BitSet32 idBits, const Position* positions);

    
    void addMovement(const MotionEvent* event);

    
    
    
    bool getVelocity(uint32_t id, float* outVx, float* outVy) const;

    
    
    
    bool getEstimator(uint32_t id, Estimator* outEstimator) const;

    
    inline int32_t getActivePointerId() const { return mActivePointerId; }

    
    inline BitSet32 getCurrentPointerIdBits() const { return mCurrentPointerIdBits; }

private:
    static const char* DEFAULT_STRATEGY;

    nsecs_t mLastEventTime;
    BitSet32 mCurrentPointerIdBits;
    int32_t mActivePointerId;
    VelocityTrackerStrategy* mStrategy;

    bool configureStrategy(const char* strategy);

    static VelocityTrackerStrategy* createStrategy(const char* strategy);
};





class VelocityTrackerStrategy {
protected:
    VelocityTrackerStrategy() { }

public:
    virtual ~VelocityTrackerStrategy() { }

    virtual void clear() = 0;
    virtual void clearPointers(BitSet32 idBits) = 0;
    virtual void addMovement(nsecs_t eventTime, BitSet32 idBits,
            const VelocityTracker::Position* positions) = 0;
    virtual bool getEstimator(uint32_t id, VelocityTracker::Estimator* outEstimator) const = 0;
};





class LeastSquaresVelocityTrackerStrategy : public VelocityTrackerStrategy {
public:
    enum Weighting {
        
        WEIGHTING_NONE,

        
        WEIGHTING_DELTA,

        
        
        WEIGHTING_CENTRAL,

        
        WEIGHTING_RECENT,
    };

    
    LeastSquaresVelocityTrackerStrategy(uint32_t degree, Weighting weighting = WEIGHTING_NONE);
    virtual ~LeastSquaresVelocityTrackerStrategy();

    virtual void clear();
    virtual void clearPointers(BitSet32 idBits);
    virtual void addMovement(nsecs_t eventTime, BitSet32 idBits,
            const VelocityTracker::Position* positions);
    virtual bool getEstimator(uint32_t id, VelocityTracker::Estimator* outEstimator) const;

private:
    
    
    
    static const nsecs_t HORIZON = 100 * 1000000; 

    
    static const uint32_t HISTORY_SIZE = 20;

    struct Movement {
        nsecs_t eventTime;
        BitSet32 idBits;
        VelocityTracker::Position positions[MAX_POINTERS];

        inline const VelocityTracker::Position& getPosition(uint32_t id) const {
            return positions[idBits.getIndexOfBit(id)];
        }
    };

    float chooseWeight(uint32_t index) const;

    const uint32_t mDegree;
    const Weighting mWeighting;
    uint32_t mIndex;
    Movement mMovements[HISTORY_SIZE];
};





class IntegratingVelocityTrackerStrategy : public VelocityTrackerStrategy {
public:
    
    IntegratingVelocityTrackerStrategy(uint32_t degree);
    ~IntegratingVelocityTrackerStrategy();

    virtual void clear();
    virtual void clearPointers(BitSet32 idBits);
    virtual void addMovement(nsecs_t eventTime, BitSet32 idBits,
            const VelocityTracker::Position* positions);
    virtual bool getEstimator(uint32_t id, VelocityTracker::Estimator* outEstimator) const;

private:
    
    struct State {
        nsecs_t updateTime;
        uint32_t degree;

        float xpos, xvel, xaccel;
        float ypos, yvel, yaccel;
    };

    const uint32_t mDegree;
    BitSet32 mPointerIdBits;
    State mPointerState[MAX_POINTER_ID + 1];

    void initState(State& state, nsecs_t eventTime, float xpos, float ypos) const;
    void updateState(State& state, nsecs_t eventTime, float xpos, float ypos) const;
    void populateEstimator(const State& state, VelocityTracker::Estimator* outEstimator) const;
};





class LegacyVelocityTrackerStrategy : public VelocityTrackerStrategy {
public:
    LegacyVelocityTrackerStrategy();
    virtual ~LegacyVelocityTrackerStrategy();

    virtual void clear();
    virtual void clearPointers(BitSet32 idBits);
    virtual void addMovement(nsecs_t eventTime, BitSet32 idBits,
            const VelocityTracker::Position* positions);
    virtual bool getEstimator(uint32_t id, VelocityTracker::Estimator* outEstimator) const;

private:
    
    static const nsecs_t HORIZON = 200 * 1000000; 

    
    static const uint32_t HISTORY_SIZE = 20;

    
    static const nsecs_t MIN_DURATION = 10 * 1000000; 

    struct Movement {
        nsecs_t eventTime;
        BitSet32 idBits;
        VelocityTracker::Position positions[MAX_POINTERS];

        inline const VelocityTracker::Position& getPosition(uint32_t id) const {
            return positions[idBits.getIndexOfBit(id)];
        }
    };

    uint32_t mIndex;
    Movement mMovements[HISTORY_SIZE];
};

} 

#endif 
