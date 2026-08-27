# Embedded Linux System Monitoring Appliance on Raspberry Pi 4 Model B

## Project Overview
Please check the Wiki: [Project Overview page](https://github.com/cu-ecen-aeld/final-project-halder/wiki/Project-Overview) and the [Project Schedule page](https://github.com/users/halder/projects/2/views/1?groupedBy%5BcolumnId%5D=369428154) for the current status of the project.

## Build Configuration
After cloning the repository, load the config, set your actual WiFi credentials and build the image.

```bash
make config     # restores config/raspberrypi4-64_sysmon_defconfig
```

### Configure WiFi

Copy example `wpa_supplicant.conf` into roofs overlay:

```bash
cp board/raspberrypi4-64/examples/wpa_supplicant.conf.example \
   board/raspberrypi4-64/rootfs-overlay/etc/wpa_supplicant.conf
```

Replace WiFi credentials:

```bash
sed -i -e 's/YOUR_WIFI_NAME/<MyActualNetworkName>/' \
       -e 's/YOUR_WIFI_PASSWORD/<MyActualWifiPassword>/' \
       board/raspberrypi4-64/rootfs-overlay/etc/wpa_supplicant.conf
```

### Build Image
```bash
make
``` 

## Flash Image Onto SD Card
After building the image, you need to flash it onto your micro SD card.

**1 Identify SD Card**
```bash
lsblk
```

Look for `sdb`, `sdc` or similar with the correct size.

**2 Unmount SD Card**
```bash
sudo umount sdX1    # X == whatever you identified to be your SD card
sudo umount sdX2
lsblk               # make sure the device is unmounted
```

**3 Flash Image**
```bash
sudo dd if=buildroot/output/images/sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

## Network Configuration Notes
The default network configuration enables both Ethernet (`eth0`) and WiFi (`wlan0`).

When both interfaces are connected to the same network, the system removes the duplicate Ethernet routes so that WiFi remains the preferred LAN interface. This avoids routing conflicts when accessing the device over WiFi while Ethernet is connected.

