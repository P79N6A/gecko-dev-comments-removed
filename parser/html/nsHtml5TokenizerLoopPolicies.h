




































#ifndef nsHtml5TokenizerLoopPolicies_h_
#define nsHtml5TokenizerLoopPolicies_h_





struct nsHtml5SilentPolicy
{
  static const bool reportErrors = false;
  static PRInt32 transition(nsHtml5Highlighter* aHighlighter,
                            PRInt32 aState,
                            bool aReconsume,
                            PRInt32 aPos) {
    return aState;
  }
  static void completedNamedCharacterReference(nsHtml5Highlighter* aHighlighter) {
  }
};





struct nsHtml5ViewSourcePolicy
{
  static const bool reportErrors = true;
  static PRInt32 transition(nsHtml5Highlighter* aHighlighter,
                            PRInt32 aState,
                            bool aReconsume,
                            PRInt32 aPos) {
    return aHighlighter->Transition(aState, aReconsume, aPos);
  }
  static void completedNamedCharacterReference(nsHtml5Highlighter* aHighlighter) {
    aHighlighter->CompletedNamedCharacterReference();
  }
};

#endif 
