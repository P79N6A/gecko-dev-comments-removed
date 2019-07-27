


#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/MultiplexConsumer.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <memory>

#define CLANG_VERSION_FULL (CLANG_VERSION_MAJOR * 100 + CLANG_VERSION_MINOR)

using namespace llvm;
using namespace clang;

#if CLANG_VERSION_FULL >= 306
typedef std::unique_ptr<ASTConsumer> ASTConsumerPtr;
#else
typedef ASTConsumer *ASTConsumerPtr;
#endif

namespace {

using namespace clang::ast_matchers;
class DiagnosticsMatcher {
public:
  DiagnosticsMatcher();

  ASTConsumerPtr makeASTConsumer() {
    return astMatcher.newASTConsumer();
  }

private:
  class StackClassChecker : public MatchFinder::MatchCallback {
  public:
    virtual void run(const MatchFinder::MatchResult &Result);
    void noteInferred(QualType T, DiagnosticsEngine &Diag);
  };

  class NonHeapClassChecker : public MatchFinder::MatchCallback {
  public:
    virtual void run(const MatchFinder::MatchResult &Result);
    void noteInferred(QualType T, DiagnosticsEngine &Diag);
  };

  StackClassChecker stackClassChecker;
  NonHeapClassChecker nonheapClassChecker;
  MatchFinder astMatcher;
};

namespace {

bool isInIgnoredNamespace(const Decl *decl) {
  const DeclContext *DC = decl->getDeclContext()->getEnclosingNamespaceContext();
  const NamespaceDecl *ND = dyn_cast<NamespaceDecl>(DC);
  if (!ND) {
    return false;
  }

  while (const DeclContext *ParentDC = ND->getParent()) {
    if (!isa<NamespaceDecl>(ParentDC)) {
      break;
    }
    ND = cast<NamespaceDecl>(ParentDC);
  }

  const auto& name = ND->getName();

  
  return name == "std" ||              
         name == "__gnu_cxx" ||        
         name == "boost" ||            
         name == "webrtc" ||           
         name == "icu_52" ||           
         name == "google" ||           
         name == "google_breakpad" ||  
         name == "soundtouch" ||       
         name == "stagefright" ||      
         name == "MacFileUtilities" || 
         name == "dwarf2reader" ||     
         name == "arm_ex_to_module" || 
         name == "testing";            
}

bool isIgnoredPath(const Decl *decl) {
  decl = decl->getCanonicalDecl();
  SourceLocation Loc = decl->getLocation();
  const SourceManager &SM = decl->getASTContext().getSourceManager();
  SmallString<1024> FileName = SM.getFilename(Loc);
  llvm::sys::fs::make_absolute(FileName);
  llvm::sys::path::reverse_iterator begin = llvm::sys::path::rbegin(FileName),
                                    end   = llvm::sys::path::rend(FileName);
  for (; begin != end; ++begin) {
    if (begin->compare_lower(StringRef("skia")) == 0 ||
        begin->compare_lower(StringRef("angle")) == 0 ||
        begin->compare_lower(StringRef("harfbuzz")) == 0 ||
        begin->compare_lower(StringRef("hunspell")) == 0 ||
        begin->compare_lower(StringRef("scoped_ptr.h")) == 0 ||
        begin->compare_lower(StringRef("graphite2")) == 0) {
      return true;
    }
  }
  return false;
}

bool isInterestingDecl(const Decl *decl) {
  return !isInIgnoredNamespace(decl) &&
         !isIgnoredPath(decl);
}

}

class MozChecker : public ASTConsumer, public RecursiveASTVisitor<MozChecker> {
  DiagnosticsEngine &Diag;
  const CompilerInstance &CI;
  DiagnosticsMatcher matcher;
public:
  MozChecker(const CompilerInstance &CI) : Diag(CI.getDiagnostics()), CI(CI) {}

  ASTConsumerPtr getOtherConsumer() {
    return matcher.makeASTConsumer();
  }

  virtual void HandleTranslationUnit(ASTContext &ctx) {
    TraverseDecl(ctx.getTranslationUnitDecl());
  }

  static bool hasCustomAnnotation(const Decl *d, const char *spelling) {
    AnnotateAttr *attr = d->getAttr<AnnotateAttr>();
    if (!attr)
      return false;

    return attr->getAnnotation() == spelling;
  }

