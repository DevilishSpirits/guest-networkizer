# Quick and dirty network builder for QEMU/KVM

This software allow you to build simple networks of libvirt VMs, switchs (future:and real-world access) using VDE and QEMU/KVM. It's much simpler than GNS3 and is sufficient in my use case, it's somewhat like Marionnet (but still with less features...).

It is quick&dirty but might be useful for you, but keep in mind that it rely on a dirty `virDomainQemuMonitorCommand` based hack.

To use it create your VMs in the virt-manager (or any libvirt client), remove networks interfaces if you don't want libvirtd networking stack and use this app to network VMs using VDE.

**Warning!** Only i440FX chipset are supported, Q35 chipset does not support PCIe hotplugging and this one is the default in virt-manager.

**This link won't break** You need to remove nodes to delete links right now.

## Installation 

This project use the [Meson Build System](https://mesonbuild.com/). It require GTK3, `libvirt-glib-1.0` and a modern VDE installation.

One-liner installation (if you are happy to mess your system) :
```sh
	meson build && ninja -C build && sudo ninja -C build install
```

If you don't want to mess your system, skip installation and run the built exe at `build/guest-networkizer`.
