





































#include "nsMetricsEventItem.h"
#include "nsIPropertyBag.h"

nsMetricsEventItem::nsMetricsEventItem(const nsAString &itemNamespace,
                                       const nsAString &itemName)
    : mNamespace(itemNamespace), mName(itemName)
{
}

nsMetricsEventItem::~nsMetricsEventItem()
{
}

NS_IMPL_ISUPPORTS1(nsMetricsEventItem, nsIMetricsEventItem)

NS_IMETHODIMP
nsMetricsEventItem::GetItemNamespace(nsAString &result)
{
  result = mNamespace;
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::GetItemName(nsAString &result)
{
  result = mName;
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::GetProperties(nsIPropertyBag **aProperties)
{
  NS_IF_ADDREF(*aProperties = mProperties);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::SetProperties(nsIPropertyBag *aProperties)
{
  
  
  mProperties = do_QueryInterface(aProperties);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::ChildAt(PRInt32 index, nsIMetricsEventItem **result)
{
  NS_ENSURE_ARG_RANGE(index, 0, PRInt32(mChildren.Length()) - 1);

  NS_ADDREF(*result = mChildren[index]);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::IndexOf(nsIMetricsEventItem *item, PRInt32 *result)
{
  *result = PRInt32(mChildren.IndexOf(item));  
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::AppendChild(nsIMetricsEventItem *item)
{
  NS_ENSURE_ARG_POINTER(item);

  NS_ENSURE_TRUE(mChildren.AppendElement(item), NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::InsertChildAt(nsIMetricsEventItem *item, PRInt32 index)
{
  NS_ENSURE_ARG_POINTER(item);

  
  NS_ENSURE_ARG_RANGE(index, 0, PRInt32(mChildren.Length()));

  NS_ENSURE_TRUE(mChildren.InsertElementAt(index, item),
                 NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::RemoveChildAt(PRInt32 index)
{
  NS_ENSURE_ARG_RANGE(index, 0, PRInt32(mChildren.Length()) - 1);

  mChildren.RemoveElementAt(index);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::ReplaceChildAt(nsIMetricsEventItem *newItem, PRInt32 index)
{
  NS_ENSURE_ARG_POINTER(newItem);
  NS_ENSURE_ARG_RANGE(index, 0, PRInt32(mChildren.Length()) - 1);

  mChildren.ReplaceElementsAt(index, 1, newItem);
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::ClearChildren()
{
  mChildren.Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsMetricsEventItem::GetChildCount(PRInt32 *childCount)
{
  *childCount = PRInt32(mChildren.Length());
  return NS_OK;
}