  bool VisitCXXRecordDecl(CXXRecordDecl *d) {
    
    if (!d->isThisDeclarationADefinition()) return true;

    
    
    typedef std::vector<CXXMethodDecl *> OverridesVector;
    OverridesVector must_overrides;
    for (CXXRecordDecl::base_class_iterator base = d->bases_begin(),
         e = d->bases_end(); base != e; ++base) {
      
      CXXRecordDecl *parent = base->getType()
        .getDesugaredType(d->getASTContext())->getAsCXXRecordDecl();
      
      
      
      
      if (!parent) {
        continue;
      }
      parent = parent->getDefinition();
      for (CXXRecordDecl::method_iterator M = parent->method_begin();
          M != parent->method_end(); ++M) {
        if (hasCustomAnnotation(*M, "moz_must_override"))
          must_overrides.push_back(*M);
      }
    }

    for (OverridesVector::iterator it = must_overrides.begin();
        it != must_overrides.end(); ++it) {
      bool overridden = false;
      for (CXXRecordDecl::method_iterator M = d->method_begin();
          !overridden && M != d->method_end(); ++M) {
        
        
        if (M->getName() == (*it)->getName() &&
            !CI.getSema().IsOverload(*M, (*it), false)) {
          overridden = true;
          break;
        }
      }
      if (!overridden) {
        unsigned overrideID = Diag.getDiagnosticIDs()->getCustomDiagID(
            DiagnosticIDs::Error, "%0 must override %1");
        unsigned overrideNote = Diag.getDiagnosticIDs()->getCustomDiagID(
            DiagnosticIDs::Note, "function to override is here");
        Diag.Report(d->getLocation(), overrideID) << d->getDeclName() <<
          (*it)->getDeclName();
        Diag.Report((*it)->getLocation(), overrideNote);
      }
    }

    if (isInterestingDecl(d)) {
      for (CXXRecordDecl::ctor_iterator ctor = d->ctor_begin(),
           e = d->ctor_end(); ctor != e; ++ctor) {
        
        if (!ctor->isConvertingConstructor(false)) {
          continue;
        }
        
        if (ctor->isCopyOrMoveConstructor()) {
          continue;
        }
        
        if (ctor->isDeleted()) {
          continue;
        }
        
        if (MozChecker::hasCustomAnnotation(*ctor, "moz_implicit")) {
          continue;
        }
        unsigned ctorID = Diag.getDiagnosticIDs()->getCustomDiagID(
          DiagnosticIDs::Error, "bad implicit conversion constructor for %0");
        Diag.Report(ctor->getLocation(), ctorID) << d->getDeclName();
      }
    }

    return true;
  }
};






enum ClassAllocationNature {
  RegularClass = 0,
  NonHeapClass = 1,
  StackClass = 2
};



DenseMap<const CXXRecordDecl *,
  std::pair<const Decl *, ClassAllocationNature> > inferredAllocCauses;

ClassAllocationNature getClassAttrs(QualType T);

ClassAllocationNature getClassAttrs(CXXRecordDecl *D) {
  
  
  if (!D->hasDefinition())
    return RegularClass;
  D = D->getDefinition();
  
  if (MozChecker::hasCustomAnnotation(D, "moz_stack_class"))
    return StackClass;

  
  DenseMap<const CXXRecordDecl *,
    std::pair<const Decl *, ClassAllocationNature> >::iterator it =
    inferredAllocCauses.find(D);
  if (it != inferredAllocCauses.end()) {
    return it->second.second;
  }

  
  
  ClassAllocationNature type = RegularClass;
  if (MozChecker::hasCustomAnnotation(D, "moz_nonheap_class")) {
    type = NonHeapClass;
  }
  inferredAllocCauses.insert(std::make_pair(D,
    std::make_pair((const Decl *)0, type)));

  
  
  
  for (CXXRecordDecl::base_class_iterator base = D->bases_begin(),
       e = D->bases_end(); base != e; ++base) {
    ClassAllocationNature super = getClassAttrs(base->getType());
    if (super == StackClass) {
      inferredAllocCauses[D] = std::make_pair(
        base->getType()->getAsCXXRecordDecl(), StackClass);
      return StackClass;
    } else if (super == NonHeapClass) {
      inferredAllocCauses[D] = std::make_pair(
        base->getType()->getAsCXXRecordDecl(), NonHeapClass);
      type = NonHeapClass;
    }
  }

  
  for (RecordDecl::field_iterator field = D->field_begin(), e = D->field_end();
       field != e; ++field) {
    ClassAllocationNature fieldType = getClassAttrs(field->getType());
    if (fieldType == StackClass) {
      inferredAllocCauses[D] = std::make_pair(*field, StackClass);
      return StackClass;
    } else if (fieldType == NonHeapClass) {
      inferredAllocCauses[D] = std::make_pair(*field, NonHeapClass);
      type = NonHeapClass;
    }
  }

  return type;
}

ClassAllocationNature getClassAttrs(QualType T) {
  while (const ArrayType *arrTy = T->getAsArrayTypeUnsafe())
    T = arrTy->getElementType();
  CXXRecordDecl *clazz = T->getAsCXXRecordDecl();
  return clazz ? getClassAttrs(clazz) : RegularClass;
}

}

