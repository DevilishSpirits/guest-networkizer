# Quick and dirty network builder for QEMU/KVM

**WARNING!!!** It's extremely drafty and have a **BUNCH** of TODO. It may look fine in the front but it's very fragil and incomplete.

This software allow you to build simple networks of libvirt VMs, switchs (future:and real-world access) using VDE and QEMU/KVM. It's much simpler than GNS3 and is sufficient in my use case, it's somewhat like Marionnet (but still with less features...).

**This is really quick&dirty**, it can be seen as a virt-manager addon but that's just a GUI to hack it in a dirty and unsupported way.

To use it create your VMs in the virt-manager (or any libvirt client), remove networks interfaces if you don't want libvirtd networking stack and use this app to network VMs using VDE.

**Note!** I use `virDomainQemuMonitorCommand` to perform some black magic that may crash libvirt (especially a future VDE enabled libvirt...).

**It's really bad!!!** To have it works, starts VM __before__ adding them in the app and never stop them (I'm rushing to have a at least a usable demo so I didn't made hotplug right now, but it's a must have !).

**I lost my crossed cables** So VM to switch, no VM to VM, nor switch to switch for now.

**I'm shameful 😭️** To have it works, link from the VM to the switch, not the reverse ! (Need to brain that more)

**This link won't break** Restart the software to delete links
