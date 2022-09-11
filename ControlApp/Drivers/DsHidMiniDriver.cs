using System;
using System.ComponentModel;
using Nefarius.DsHidMini.Util.WPF;
using Nefarius.Utilities.DeviceManagement.PnP;

namespace Nefarius.DsHidMini.ControlApp.Drivers
{
    public static class DsHidMiniDriver
    {
        /// <summary>
        ///     Interface GUID common to all devices the DsHidMini driver supports.
        /// </summary>
        public static Guid DeviceInterfaceGuid => Guid.Parse("{399ED672-E0BD-4FB3-AB0C-4955B56FB86A}");

        #region Read-only properties

        /// <summary>
        ///     Unified Device Property exposing current battery status.
        /// </summary>
        public static DevicePropertyKey BatteryStatusProperty => CustomDeviceProperty.CreateCustomDeviceProperty(
            Guid.Parse("{3FECF510-CC94-4FBE-8839-738201F84D59}"), 2,
            typeof(byte));

        public static DevicePropertyKey LastPairingStatusProperty => CustomDeviceProperty.CreateCustomDeviceProperty(
            Guid.Parse("{3FECF510-CC94-4FBE-8839-738201F84D59}"), 3,
            typeof(int));

        #endregion
    }

    /// <summary>
    ///     Battery status values.
    /// </summary>
    [TypeConverter(typeof(EnumDescriptionTypeConverter))]
    public enum DsBatteryStatus : byte
    {
        [Description("Unknown")] Unknown = 0x00,
        [Description("Dying")] Dying = 0x01,
        [Description("Low")] Low = 0x02,
        [Description("Medium")] Medium = 0x03,
        [Description("High")] High = 0x04,
        [Description("Full")] Full = 0x05,
        [Description("Charging")] Charging = 0xEE,
        [Description("Charged")] Charged = 0xEF
    }

    /// <summary>
    ///     HID device emulation modes.
    /// </summary>
    [TypeConverter(typeof(EnumDescriptionTypeConverter))]
    public enum DsHidDeviceMode : byte
    {
        [Description("SDF (PCSX2)")] Single = 0x01,

        [Description("GPJ (Generic DirectInput)")]
        Multi = 0x02,
        [Description("SXS (Steam, RPCS3)")] SixaxisCompatible = 0x03,
        [Description("DS4Windows")] DualShock4Rev1Compatible = 0x04,
        [Description("XInput")] XInputHIDCompatible = 0x05
    }
}