namespace clang {
namespace ast_matchers {



AST_MATCHER(QualType, stackClassAggregate) {
  return getClassAttrs(Node) == StackClass;
}



AST_MATCHER(QualType, nonheapClassAggregate) {
  return getClassAttrs(Node) == NonHeapClass;
}



AST_MATCHER(FunctionDecl, heapAllocator) {
  return MozChecker::hasCustomAnnotation(&Node, "moz_heap_allocator");
}
}
}

namespace {

bool isPlacementNew(const CXXNewExpr *expr) {
  
  if (expr->getNumPlacementArgs() == 0)
    return false;
  if (MozChecker::hasCustomAnnotation(expr->getOperatorNew(),
      "moz_heap_allocator"))
    return false;
  return true;
}

DiagnosticsMatcher::DiagnosticsMatcher() {
  
  
  astMatcher.addMatcher(varDecl(hasType(stackClassAggregate())).bind("node"),
    &stackClassChecker);
  
  astMatcher.addMatcher(newExpr(hasType(pointerType(
      pointee(stackClassAggregate())
    ))).bind("node"), &stackClassChecker);
  
  
  astMatcher.addMatcher(newExpr(hasType(pointerType(
      pointee(nonheapClassAggregate())
    ))).bind("node"), &nonheapClassChecker);

  
  
  astMatcher.addMatcher(callExpr(callee(functionDecl(allOf(heapAllocator(),
      returns(pointerType(pointee(nonheapClassAggregate()))))))).bind("node"),
    &nonheapClassChecker);
  astMatcher.addMatcher(callExpr(callee(functionDecl(allOf(heapAllocator(),
      returns(pointerType(pointee(stackClassAggregate()))))))).bind("node"),
    &stackClassChecker);
}

void DiagnosticsMatcher::StackClassChecker::run(
    const MatchFinder::MatchResult &Result) {
  DiagnosticsEngine &Diag = Result.Context->getDiagnostics();
  unsigned stackID = Diag.getDiagnosticIDs()->getCustomDiagID(
    DiagnosticIDs::Error, "variable of type %0 only valid on the stack");
  if (const VarDecl *d = Result.Nodes.getNodeAs<VarDecl>("node")) {
    
    if (d->hasLocalStorage())
      return;

    Diag.Report(d->getLocation(), stackID) << d->getType();
    noteInferred(d->getType(), Diag);
  } else if (const CXXNewExpr *expr =
      Result.Nodes.getNodeAs<CXXNewExpr>("node")) {
    
    if (isPlacementNew(expr))
      return;
    Diag.Report(expr->getStartLoc(), stackID) << expr->getAllocatedType();
    noteInferred(expr->getAllocatedType(), Diag);
  } else if (const CallExpr *expr =
      Result.Nodes.getNodeAs<CallExpr>("node")) {
    QualType badType = expr->getCallReturnType()->getPointeeType();
    Diag.Report(expr->getLocStart(), stackID) << badType;
    noteInferred(badType, Diag);
  }
}

void DiagnosticsMatcher::StackClassChecker::noteInferred(QualType T,
    DiagnosticsEngine &Diag) {
  unsigned inheritsID = Diag.getDiagnosticIDs()->getCustomDiagID(
    DiagnosticIDs::Note,
    "%0 is a stack class because it inherits from a stack class %1");
  unsigned memberID = Diag.getDiagnosticIDs()->getCustomDiagID(
    DiagnosticIDs::Note,
    "%0 is a stack class because member %1 is a stack class %2");

  
  while (const ArrayType *arrTy = T->getAsArrayTypeUnsafe())
    T = arrTy->getElementType();
  CXXRecordDecl *clazz = T->getAsCXXRecordDecl();

  
  if (MozChecker::hasCustomAnnotation(clazz, "moz_stack_class"))
    return;

  const Decl *cause = inferredAllocCauses[clazz].first;
  if (const CXXRecordDecl *CRD = dyn_cast<CXXRecordDecl>(cause)) {
    Diag.Report(clazz->getLocation(), inheritsID) << T << CRD->getDeclName();
  } else if (const FieldDecl *FD = dyn_cast<FieldDecl>(cause)) {
    Diag.Report(FD->getLocation(), memberID) << T << FD << FD->getType();
  }
  
  
  noteInferred(cast<ValueDecl>(cause)->getType(), Diag);
}

void DiagnosticsMatcher::NonHeapClassChecker::run(
    const MatchFinder::MatchResult &Result) {
  DiagnosticsEngine &Diag = Result.Context->getDiagnostics();
  unsigned stackID = Diag.getDiagnosticIDs()->getCustomDiagID(
    DiagnosticIDs::Error, "variable of type %0 is not valid on the heap");
  if (const CXXNewExpr *expr = Result.Nodes.getNodeAs<CXXNewExpr>("node")) {
    
    if (isPlacementNew(expr))
      return;
    Diag.Report(expr->getStartLoc(), stackID) << expr->getAllocatedType();
    noteInferred(expr->getAllocatedType(), Diag);
  } else if (const CallExpr *expr = Result.Nodes.getNodeAs<CallExpr>("node")) {
    QualType badType = expr->getCallReturnType()->getPointeeType();
    Diag.Report(expr->getLocStart(), stackID) << badType;
    noteInferred(badType, Diag);
  }
}

void DiagnosticsMatcher::NonHeapClassChecker::noteInferred(QualType T,
    DiagnosticsEngine &Diag) {
  unsigned inheritsID = Diag.getDiagnosticIDs()->getCustomDiagID(
    DiagnosticIDs::Note,
    "%0 is a non-heap class because it inherits from a non-heap class %1");
  unsigned memberID = Diag.getDiagnosticIDs()->getCustomDiagID(
    DiagnosticIDs::Note,
    "%0 is a non-heap class because member %1 is a non-heap class %2");

  
  while (const ArrayType *arrTy = T->getAsArrayTypeUnsafe())
    T = arrTy->getElementType();
  CXXRecordDecl *clazz = T->getAsCXXRecordDecl();

  
  if (MozChecker::hasCustomAnnotation(clazz, "moz_nonheap_class"))
    return;

  const Decl *cause = inferredAllocCauses[clazz].first;
  if (const CXXRecordDecl *CRD = dyn_cast<CXXRecordDecl>(cause)) {
    Diag.Report(clazz->getLocation(), inheritsID) << T << CRD->getDeclName();
  } else if (const FieldDecl *FD = dyn_cast<FieldDecl>(cause)) {
    Diag.Report(FD->getLocation(), memberID) << T << FD << FD->getType();
  }
  
  
  noteInferred(cast<ValueDecl>(cause)->getType(), Diag);
}

class MozCheckAction : public PluginASTAction {
public:
  ASTConsumerPtr CreateASTConsumer(CompilerInstance &CI, StringRef fileName) override {
#if CLANG_VERSION_FULL >= 306
    std::unique_ptr<MozChecker> checker(make_unique<MozChecker>(CI));

    std::vector<std::unique_ptr<ASTConsumer>> consumers;
    consumers.push_back(std::move(checker));
    consumers.push_back(checker->getOtherConsumer());
    return make_unique<MultiplexConsumer>(std::move(consumers));
#else
    MozChecker *checker = new MozChecker(CI);

    ASTConsumer *consumers[] = { checker, checker->getOtherConsumer() };
    return new MultiplexConsumer(consumers);
#endif
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};
}

static FrontendPluginRegistry::Add<MozCheckAction>
X("moz-check", "check moz action");
