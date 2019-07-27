





#ifndef TraceLoggingGraph_h
#define TraceLoggingGraph_h

#include "mozilla/DebugOnly.h"

#include "jslock.h"

#include "js/TypeDecls.h"
#include "vm/TraceLoggingTypes.h"

















































class TraceLoggerGraphState
{
    size_t numLoggers;

    
    FILE *out;

  public:
    PRLock *lock;

  public:
    TraceLoggerGraphState();
    ~TraceLoggerGraphState();

    uint32_t nextLoggerId();

  private:
    bool ensureInitialized();
};

class TraceLoggerGraph
{
    
    
    struct TreeEntry {
        uint64_t start_;
        uint64_t stop_;
        union {
            struct {
                uint32_t textId_: 31;
                uint32_t hasChildren_: 1;
            } s;
            uint32_t value_;
        } u;
        uint32_t nextId_;

        TreeEntry(uint64_t start, uint64_t stop, uint32_t textId, bool hasChildren,
                  uint32_t nextId)
        {
            start_ = start;
            stop_ = stop;
            u.s.textId_ = textId;
            u.s.hasChildren_ = hasChildren;
            nextId_ = nextId;
        }
        TreeEntry()
        { }
        uint64_t start() {
            return start_;
        }
        uint64_t stop() {
            return stop_;
        }
        uint32_t textId() {
            return u.s.textId_;
        }
        bool hasChildren() {
            return u.s.hasChildren_;
        }
        uint32_t nextId() {
            return nextId_;
        }
        void setStart(uint64_t start) {
            start_ = start;
        }
        void setStop(uint64_t stop) {
            stop_ = stop;
        }
        void setTextId(uint32_t textId) {
            MOZ_ASSERT(textId < uint32_t(1 << 31));
            u.s.textId_ = textId;
        }
        void setHasChildren(bool hasChildren) {
            u.s.hasChildren_ = hasChildren;
        }
        void setNextId(uint32_t nextId) {
            nextId_ = nextId;
        }
    };

    
    
    
    struct StackEntry {
        uint32_t treeId_;
        uint32_t lastChildId_;
        struct {
            uint32_t textId_: 31;
            uint32_t active_: 1;
        } s;
        StackEntry(uint32_t treeId, uint32_t lastChildId, bool active = true)
          : treeId_(treeId), lastChildId_(lastChildId)
        {
            s.textId_ = 0;
            s.active_ = active;
        }
        uint32_t treeId() {
            return treeId_;
        }
        uint32_t lastChildId() {
            return lastChildId_;
        }
        uint32_t textId() {
            return s.textId_;
        }
        bool active() {
            return s.active_;
        }
        void setTreeId(uint32_t treeId) {
            treeId_ = treeId;
        }
        void setLastChildId(uint32_t lastChildId) {
            lastChildId_ = lastChildId;
        }
        void setTextId(uint32_t textId) {
            MOZ_ASSERT(textId < uint32_t(1<<31));
            s.textId_ = textId;
        }
        void setActive(bool active) {
            s.active_ = active;
        }
    };

  public:
    TraceLoggerGraph()
      : failed(false),
        enabled(false),
        nextTextId(0)
    { }
    ~TraceLoggerGraph();

    bool init(uint64_t timestamp);

    
    void addTextId(uint32_t id, const char *text);

    
    void log(ContinuousSpace<EventEntry> &events);

  private:
    bool failed;
    bool enabled;
    mozilla::DebugOnly<uint32_t> nextTextId;

    FILE *dictFile;
    FILE *treeFile;
    FILE *eventFile;

    ContinuousSpace<TreeEntry> tree;
    ContinuousSpace<StackEntry> stack;
    uint32_t treeOffset;

    
    
    void entryToBigEndian(TreeEntry *entry);
    void entryToSystemEndian(TreeEntry *entry);

    
    bool getTreeEntry(uint32_t treeId, TreeEntry *entry);
    bool saveTreeEntry(uint32_t treeId, TreeEntry *entry);

    
    StackEntry &getActiveAncestor();

    
    
    void startEvent(uint32_t id, uint64_t timestamp);
    bool startEventInternal(uint32_t id, uint64_t timestamp);

    
    
    bool updateHasChildren(uint32_t treeId, bool hasChildren = true);
    bool updateNextId(uint32_t treeId, uint32_t nextId);
    bool updateStop(uint32_t treeId, uint64_t timestamp);

    
    bool flush();

    
    void stopEvent(uint32_t id, uint64_t timestamp);
    void stopEvent(uint64_t timestamp);

    
    void logTimestamp(uint32_t id, uint64_t timestamp);

    
    
    void disable(uint64_t timestamp);
};

#endif 
