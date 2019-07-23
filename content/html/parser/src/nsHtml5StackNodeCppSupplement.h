




































NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5StackNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(nsHtml5StackNode)
  NS_IF_RELEASE(tmp->node);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMETHODIMP                                                                
nsHtml5StackNode::NS_CYCLE_COLLECTION_INNERCLASS::Traverse
  (void *p, nsCycleCollectionTraversalCallback &cb)              
{                                                                            
  nsHtml5StackNode *tmp = static_cast<nsHtml5StackNode*>(p);                                     
  NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsHtml5StackNode, tmp->refcount)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(node);
  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsHtml5StackNode, retain)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsHtml5StackNode, release)
