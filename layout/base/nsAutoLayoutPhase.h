



#ifndef nsAutoLayoutPhase_h
#define nsAutoLayoutPhase_h

#ifdef DEBUG



#include "nsPresContext.h"

struct nsAutoLayoutPhase {
  nsAutoLayoutPhase(nsPresContext* aPresContext, nsLayoutPhase aPhase);
  ~nsAutoLayoutPhase();

  void Enter();
  void Exit();

private:
  nsPresContext* mPresContext;
  nsLayoutPhase mPhase;
  uint32_t mCount;
};

#define AUTO_LAYOUT_PHASE_ENTRY_POINT(pc_, phase_) \
  nsAutoLayoutPhase autoLayoutPhase((pc_), (eLayoutPhase_##phase_))
#define LAYOUT_PHASE_TEMP_EXIT() \
  PR_BEGIN_MACRO \
    autoLayoutPhase.Exit(); \
  PR_END_MACRO
#define LAYOUT_PHASE_TEMP_REENTER() \
  PR_BEGIN_MACRO \
    autoLayoutPhase.Enter(); \
  PR_END_MACRO

#else 

#define AUTO_LAYOUT_PHASE_ENTRY_POINT(pc_, phase_) \
  PR_BEGIN_MACRO PR_END_MACRO
#define LAYOUT_PHASE_TEMP_EXIT() \
  PR_BEGIN_MACRO PR_END_MACRO
#define LAYOUT_PHASE_TEMP_REENTER() \
  PR_BEGIN_MACRO PR_END_MACRO

#endif 

#endif 
