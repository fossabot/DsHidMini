using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace ControlApp.UI.Devices
{
    public partial class DeviceListView : UserControl
    {
        public DeviceListView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
