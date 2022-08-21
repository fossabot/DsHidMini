using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace ControlApp.UI.Devices
{
    public partial class DeviceDetailsView : UserControl
    {
        public DeviceDetailsView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
