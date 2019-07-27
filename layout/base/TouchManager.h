










#ifndef TouchManager_h_
#define TouchManager_h_

class PresShell;
class nsIDocument;

class TouchManager {
public:
  
  static void InitializeStatics();
  static void ReleaseStatics();

  void Init(PresShell* aPresShell, nsIDocument* aDocument);
  void Destroy();

  bool PreHandleEvent(mozilla::WidgetEvent* aEvent,
                      nsEventStatus* aStatus,
                      bool& aTouchIsNew,
                      bool& aIsHandlingUserInput,
                      nsCOMPtr<nsIContent>& aCurrentEventContent);

  static bool gPreventMouseEvents;
  static nsRefPtrHashtable<nsUint32HashKey, mozilla::dom::Touch>* gCaptureTouchList;

private:
  void EvictTouches();

  nsRefPtr<PresShell>   mPresShell;
  nsCOMPtr<nsIDocument> mDocument;
};

#endif 
