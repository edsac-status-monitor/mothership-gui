# mothership-gui
GUI for the mothership computer. This project is distributed under GPLv3.

**This is not suitable for connection to public networks**

GLib2.0 >= 2.32 is required as a dependency so that will need to be installed. On Debian this package is called libglib2.0-dev.
GTK3 >= 3.22.11 is is also required as a dependency. On debian this is called libgtk-3-dev
SQLite-3 >= 3.8.7

On debian & ubuntu the following packages are required to build mothership-gui:
```
gcc autoconf libglib2.0-dev libtool make libgtk-3-dev libsqlite3-dev
```

You will also need edsac-status-monitor/libnetworking to be installed. 

Compile using
```
autoreconf -i
./configure
make
```

Run unit tests using
```
make check
```

Clean up using
```
make distclean
```
