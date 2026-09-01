# Development / Test Build

Use `Makefile-manual` for building the kernel module on the development host:

```bash
make -f Makefile-manual
scp sysmon.ko <user>@<raspberrypi4-ip-addr>:/tmp/
```

On the target run:
```bash
insmod /tmp/sysmon.ko
dmesg | tail -n10        # check kernel logs
```
