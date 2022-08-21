using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace ControlApp.UI.Settings
{
    public partial class BthPS3DriverSettingsView : UserControl
    {
        public BthPS3DriverSettingsView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
