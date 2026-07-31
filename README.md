# Embedded Linux System Monitoring Appliance on Raspberry Pi 4 Model B

## Project Overview
Please check the Wiki: [Project Overview Page](https://github.com/cu-ecen-aeld/final-project-halder/wiki/Project-Overview)

## Build Configuration
To restore the project configuration:

```bash
make configs/finalproject_wifi_working_rpi_defconfig
make
```

### Configure WiFi
Copy example `wpa_supplicant.conf` into roofs overlay:

```bash
cp board/raspberrypi4-64/examples/wpa_supplicant.conf.example \
   board/raspberrypi4-64/rootfs-overlay/etc/wpa_supplicant.conf
```

Replace WiFi credentials:

```bash
sed -i -e 's/YOUR_WIFI_NAME/MyActualNetworkName/' \
       -e 's/YOUR_WIFI_PASSWORD/MyActualWifiPassword/' \
       board/raspberrypi4-64/rootfs-overlay/etc/wpa_supplicant.conf
```

### Network Configuration Notes
The default network configuration enables both Ethernet (`eth0`) and WiFi (`wlan0`).

When both interfaces are connected to the same network, the system removes the duplicate Ethernet routes so that WiFi remains the preferred LAN interface. This avoids routing conflicts when accessing the device over WiFi while Ethernet is connected.
