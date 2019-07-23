









namespace LzmaAlone.Properties
{
	using System;
	using System.IO;
	using System.Resources;

	
	
	
	
	
	
	
	class Resources
	{

		private static System.Resources.ResourceManager _resMgr;

		private static System.Globalization.CultureInfo _resCulture;

		
		internal Resources()
		{
		}

		
		
		
		[System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
		public static System.Resources.ResourceManager ResourceManager
		{
			get
			{
				if ((_resMgr == null))
				{
					System.Resources.ResourceManager temp = new System.Resources.ResourceManager("Resources", typeof(Resources).Assembly);
					_resMgr = temp;
				}
				return _resMgr;
			}
		}

		
		
		
		
		[System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
		public static System.Globalization.CultureInfo Culture
		{
			get
			{
				return _resCulture;
			}
			set
			{
				_resCulture = value;
			}
		}
	}
}
