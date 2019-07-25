




































function createEmptyGroupItem(contentWindow, width, height, padding, noAnimation) {
  let pageBounds = contentWindow.Items.getPageBounds();
  pageBounds.inset(padding, padding);

  let box = new contentWindow.Rect(pageBounds);
  box.width = width;
  box.height = height;
  
  let immediately = noAnimation ? true: false;
  let emptyGroupItem = 
    new contentWindow.GroupItem([], { bounds: box, immediately: immediately });

  return emptyGroupItem;
}
