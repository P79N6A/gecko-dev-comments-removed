







































function redirect(aURL)
{
  SpecialPowers.loadURI(window, aURL + location.search,
                 null, null, null, null);
}
