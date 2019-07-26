



#include "history_indexer.h"


int main() {
  IChromeHistoryIndexer** indexer = 0;
  IID fake_iid;
  CoCreateInstance(fake_iid, NULL, CLSCTX_INPROC,
                   __uuidof(IChromeHistoryIndexer),
                   reinterpret_cast<void**>(indexer));
  return 0;
}
