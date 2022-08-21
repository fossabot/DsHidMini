using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace ControlApp.UI.Settings
{
    public partial class DsHidMiniDriverSettingsView : UserControl
    {
        public DsHidMiniDriverSettingsView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
