





#include "vm/TraceLoggingGraph.h"

#include "mozilla/Endian.h"

#include "vm/TraceLogging.h"

#ifndef TRACE_LOG_DIR
# if defined(_WIN32)
#  define TRACE_LOG_DIR ""
# else
#  define TRACE_LOG_DIR "/tmp/"
# endif
#endif

using mozilla::NativeEndian;

TraceLoggerGraphState traceLoggersGraph;

class AutoTraceLoggerGraphStateLock
{
  TraceLoggerGraphState *graph;

  public:
    AutoTraceLoggerGraphStateLock(TraceLoggerGraphState *graph MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : graph(graph)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        PR_Lock(graph->lock);
    }
    ~AutoTraceLoggerGraphStateLock() {
        PR_Unlock(graph->lock);
    }
  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

TraceLoggerGraphState::TraceLoggerGraphState()
  : numLoggers(0),
    out(nullptr)
{
    lock = PR_NewLock();
    if (!lock)
        MOZ_CRASH();
}

bool
TraceLoggerGraphState::ensureInitialized()
{
    if (out)
        return true;

    out = fopen(TRACE_LOG_DIR "tl-data.json", "w");
    if (!out)
        return false;

    fprintf(out, "[");
    return true;
}

TraceLoggerGraphState::~TraceLoggerGraphState()
{
    if (out) {
        fprintf(out, "]");
        fclose(out);
        out = nullptr;
    }

    if (lock) {
        PR_DestroyLock(lock);
        lock = nullptr;
    }
}

uint32_t
TraceLoggerGraphState::nextLoggerId()
{
    AutoTraceLoggerGraphStateLock lock(this);

    if (!ensureInitialized()) {
        fprintf(stderr, "TraceLogging: Couldn't create the main log file.");
        return uint32_t(-1);
    }

    if (numLoggers > 999) {
        fprintf(stderr, "TraceLogging: Can't create more than 999 different loggers.");
        return uint32_t(-1);
    }

    if (numLoggers > 0) {
        int written = fprintf(out, ",\n");
        if (written < 0) {
            fprintf(stderr, "TraceLogging: Error while writing.\n");
            return uint32_t(-1);
        }
    }

    int written = fprintf(out, "{\"tree\":\"tl-tree.%d.tl\", \"events\":\"tl-event.%d.tl\", "
                               "\"dict\":\"tl-dict.%d.json\", \"treeFormat\":\"64,64,31,1,32\"}",
                          numLoggers, numLoggers, numLoggers);
    if (written < 0) {
        fprintf(stderr, "TraceLogging: Error while writing.\n");
        return uint32_t(-1);
    }

    return numLoggers++;
}

bool
TraceLoggerGraph::init(uint64_t startTimestamp)
{
    if (!tree.init()) {
        failed = true;
        return false;
    }
    if (!stack.init()) {
        failed = true;
        return false;
    }

    uint32_t loggerId = traceLoggersGraph.nextLoggerId();
    if (loggerId == uint32_t(-1)) {
        failed = true;
        return false;
    }

    char dictFilename[sizeof TRACE_LOG_DIR "tl-dict.100.json"];
    sprintf(dictFilename, TRACE_LOG_DIR "tl-dict.%d.json", loggerId);
    dictFile = fopen(dictFilename, "w");
    if (!dictFile) {
        failed = true;
        return false;
    }

    char treeFilename[sizeof TRACE_LOG_DIR "tl-tree.100.tl"];
    sprintf(treeFilename, TRACE_LOG_DIR "tl-tree.%d.tl", loggerId);
    treeFile = fopen(treeFilename, "wb");
    if (!treeFile) {
        fclose(dictFile);
        dictFile = nullptr;
        failed = true;
        return false;
    }

    char eventFilename[sizeof TRACE_LOG_DIR "tl-event.100.tl"];
    sprintf(eventFilename, TRACE_LOG_DIR "tl-event.%d.tl", loggerId);
    eventFile = fopen(eventFilename, "wb");
    if (!eventFile) {
        fclose(dictFile);
        fclose(treeFile);
        dictFile = nullptr;
        treeFile = nullptr;
        failed = true;
        return false;
    }

    
    TreeEntry &treeEntry = tree.pushUninitialized();
    treeEntry.setStart(startTimestamp);
    treeEntry.setStop(0);
    treeEntry.setTextId(0);
    treeEntry.setHasChildren(false);
    treeEntry.setNextId(0);

    StackEntry &stackEntry = stack.pushUninitialized();
    stackEntry.setTreeId(0);
    stackEntry.setLastChildId(0);
    stackEntry.setActive(true);

    int written = fprintf(dictFile, "[");
    if (written < 0) {
        fprintf(stderr, "TraceLogging: Error while writing.\n");
        fclose(dictFile);
        fclose(treeFile);
        fclose(eventFile);
        dictFile = nullptr;
        treeFile = nullptr;
        eventFile = nullptr;
        failed = true;
        return false;
    }

    return true;
}

TraceLoggerGraph::~TraceLoggerGraph()
{
    
    if (dictFile) {
        int written = fprintf(dictFile, "]");
        if (written < 0)
            fprintf(stderr, "TraceLogging: Error while writing.\n");
        fclose(dictFile);

        dictFile = nullptr;
    }

    if (!failed && treeFile) {
        
        
        
        enabled = 1;
        while (stack.size() > 1)
            stopEvent(0);
        enabled = 0;
    }

    if (!failed && !flush()) {
        fprintf(stderr, "TraceLogging: Couldn't write the data to disk.\n");
        enabled = 0;
        failed = true;
    }

    if (treeFile) {
        fclose(treeFile);
        treeFile = nullptr;
    }

    if (eventFile) {
        fclose(eventFile);
        eventFile = nullptr;
    }
}

bool
TraceLoggerGraph::flush()
{
    MOZ_ASSERT(!failed);

    if (treeFile) {
        
        for (size_t i = 0; i < tree.size(); i++)
            entryToBigEndian(&tree[i]);

        int success = fseek(treeFile, 0, SEEK_END);
        if (success != 0)
            return false;

        size_t bytesWritten = fwrite(tree.data(), sizeof(TreeEntry), tree.size(), treeFile);
        if (bytesWritten < tree.size())
            return false;

        treeOffset += tree.lastEntryId();
        tree.clear();
    }

    return true;
}

void
TraceLoggerGraph::entryToBigEndian(TreeEntry *entry)
{
    entry->start_ = NativeEndian::swapToBigEndian(entry->start_);
    entry->stop_ = NativeEndian::swapToBigEndian(entry->stop_);
    uint32_t data = (entry->u.s.textId_ << 1) + entry->u.s.hasChildren_;
    entry->u.value_ = NativeEndian::swapToBigEndian(data);
    entry->nextId_ = NativeEndian::swapToBigEndian(entry->nextId_);
}

void
TraceLoggerGraph::entryToSystemEndian(TreeEntry *entry)
{
    entry->start_ = NativeEndian::swapFromBigEndian(entry->start_);
    entry->stop_ = NativeEndian::swapFromBigEndian(entry->stop_);

    uint32_t data = NativeEndian::swapFromBigEndian(entry->u.value_);
    entry->u.s.textId_ = data >> 1;
    entry->u.s.hasChildren_ = data & 0x1;

    entry->nextId_ = NativeEndian::swapFromBigEndian(entry->nextId_);
}

void
TraceLoggerGraph::startEvent(uint32_t id, uint64_t timestamp)
{
    if (failed || enabled == 0)
        return;

    if (!tree.hasSpaceForAdd()) {
        if (!tree.ensureSpaceBeforeAdd()) {
            if (!flush()) {
                fprintf(stderr, "TraceLogging: Couldn't write the data to disk.\n");
                enabled = 0;
                failed = true;
                return;
            }
        }
    }

    if (!startEventInternal(id, timestamp)) {
        fprintf(stderr, "TraceLogging: Failed to start an event.\n");
        enabled = 0;
        failed = true;
        return;
    }
}

TraceLoggerGraph::StackEntry &
TraceLoggerGraph::getActiveAncestor()
{
    uint32_t parentId = stack.lastEntryId();
    while (!stack[parentId].active())
        parentId--;
    return stack[parentId];
}

bool
TraceLoggerGraph::startEventInternal(uint32_t id, uint64_t timestamp)
{
    if (!stack.ensureSpaceBeforeAdd())
        return false;

    
    
    
    
    StackEntry &parent = getActiveAncestor();
#ifdef DEBUG
    TreeEntry entry;
    if (!getTreeEntry(parent.treeId(), &entry))
        return false;
#endif

    if (parent.lastChildId() == 0) {
        MOZ_ASSERT(!entry.hasChildren());
        MOZ_ASSERT(parent.treeId() == tree.lastEntryId() + treeOffset);

        if (!updateHasChildren(parent.treeId()))
            return false;
    } else {
        MOZ_ASSERT(entry.hasChildren());

        if (!updateNextId(parent.lastChildId(), tree.size() + treeOffset))
            return false;
    }

    
    TreeEntry &treeEntry = tree.pushUninitialized();
    treeEntry.setStart(timestamp);
    treeEntry.setStop(0);
    treeEntry.setTextId(id);
    treeEntry.setHasChildren(false);
    treeEntry.setNextId(0);

    
    StackEntry &stackEntry = stack.pushUninitialized();
    stackEntry.setTreeId(tree.lastEntryId() + treeOffset);
    stackEntry.setLastChildId(0);
    stackEntry.setActive(true);

    
    parent.setLastChildId(tree.lastEntryId() + treeOffset);

    return true;
}

void
TraceLoggerGraph::stopEvent(uint32_t id, uint64_t timestamp)
{
#ifdef DEBUG
    if (id != TraceLogger_Scripts &&
        id != TraceLogger_Engine &&
        stack.size() > 1 &&
        stack.lastEntry().active())
    {
        TreeEntry entry;
        MOZ_ASSERT(getTreeEntry(stack.lastEntry().treeId(), &entry));
        MOZ_ASSERT(entry.textId() == id);
    }
#endif

    stopEvent(timestamp);
}

void
TraceLoggerGraph::stopEvent(uint64_t timestamp)
{
    if (enabled && stack.lastEntry().active()) {
        if (!updateStop(stack.lastEntry().treeId(), timestamp)) {
            fprintf(stderr, "TraceLogging: Failed to stop an event.\n");
            enabled = 0;
            failed = true;
            return;
        }
    }
    if (stack.size() == 1) {
        if (!enabled)
            return;

        
        logTimestamp(TraceLogger_Disable, timestamp);
        return;
    }
    stack.pop();
}

void
TraceLoggerGraph::logTimestamp(uint32_t id, uint64_t timestamp)
{
    if (failed)
        return;

    if (id == TraceLogger_Enable)
        enabled = true;

    if (!enabled)
        return;

    if (id == TraceLogger_Disable)
        disable(timestamp);

    MOZ_ASSERT(eventFile);

    
    timestamp = NativeEndian::swapToBigEndian(timestamp);
    id = NativeEndian::swapToBigEndian(id);

    
    
    size_t itemsWritten = 0;
    itemsWritten += fwrite(&timestamp, sizeof(uint64_t), 1, eventFile);
    itemsWritten += fwrite(&id, sizeof(uint32_t), 1, eventFile);
    if (itemsWritten < 2) {
        failed = true;
        enabled = false;
    }
}

bool
TraceLoggerGraph::getTreeEntry(uint32_t treeId, TreeEntry *entry)
{
    
    if (treeId >= treeOffset) {
        *entry = tree[treeId - treeOffset];
        return true;
    }

    int success = fseek(treeFile, treeId * sizeof(TreeEntry), SEEK_SET);
    if (success != 0)
        return false;

    size_t itemsRead = fread((void *)entry, sizeof(TreeEntry), 1, treeFile);
    if (itemsRead < 1)
        return false;

    entryToSystemEndian(entry);
    return true;
}

bool
TraceLoggerGraph::saveTreeEntry(uint32_t treeId, TreeEntry *entry)
{
    int success = fseek(treeFile, treeId * sizeof(TreeEntry), SEEK_SET);
    if (success != 0)
        return false;

    entryToBigEndian(entry);

    size_t itemsWritten = fwrite(entry, sizeof(TreeEntry), 1, treeFile);
    if (itemsWritten < 1)
        return false;

    return true;
}

bool
TraceLoggerGraph::updateHasChildren(uint32_t treeId, bool hasChildren)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.setHasChildren(hasChildren);
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].setHasChildren(hasChildren);
    return true;
}

