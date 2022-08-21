using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace ControlApp.UI.Settings
{
    public partial class DsHidMiniControlSettingsView : UserControl
    {
        public DsHidMiniControlSettingsView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
