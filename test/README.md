# Sysmon Kernel Module Manual Test Scripts

These are minimal scripts for manual testing of the sysmon kernel module functionality. They are only intended for making sure the kernel module works as intended, they are neither complete in terms of cases nor do they use any kind of test framework.

In order to run the tests, `scp` the compiled files onto the Raspberry Pi 4 target with the `sysmon` kernel module running:
```bash
make
scp test_sysmon_data test_ioctl test_poll test_dashboard_load <user>@<raspberrypi4-ip-addr>:/tmp/
```

Then on the Pi run:
```
./tmp/test_sysmon_data
./tmp/test_ioctl
./tmp/test_poll
./tmp/test_dashboard_load
```

Verify the test script output against the kernel messages (or check the web app dashboard for `test_dashboard_load`):
```
dmesg | tail -n10
```
