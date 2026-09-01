# Development / Test Build

Use `Makefile-manual` for building the sysmon daemon on the development host:

```bash
make -f Makefile-manual
scp sysmond <user>@<raspberrypi4-ip-addr>:/tmp/
```

On the target run:
```bash
./tmp/sysmond
ps aux | grep sysmond
```
