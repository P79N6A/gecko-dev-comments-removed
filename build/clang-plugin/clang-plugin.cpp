


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

#define CLANG_VERSION_FULL (CLANG_VERSION_MAJOR * 100 + CLANG_VERSION_MINOR)

using namespace llvm;
using namespace clang;

namespace {

using namespace clang::ast_matchers;
class DiagnosticsMatcher {
public:
  DiagnosticsMatcher();

  ASTConsumer *makeASTConsumer() {
    return astMatcher.newASTConsumer();
  }

private:
  class StackClassChecker : public MatchFinder::MatchCallback {
  public:
    virtual void run(const MatchFinder::MatchResult &Result);
  };

  StackClassChecker stackClassChecker;
  MatchFinder astMatcher;
};

class MozChecker : public ASTConsumer, public RecursiveASTVisitor<MozChecker> {
  DiagnosticsEngine &Diag;
  const CompilerInstance &CI;
  DiagnosticsMatcher matcher;
public:
  MozChecker(const CompilerInstance &CI) : Diag(CI.getDiagnostics()), CI(CI) {}

  ASTConsumer *getOtherConsumer() {
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

      
      
      if (hasCustomAnnotation(parent, "moz_stack_class") &&
          !hasCustomAnnotation(d, "moz_stack_class")) {
        unsigned badInheritID = Diag.getDiagnosticIDs()->getCustomDiagID(
            DiagnosticIDs::Error, "%0 inherits from a stack class %1");
        Diag.Report(d->getLocation(), badInheritID)
          << d->getDeclName() << parent->getDeclName();
      }
    }

    for (OverridesVector::iterator it = must_overrides.begin();
        it != must_overrides.end(); ++it) {
      bool overridden = false;
      for (CXXRecordDecl::method_iterator M = d->method_begin();
          !overridden && M != d->method_end(); ++M) {
        
        
        if (M->getName() == (*it)->getName() &&
            !CI.getSema().IsOverload(*M, (*it), false))
          overridden = true;
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
    return true;
  }
};
}

namespace clang {
namespace ast_matchers {



AST_MATCHER(QualType, stackClassAggregate) {
  QualType t = Node;
  while (const ArrayType *arrTy = t->getAsArrayTypeUnsafe())
    t = arrTy->getElementType();
  CXXRecordDecl *clazz = t->getAsCXXRecordDecl();
  return clazz && MozChecker::hasCustomAnnotation(clazz, "moz_stack_class");
}
}
}

namespace {

DiagnosticsMatcher::DiagnosticsMatcher() {
  
  
  astMatcher.addMatcher(varDecl(hasType(stackClassAggregate())).bind("node"),
    &stackClassChecker);
  
  astMatcher.addMatcher(newExpr(hasType(pointerType(
      pointee(stackClassAggregate())
    ))).bind("node"), &stackClassChecker);
  
  
  astMatcher.addMatcher(fieldDecl(hasType(stackClassAggregate())).bind("field"),
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
  } else if (const CXXNewExpr *expr =
      Result.Nodes.getNodeAs<CXXNewExpr>("node")) {
    
    if (expr->getNumPlacementArgs() > 0)
      return;
    Diag.Report(expr->getStartLoc(), stackID) << expr->getAllocatedType();
  } else if (const FieldDecl *field =
      Result.Nodes.getNodeAs<FieldDecl>("field")) {
    
    const RecordDecl *parent = field->getParent();
    if (!MozChecker::hasCustomAnnotation(parent, "moz_stack_class")) {
      
      unsigned stackID = Diag.getDiagnosticIDs()->getCustomDiagID(
        DiagnosticIDs::Error,
        "member of type %0 in class %1 that is not a stack class");
      Diag.Report(field->getLocation(), stackID) << field->getType() <<
        parent->getDeclName();
    }
  }
}

class MozCheckAction : public PluginASTAction {
public:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, StringRef fileName) {
    MozChecker *checker = new MozChecker(CI);

    ASTConsumer *consumers[] = { checker, checker->getOtherConsumer() };
    return new MultiplexConsumer(consumers);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) {
    return true;
  }
};
}

static FrontendPluginRegistry::Add<MozCheckAction>
X("moz-check", "check moz action");
