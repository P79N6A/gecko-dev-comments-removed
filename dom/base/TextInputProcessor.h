




#ifndef mozilla_dom_textinputprocessor_h_
#define mozilla_dom_textinputprocessor_h_

#include "mozilla/EventForwards.h"
#include "mozilla/TextEventDispatcherListener.h"
#include "nsITextInputProcessor.h"
#include "nsITextInputProcessorCallback.h"
#include "nsTArray.h"

namespace mozilla {

namespace widget{
class TextEventDispatcher;
} 

class TextInputProcessor MOZ_FINAL : public nsITextInputProcessor
                                   , public widget::TextEventDispatcherListener
{
  typedef mozilla::widget::IMENotification IMENotification;
  typedef mozilla::widget::TextEventDispatcher TextEventDispatcher;

public:
  TextInputProcessor();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITEXTINPUTPROCESSOR

  
  NS_IMETHOD NotifyIME(TextEventDispatcher* aTextEventDispatcher,
                       const IMENotification& aNotification) MOZ_OVERRIDE;
  NS_IMETHOD_(void)
    OnRemovedFrom(TextEventDispatcher* aTextEventDispatcher) MOZ_OVERRIDE;

protected:
  virtual ~TextInputProcessor();

private:
  nsresult BeginInputTransactionInternal(
             nsIDOMWindow* aWindow,
             nsITextInputProcessorCallback* aCallback,
             bool aForTests,
             bool& aSucceeded);
  nsresult CommitCompositionInternal(const nsAString* aCommitString = nullptr,
                                     bool* aSucceeded = nullptr);
  nsresult CancelCompositionInternal();
  nsresult IsValidStateForComposition();
  void UnlinkFromTextEventDispatcher();
  nsresult PrepareKeyboardEventToDispatch(WidgetKeyboardEvent& aKeyboardEvent,
                                          uint32_t aKeyFlags);

  




  struct ModifierKeyData
  {
    
    KeyNameIndex mKeyNameIndex;
    
    CodeNameIndex mCodeNameIndex;
    
    Modifiers mModifier;

    explicit ModifierKeyData(const WidgetKeyboardEvent& aKeyboardEvent);

    bool operator==(const ModifierKeyData& aOther) const
    {
      return mKeyNameIndex == aOther.mKeyNameIndex &&
             mCodeNameIndex == aOther.mCodeNameIndex;
    }
  };

  class ModifierKeyDataArray : public nsTArray<ModifierKeyData>
  {
  public:
    Modifiers GetActiveModifiers() const;
    void ActivateModifierKey(const ModifierKeyData& aModifierKeyData);
    void InactivateModifierKey(const ModifierKeyData& aModifierKeyData);
    void ToggleModifierKey(const ModifierKeyData& aModifierKeyData);
  };

  Modifiers GetActiveModifiers() const
  {
    return mModifierKeyDataArray.GetActiveModifiers();
  }
  void ActivateModifierKey(const ModifierKeyData& aModifierKeyData)
  {
    mModifierKeyDataArray.ActivateModifierKey(aModifierKeyData);
  }
  void InactivateModifierKey(const ModifierKeyData& aModifierKeyData)
  {
    mModifierKeyDataArray.InactivateModifierKey(aModifierKeyData);
  }
  void ToggleModifierKey(const ModifierKeyData& aModifierKeyData)
  {
    mModifierKeyDataArray.ToggleModifierKey(aModifierKeyData);
  }

  TextEventDispatcher* mDispatcher; 
  nsCOMPtr<nsITextInputProcessorCallback> mCallback;
  ModifierKeyDataArray mModifierKeyDataArray;

  bool mForTests;
};

} 

#endif 
