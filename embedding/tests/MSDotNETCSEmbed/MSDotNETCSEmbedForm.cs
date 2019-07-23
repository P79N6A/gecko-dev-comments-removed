






























using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;


using Mozilla.Embedding;

namespace MSDotNETCSEmbed
{
	
	
	
	public class MSDotNETCSEmbedForm : System.Windows.Forms.Form
	{
		
		
		
		private System.ComponentModel.Container components = null;
		private Mozilla.Embedding.Gecko gecko1;
		private System.Windows.Forms.Button goButton;
		private System.Windows.Forms.TextBox urlBar;

		public MSDotNETCSEmbedForm()
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
			this.gecko1 = new Mozilla.Embedding.Gecko();
			this.goButton = new System.Windows.Forms.Button();
			this.urlBar = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			
			
			
			this.gecko1.Location = new System.Drawing.Point(0, 40);
			this.gecko1.Name = "gecko1";
			this.gecko1.Size = new System.Drawing.Size(664, 392);
			this.gecko1.TabIndex = 0;
			
			
			
			this.goButton.Location = new System.Drawing.Point(600, 8);
			this.goButton.Name = "goButton";
			this.goButton.Size = new System.Drawing.Size(56, 24);
			this.goButton.TabIndex = 1;
			this.goButton.Text = "Go";
			this.goButton.Click += new System.EventHandler(this.goButton_Click);
			
			
			
			this.urlBar.Location = new System.Drawing.Point(8, 10);
			this.urlBar.Name = "urlBar";
			this.urlBar.Size = new System.Drawing.Size(576, 20);
			this.urlBar.TabIndex = 2;
			this.urlBar.Text = "";
			this.urlBar.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.urlBar_KeyPress);
			
			
			
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(664, 429);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.urlBar,
																		  this.goButton,
																		  this.gecko1});
			this.Name = "MSDotNETCSEmbedForm";
			this.Text = "MSDotNETCSEmbed [UNSUPPORTED]";
			this.Resize += new System.EventHandler(this.MSDotNETCSEmbedForm_Resize);
			this.Load += new System.EventHandler(this.MSDotNETCSEmbedForm_Load);
			this.ResumeLayout(false);

		}
		#endregion

		
		
		
		[STAThread]
		static void Main() 
		{
			Application.Run(new MSDotNETCSEmbedForm());

			
			Gecko.TermEmbedding();
		}

		private void MSDotNETCSEmbedForm_Load(object sender, System.EventArgs e)
		{
			urlBar.Text = "http:
			gecko1.OpenURL(urlBar.Text);
			this.Text = "MSDotNETCSEmbed [UNSUPPORTED] - " + urlBar.Text;
		}

		private void MSDotNETCSEmbedForm_Resize(object sender, System.EventArgs e)
		{
			gecko1.Size =
				new Size(ClientSize.Width,
						 ClientSize.Height - gecko1.Location.Y);
		}

		private void urlBar_KeyPress(object sender, System.Windows.Forms.KeyPressEventArgs e)
		{
			switch (e.KeyChar)
			{
				case '\r':
					gecko1.OpenURL(urlBar.Text);
					e.Handled = true;
					break;
			}
		}

		private void goButton_Click(object sender, System.EventArgs e)
		{
			gecko1.OpenURL(urlBar.Text);
		}
	}
}