bool
TraceLoggerGraph::updateNextId(uint32_t treeId, uint32_t nextId)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.setNextId(nextId);
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].setNextId(nextId);
    return true;
}

bool
TraceLoggerGraph::updateStop(uint32_t treeId, uint64_t timestamp)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.setStop(timestamp);
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].setStop(timestamp);
    return true;
}

void
TraceLoggerGraph::disable(uint64_t timestamp)
{
    MOZ_ASSERT(enabled);
    while (stack.size() > 1)
        stopEvent(timestamp);

    enabled = false;
}

void
TraceLoggerGraph::log(ContinuousSpace<EventEntry> &events)
{
    if (!enabled)
        return;

    for (uint32_t i = 0; i < events.size(); i++) {
        if (events[i].textId == TraceLogger_Stop)
            stopEvent(events[i].time);
        else if (TLTextIdIsTreeEvent(events[i].textId))
            startEvent(events[i].textId, events[i].time);
        else
            logTimestamp(events[i].textId, events[i].time);
    }
}

void
TraceLoggerGraph::addTextId(uint32_t id, const char *text)
{
    if (failed)
        return;

    
    MOZ_ASSERT(id == nextTextId);
    nextTextId++;

    int written;
    if (id > 0)
        written = fprintf(dictFile, ",\n\"%s\"", text);
    else
        written = fprintf(dictFile, "\"%s\"", text);

    if (written < 0)
        failed = true;
}
