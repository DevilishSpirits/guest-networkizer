# Quick and dirty network builder for QEMU/KVM

**WARNING!!!** It's extremely drafty and have a **BUNCH** of TODO. It may look fine in the front but it's still buggy and very incomplete.

This software allow you to build simple networks of libvirt VMs, switchs (future:and real-world access) using VDE and QEMU/KVM. It's much simpler than GNS3 and is sufficient in my use case, it's somewhat like Marionnet (but still with less features...).

**This is really quick&dirty**, it can be seen as a virt-manager addon but that's just a GUI to hack it in a dirty and unsupported way.

To use it create your VMs in the virt-manager (or any libvirt client), remove networks interfaces if you don't want libvirtd networking stack and use this app to network VMs using VDE.

**Warning!** Only i440FX chipset are supported, Q35 chipset does not support PCIe hotplugging and this one is the default in virt-manager.

**Note!** I use `virDomainQemuMonitorCommand` to perform some black magic that may crash libvirt (especially a future VDE enabled libvirt...).

**This link won't break** Restart the software to delete links

## Installation 

This project use the [Meson Build System](https://mesonbuild.com/).

One-liner installation (if you are happy to mess your system) :
```sh
	meson build && ninja -C build && sudo ninja -C build install
```

If you don't want to mess your system, skip installation and run the built exe at `build/guest-networkizer`.
