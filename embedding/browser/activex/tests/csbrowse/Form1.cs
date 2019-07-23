using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

namespace CSBrowse
{
	
	
	
	public class Form1 : System.Windows.Forms.Form
	{
		private AxMOZILLACONTROLLib.AxMozillaBrowser axMozillaBrowser1;
		private System.Windows.Forms.StatusBar statusBar1;
		private System.Windows.Forms.Button goBtn;
		private System.Windows.Forms.StatusBarPanel statusMessagePane;
		private System.Windows.Forms.ToolBar toolBar1;
		private System.Windows.Forms.ToolBarButton btnBack;
		private System.Windows.Forms.ToolBarButton btnForward;
		private System.Windows.Forms.StatusBarPanel statusProgressPane;
		private System.Windows.Forms.ImageList imageList1;
		private System.Windows.Forms.ToolBarButton btnSep1;
		private System.Windows.Forms.ToolBarButton btnStop;
		private System.Windows.Forms.ToolBarButton btnReload;
		private System.Windows.Forms.ToolBarButton btnHome;
		private System.Windows.Forms.MainMenu mainMenu1;
		private System.Windows.Forms.MenuItem menuItem1;
		private System.Windows.Forms.MenuItem menuExit;
		private System.Windows.Forms.MenuItem menuItem2;
		private System.Windows.Forms.MenuItem menuAbout;
		private System.Windows.Forms.ComboBox url;
		private System.Windows.Forms.ToolBarButton btnSep2;
		private System.ComponentModel.IContainer components;

