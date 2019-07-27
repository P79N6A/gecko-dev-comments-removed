










#ifndef TouchManager_h_
#define TouchManager_h_

class PresShell;
class nsIDocument;

class TouchManager {
public:
  void Init(PresShell* aPresShell, nsIDocument* aDocument);
  void Destroy();

  bool PreHandleEvent(mozilla::WidgetEvent* aEvent,
                      nsEventStatus* aStatus,
                      bool& aTouchIsNew,
                      bool& aIsHandlingUserInput,
                      nsCOMPtr<nsIContent>& aCurrentEventContent);

private:
  void EvictTouches();

  nsRefPtr<PresShell>   mPresShell;
  nsCOMPtr<nsIDocument> mDocument;
};

#endif 
