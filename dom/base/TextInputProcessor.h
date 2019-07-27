




#ifndef mozilla_dom_textinputprocessor_h_
#define mozilla_dom_textinputprocessor_h_

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TextEventDispatcherListener.h"
#include "nsAutoPtr.h"
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
  bool IsComposing() const;
  nsresult BeginInputTransactionInternal(
             nsIDOMWindow* aWindow,
             nsITextInputProcessorCallback* aCallback,
             bool aForTests,
             bool& aSucceeded);
  nsresult CommitCompositionInternal(
             const WidgetKeyboardEvent* aKeyboardEvent = nullptr,
             uint32_t aKeyFlags = 0,
             const nsAString* aCommitString = nullptr,
             bool* aSucceeded = nullptr);
  nsresult CancelCompositionInternal(
             const WidgetKeyboardEvent* aKeyboardEvent = nullptr,
             uint32_t aKeyFlags = 0);
  nsresult KeydownInternal(const WidgetKeyboardEvent& aKeyboardEvent,
                           uint32_t aKeyFlags,
                           bool aAllowToDispatchKeypress,
                           bool& aDoDefault);
  nsresult KeyupInternal(const WidgetKeyboardEvent& aKeyboardEvent,
                         uint32_t aKeyFlags,
                         bool& aDoDefault);
  nsresult IsValidStateForComposition();
  void UnlinkFromTextEventDispatcher();
  nsresult PrepareKeyboardEventToDispatch(WidgetKeyboardEvent& aKeyboardEvent,
                                          uint32_t aKeyFlags);
  bool IsValidEventTypeForComposition(
         const WidgetKeyboardEvent& aKeyboardEvent) const;
  nsresult PrepareKeyboardEventForComposition(
             nsIDOMKeyEvent* aDOMKeyEvent,
             uint32_t& aKeyFlags,
             uint8_t aOptionalArgc,
             WidgetKeyboardEvent*& aKeyboardEvent);

  struct EventDispatcherResult
  {
    nsresult mResult;
    bool     mDoDefault;
    bool     mCanContinue;

    EventDispatcherResult()
      : mResult(NS_OK)
      , mDoDefault(true)
      , mCanContinue(true)
    {
    }
  };
  EventDispatcherResult MaybeDispatchKeydownForComposition(
                          const WidgetKeyboardEvent* aKeyboardEvent,
                          uint32_t aKeyFlags);
  EventDispatcherResult MaybeDispatchKeyupForComposition(
                          const WidgetKeyboardEvent* aKeyboardEvent,
                          uint32_t aKeyFlags);

  



  class MOZ_STACK_CLASS AutoPendingCompositionResetter
  {
  public:
    explicit AutoPendingCompositionResetter(TextInputProcessor* aTIP);
    ~AutoPendingCompositionResetter();

  private:
    nsRefPtr<TextInputProcessor> mTIP;
  };

  




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
    NS_INLINE_DECL_REFCOUNTING(ModifierKeyDataArray)

  public:
    Modifiers GetActiveModifiers() const;
    void ActivateModifierKey(const ModifierKeyData& aModifierKeyData);
    void InactivateModifierKey(const ModifierKeyData& aModifierKeyData);
    void ToggleModifierKey(const ModifierKeyData& aModifierKeyData);

  private:
    virtual ~ModifierKeyDataArray() { }
  };

  Modifiers GetActiveModifiers() const
  {
    return mModifierKeyDataArray ?
      mModifierKeyDataArray->GetActiveModifiers() : 0;
  }
  void EnsureModifierKeyDataArray()
  {
    if (mModifierKeyDataArray) {
      return;
    }
    mModifierKeyDataArray = new ModifierKeyDataArray();
  }
  void ActivateModifierKey(const ModifierKeyData& aModifierKeyData)
  {
    EnsureModifierKeyDataArray();
    mModifierKeyDataArray->ActivateModifierKey(aModifierKeyData);
  }
  void InactivateModifierKey(const ModifierKeyData& aModifierKeyData)
  {
    if (!mModifierKeyDataArray) {
      return;
    }
    mModifierKeyDataArray->InactivateModifierKey(aModifierKeyData);
  }
  void ToggleModifierKey(const ModifierKeyData& aModifierKeyData)
  {
    EnsureModifierKeyDataArray();
    mModifierKeyDataArray->ToggleModifierKey(aModifierKeyData);
  }

  TextEventDispatcher* mDispatcher; 
  nsCOMPtr<nsITextInputProcessorCallback> mCallback;
  nsRefPtr<ModifierKeyDataArray> mModifierKeyDataArray;

  bool mForTests;
};

} 

#endif 