		public Form1()
		{
			
			
			
			InitializeComponent();

			
			
			
		}

		
		
		
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		
		
		
		
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(Form1));
			this.axMozillaBrowser1 = new AxMOZILLACONTROLLib.AxMozillaBrowser();
			this.goBtn = new System.Windows.Forms.Button();
			this.statusBar1 = new System.Windows.Forms.StatusBar();
			this.statusMessagePane = new System.Windows.Forms.StatusBarPanel();
			this.statusProgressPane = new System.Windows.Forms.StatusBarPanel();
			this.toolBar1 = new System.Windows.Forms.ToolBar();
			this.btnBack = new System.Windows.Forms.ToolBarButton();
			this.btnForward = new System.Windows.Forms.ToolBarButton();
			this.btnSep1 = new System.Windows.Forms.ToolBarButton();
			this.btnStop = new System.Windows.Forms.ToolBarButton();
			this.btnReload = new System.Windows.Forms.ToolBarButton();
			this.btnHome = new System.Windows.Forms.ToolBarButton();
			this.imageList1 = new System.Windows.Forms.ImageList(this.components);
			this.mainMenu1 = new System.Windows.Forms.MainMenu();
			this.menuItem1 = new System.Windows.Forms.MenuItem();
			this.menuExit = new System.Windows.Forms.MenuItem();
			this.menuItem2 = new System.Windows.Forms.MenuItem();
			this.menuAbout = new System.Windows.Forms.MenuItem();
			this.url = new System.Windows.Forms.ComboBox();
			this.btnSep2 = new System.Windows.Forms.ToolBarButton();
			((System.ComponentModel.ISupportInitialize)(this.axMozillaBrowser1)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.statusMessagePane)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.statusProgressPane)).BeginInit();
			this.SuspendLayout();
			
			
			
			this.axMozillaBrowser1.Enabled = true;
			this.axMozillaBrowser1.Location = new System.Drawing.Point(24, 112);
			this.axMozillaBrowser1.OcxState = ((System.Windows.Forms.AxHost.State)(resources.GetObject("axMozillaBrowser1.OcxState")));
			this.axMozillaBrowser1.Size = new System.Drawing.Size(640, 400);
			this.axMozillaBrowser1.TabIndex = 3;
			this.axMozillaBrowser1.DownloadComplete += new System.EventHandler(this.axMozillaBrowser1_DownloadComplete);
			this.axMozillaBrowser1.StatusTextChange += new AxMOZILLACONTROLLib.DWebBrowserEvents2_StatusTextChangeEventHandler(this.axMozillaBrowser1_StatusTextChange);
			this.axMozillaBrowser1.NavigateComplete2 += new AxMOZILLACONTROLLib.DWebBrowserEvents2_NavigateComplete2EventHandler(this.axMozillaBrowser1_NavigateComplete2);
			this.axMozillaBrowser1.CommandStateChange += new AxMOZILLACONTROLLib.DWebBrowserEvents2_CommandStateChangeEventHandler(this.axMozillaBrowser1_CommandStateChange);
			this.axMozillaBrowser1.LocationChanged += new System.EventHandler(this.axMozillaBrowser1_LocationChanged);
			this.axMozillaBrowser1.BeforeNavigate2 += new AxMOZILLACONTROLLib.DWebBrowserEvents2_BeforeNavigate2EventHandler(this.axMozillaBrowser1_BeforeNavigate2);
			this.axMozillaBrowser1.NewWindow2 += new AxMOZILLACONTROLLib.DWebBrowserEvents2_NewWindow2EventHandler(this.axMozillaBrowser1_NewWindow2);
			this.axMozillaBrowser1.DownloadBegin += new System.EventHandler(this.axMozillaBrowser1_DownloadBegin);
			
			
			
			this.goBtn.Location = new System.Drawing.Point(616, 72);
			this.goBtn.Name = "goBtn";
			this.goBtn.Size = new System.Drawing.Size(48, 21);
			this.goBtn.TabIndex = 4;
			this.goBtn.Text = "Go";
			this.goBtn.Click += new System.EventHandler(this.button1_Click);
			
			
			
			this.statusBar1.Location = new System.Drawing.Point(0, 539);
			this.statusBar1.Name = "statusBar1";
			this.statusBar1.Panels.AddRange(new System.Windows.Forms.StatusBarPanel[] {
																						  this.statusMessagePane,
																						  this.statusProgressPane});
			this.statusBar1.ShowPanels = true;
			this.statusBar1.Size = new System.Drawing.Size(696, 22);
			this.statusBar1.TabIndex = 5;
			this.statusBar1.PanelClick += new System.Windows.Forms.StatusBarPanelClickEventHandler(this.statusBar1_PanelClick);
			this.statusBar1.DrawItem += new System.Windows.Forms.StatusBarDrawItemEventHandler(this.statusBar1_DrawItem);
			
			
			
			this.statusMessagePane.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Spring;
			this.statusMessagePane.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
			this.statusMessagePane.Width = 580;
			
			
			
			this.statusProgressPane.Style = System.Windows.Forms.StatusBarPanelStyle.OwnerDraw;
			this.statusProgressPane.Text = "statusBarPanel1";
			
			
			
			this.toolBar1.Appearance = System.Windows.Forms.ToolBarAppearance.Flat;
			this.toolBar1.Buttons.AddRange(new System.Windows.Forms.ToolBarButton[] {
																						this.btnBack,
																						this.btnForward,
																						this.btnSep1,
																						this.btnReload,
																						this.btnStop,
																						this.btnSep2,
																						this.btnHome});
			this.toolBar1.ButtonSize = new System.Drawing.Size(32, 32);
			this.toolBar1.DropDownArrows = true;
			this.toolBar1.ImageList = this.imageList1;
			this.toolBar1.Name = "toolBar1";
			this.toolBar1.ShowToolTips = true;
			this.toolBar1.Size = new System.Drawing.Size(696, 55);
			this.toolBar1.TabIndex = 6;
			this.toolBar1.ButtonClick += new System.Windows.Forms.ToolBarButtonClickEventHandler(this.toolBar1_ButtonClick);
			
			
			
			this.btnBack.Enabled = false;
			this.btnBack.ImageIndex = 0;
			this.btnBack.Style = System.Windows.Forms.ToolBarButtonStyle.DropDownButton;
			this.btnBack.Tag = "Back";
			this.btnBack.Text = "Back";
			this.btnBack.ToolTipText = "Back one page";
			
			
			
			this.btnForward.Enabled = false;
			this.btnForward.ImageIndex = 1;
			this.btnForward.Style = System.Windows.Forms.ToolBarButtonStyle.DropDownButton;
			this.btnForward.Text = "Forward";
			this.btnForward.ToolTipText = "Forward one page";
			
			
			
			this.btnSep1.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			
			
			
			this.btnStop.Enabled = false;
			this.btnStop.ImageIndex = 4;
			this.btnStop.Text = "Stop";
			this.btnStop.ToolTipText = "Stop loading";
			
			
			
			this.btnReload.ImageIndex = 3;
			this.btnReload.Text = "Reload";
			this.btnReload.ToolTipText = "Reload this page";
			
			
			
			this.btnHome.ImageIndex = 2;
			this.btnHome.Text = "Home";
			this.btnHome.ToolTipText = "Go Home";
			
			
			
			this.imageList1.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
			this.imageList1.ImageSize = new System.Drawing.Size(32, 32);
			this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
			this.imageList1.TransparentColor = System.Drawing.Color.Black;
			
			
			
			this.mainMenu1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.menuItem1,
																					  this.menuItem2});
			
			
			
			this.menuItem1.Index = 0;
			this.menuItem1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.menuExit});
			this.menuItem1.Text = "&File";
			
			
			
			this.menuExit.Index = 0;
			this.menuExit.Text = "E&xit";
			this.menuExit.Click += new System.EventHandler(this.menuExit_Click);
			
			
			
			this.menuItem2.Index = 1;
			this.menuItem2.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.menuAbout});
			this.menuItem2.Text = "&Help";
			
			
			
			this.menuAbout.Index = 0;
			this.menuAbout.Text = "&About...";
			this.menuAbout.Click += new System.EventHandler(this.menuAbout_Click);
			
			
			
			this.url.Items.AddRange(new object[] {
													 "http:
													 "http:
													 "http:
													 "http:
													 "http:
													 "http:
													 "http:
			this.url.Location = new System.Drawing.Point(24, 72);
			this.url.MaxLength = 1024;
			this.url.Name = "url";
			this.url.Size = new System.Drawing.Size(568, 21);
			this.url.TabIndex = 0;
			
			
			
			this.btnSep2.Style = System.Windows.Forms.ToolBarButtonStyle.Separator;
			
			
			
			this.AcceptButton = this.goBtn;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(696, 561);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.url,
																		  this.toolBar1,
																		  this.statusBar1,
																		  this.goBtn,
																		  this.axMozillaBrowser1});
			this.Menu = this.mainMenu1;
			this.Name = "Form1";
			this.Text = "CSBrowse";
			this.Load += new System.EventHandler(this.Form1_Load);
			this.Layout += new System.Windows.Forms.LayoutEventHandler(this.Form1_Layout);
			((System.ComponentModel.ISupportInitialize)(this.axMozillaBrowser1)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.statusMessagePane)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.statusProgressPane)).EndInit();
			this.ResumeLayout(false);

		}
		#endregion

		
		
		
		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}

		private void Form1_Load(object sender, System.EventArgs e)
		{
				
		}

		private void button1_Click(object sender, System.EventArgs e)
		{
			object n = null;
			axMozillaBrowser1.Navigate(url.Text, ref n, ref n, ref n, ref n);
		}

		private void statusBar1_PanelClick(object sender, System.Windows.Forms.StatusBarPanelClickEventArgs e)
		{
		
		}
		
		private void toolBar1_ButtonClick(object sender, System.Windows.Forms.ToolBarButtonClickEventArgs e)
		{
			if (e.Button.Equals(btnBack))
			{
				axMozillaBrowser1.GoBack();
			}
			else if (e.Button.Equals(btnForward))
			{
				axMozillaBrowser1.GoForward();
			}
			else if (e.Button.Equals(btnStop))
			{
				axMozillaBrowser1.Stop();
			}
			else if (e.Button.Equals(btnReload))
			{
				axMozillaBrowser1.Refresh();
			}
			else if (e.Button.Equals(btnHome))
			{
				axMozillaBrowser1.GoHome();
			}
		}

		private void statusBar1_DrawItem(object sender, System.Windows.Forms.StatusBarDrawItemEventArgs sbdevent)
		{
			if (sbdevent.Panel.Equals(statusProgressPane))
			{

			}
		}

		private void axMozillaBrowser1_StatusTextChange(object sender, AxMOZILLACONTROLLib.DWebBrowserEvents2_StatusTextChangeEvent e)
		{
			statusMessagePane.Text = e.text;
		}

		private void axMozillaBrowser1_CommandStateChange(object sender, AxMOZILLACONTROLLib.DWebBrowserEvents2_CommandStateChangeEvent e)
		{
			if ((e.command & 0x1) != 0) 
			{
				btnForward.Enabled = e.enable;
			}
			if ((e.command & 0x2) != 0) 
			{
				btnBack.Enabled = e.enable;
			}
		}

		private void axMozillaBrowser1_NewWindow2(object sender, AxMOZILLACONTROLLib.DWebBrowserEvents2_NewWindow2Event e)
		{
			Form1 f = new Form1();
			f.Show();
			object n = null;
			f.axMozillaBrowser1.Navigate("about:blank", ref n, ref n, ref n, ref n);
			e.ppDisp = f.axMozillaBrowser1.Application;
		}

		private void axMozillaBrowser1_DownloadBegin(object sender, System.EventArgs e)
		{
			btnStop.Enabled = true;
		}

		private void axMozillaBrowser1_DownloadComplete(object sender, System.EventArgs e)
		{
			btnStop.Enabled = false;
		}

		private void axMozillaBrowser1_BeforeNavigate2(object sender, AxMOZILLACONTROLLib.DWebBrowserEvents2_BeforeNavigate2Event e)
		{
		}

		private void axMozillaBrowser1_NavigateComplete2(object sender, AxMOZILLACONTROLLib.DWebBrowserEvents2_NavigateComplete2Event e)
		{
			this.Text = axMozillaBrowser1.LocationName + " - CSBrowse";
			url.Text = axMozillaBrowser1.LocationURL;
		}

		private void axMozillaBrowser1_LocationChanged(object sender, System.EventArgs e)
		{
			url.Text = axMozillaBrowser1.LocationURL;
		}

		private void Form1_Layout(object sender, System.Windows.Forms.LayoutEventArgs e)
		{
			
			toolBar1.Location =
				new Point(this.ClientRectangle.Left, this.ClientRectangle.Top);
			
			goBtn.Location = new Point(
				this.ClientRectangle.Right - goBtn.Width,
				toolBar1.Left + toolBar1.Height);
			
			url.Location = new Point(
				this.ClientRectangle.Left,
				toolBar1.Left + toolBar1.Height);
			url.Size = new Size(
				goBtn.Left - url.Left, goBtn.Height);

			statusBar1.Location = new Point(
				this.ClientRectangle.Left,
				this.ClientRectangle.Bottom - statusBar1.Height);

			axMozillaBrowser1.Location = new Point(
				this.ClientRectangle.Left, url.Bottom);
			axMozillaBrowser1.Size = new Size(
				this.ClientSize.Width,
				statusBar1.Top - axMozillaBrowser1.Top);
		}

		private void url_TextChanged(object sender, System.EventArgs e)
		{
		
		}

		
		private void menuExit_Click(object sender, System.EventArgs e)
		{
			this.Close();
		}

		private void menuAbout_Click(object sender, System.EventArgs e)
		{
			
			MessageBox.Show(this, "CSBrowse Test Application", "CSBrowse");
		}


	}
}
