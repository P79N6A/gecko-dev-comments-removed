



























#define nsHtml5HtmlAttributes_cpp__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsHtml5AtomTable.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsTraceRefcnt.h"
#include "jArray.h"
#include "nsHtml5ArrayCopy.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5Atoms.h"
#include "nsHtml5ByteReadable.h"
#include "nsIUnicodeDecoder.h"
#include "nsHtml5Macros.h"

#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5MetaScanner.h"
#include "nsHtml5AttributeName.h"
#include "nsHtml5ElementName.h"
#include "nsHtml5StackNode.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5StateSnapshot.h"
#include "nsHtml5Portability.h"

#include "nsHtml5HtmlAttributes.h"

nsHtml5HtmlAttributes* nsHtml5HtmlAttributes::EMPTY_ATTRIBUTES = nullptr;

nsHtml5HtmlAttributes::nsHtml5HtmlAttributes(int32_t mode)
  : mode(mode),
    length(0),
    names(jArray<nsHtml5AttributeName*,int32_t>::newJArray(5)),
    values(jArray<nsString*,int32_t>::newJArray(5))
{
  MOZ_COUNT_CTOR(nsHtml5HtmlAttributes);
}


nsHtml5HtmlAttributes::~nsHtml5HtmlAttributes()
{
  MOZ_COUNT_DTOR(nsHtml5HtmlAttributes);
  clear(0);
}

int32_t 
nsHtml5HtmlAttributes::getIndex(nsHtml5AttributeName* name)
{
  for (int32_t i = 0; i < length; i++) {
    if (names[i] == name) {
      return i;
    }
  }
  return -1;
}

int32_t 
nsHtml5HtmlAttributes::getLength()
{
  return length;
}

nsIAtom* 
nsHtml5HtmlAttributes::getLocalName(int32_t index)
{
  if (index < length && index >= 0) {
    return names[index]->getLocal(mode);
  } else {
    return nullptr;
  }
}

nsHtml5AttributeName* 
nsHtml5HtmlAttributes::getAttributeName(int32_t index)
{
  if (index < length && index >= 0) {
    return names[index];
  } else {
    return nullptr;
  }
}

int32_t 
nsHtml5HtmlAttributes::getURI(int32_t index)
{
  if (index < length && index >= 0) {
    return names[index]->getUri(mode);
  } else {
    return 0;
  }
}

nsIAtom* 
nsHtml5HtmlAttributes::getPrefix(int32_t index)
{
  if (index < length && index >= 0) {
    return names[index]->getPrefix(mode);
  } else {
    return nullptr;
  }
}

nsString* 
nsHtml5HtmlAttributes::getValue(int32_t index)
{
  if (index < length && index >= 0) {
    return values[index];
  } else {
    return nullptr;
  }
}

nsString* 
nsHtml5HtmlAttributes::getValue(nsHtml5AttributeName* name)
{
  int32_t index = getIndex(name);
  if (index == -1) {
    return nullptr;
  } else {
    return getValue(index);
  }
}

void 
nsHtml5HtmlAttributes::addAttribute(nsHtml5AttributeName* name, nsString* value)
{
  if (names.length == length) {
    int32_t newLen = length << 1;
    jArray<nsHtml5AttributeName*,int32_t> newNames = jArray<nsHtml5AttributeName*,int32_t>::newJArray(newLen);
    nsHtml5ArrayCopy::arraycopy(names, newNames, names.length);
    names = newNames;
    jArray<nsString*,int32_t> newValues = jArray<nsString*,int32_t>::newJArray(newLen);
    nsHtml5ArrayCopy::arraycopy(values, newValues, values.length);
    values = newValues;
  }
  names[length] = name;
  values[length] = value;
  length++;
}

void 
nsHtml5HtmlAttributes::clear(int32_t m)
{
  for (int32_t i = 0; i < length; i++) {
    names[i]->release();
    names[i] = nullptr;
    nsHtml5Portability::releaseString(values[i]);
    values[i] = nullptr;
  }
  length = 0;
  mode = m;
}

void 
nsHtml5HtmlAttributes::releaseValue(int32_t i)
{
  nsHtml5Portability::releaseString(values[i]);
}

void 
nsHtml5HtmlAttributes::clearWithoutReleasingContents()
{
  for (int32_t i = 0; i < length; i++) {
    names[i] = nullptr;
    values[i] = nullptr;
  }
  length = 0;
}

bool 
nsHtml5HtmlAttributes::contains(nsHtml5AttributeName* name)
{
  for (int32_t i = 0; i < length; i++) {
    if (name->equalsAnother(names[i])) {
      return true;
    }
  }
  return false;
}

void 
nsHtml5HtmlAttributes::adjustForMath()
{
  mode = NS_HTML5ATTRIBUTE_NAME_MATHML;
}

void 
nsHtml5HtmlAttributes::adjustForSvg()
{
  mode = NS_HTML5ATTRIBUTE_NAME_SVG;
}

nsHtml5HtmlAttributes* 
nsHtml5HtmlAttributes::cloneAttributes(nsHtml5AtomTable* interner)
{

  nsHtml5HtmlAttributes* clone = new nsHtml5HtmlAttributes(0);
  for (int32_t i = 0; i < length; i++) {
    clone->addAttribute(names[i]->cloneAttributeName(interner), nsHtml5Portability::newStringFromString(values[i]));
  }
  return clone;
}

bool 
nsHtml5HtmlAttributes::equalsAnother(nsHtml5HtmlAttributes* other)
{

  int32_t otherLength = other->getLength();
  if (length != otherLength) {
    return false;
  }
  for (int32_t i = 0; i < length; i++) {
    bool found = false;
    nsIAtom* ownLocal = names[i]->getLocal(NS_HTML5ATTRIBUTE_NAME_HTML);
    for (int32_t j = 0; j < otherLength; j++) {
      if (ownLocal == other->names[j]->getLocal(NS_HTML5ATTRIBUTE_NAME_HTML)) {
        found = true;
        if (!nsHtml5Portability::stringEqualsString(values[i], other->values[j])) {
          return false;
        }
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

void
nsHtml5HtmlAttributes::initializeStatics()
{
  EMPTY_ATTRIBUTES = new nsHtml5HtmlAttributes(NS_HTML5ATTRIBUTE_NAME_HTML);
}

void
nsHtml5HtmlAttributes::releaseStatics()
{
  delete EMPTY_ATTRIBUTES;
}


