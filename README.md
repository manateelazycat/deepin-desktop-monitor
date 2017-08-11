# Deepin Desktop Monitor

Deepin desktop monitor: a so cool desktop monitor.

## Dependencies

* sudo apt install libpcap-dev libncurses5-dev libprocps-dev libxtst-dev libxcb-util0-dev

## Installation

* mkdir build
* cd build
* qmake ..
* make
* sudo setcap cap_kill,cap_net_raw,cap_dac_read_search,cap_sys_ptrace+ep ./deepin-desktop-monitor

## Usage

* ./deepin-desktop-monitor

## Config file

* ~/.config/deepin/deepin-desktop-monitor/config.conf

## Getting help

Any usage issues can ask for help via

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org)
* [WiKi](http://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for users](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Users)
* [Contribution guide for developers](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Developers).

## License

Deepin Desktop Monitor is licensed under [GPLv3](LICENSE).
