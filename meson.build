project('guest-networkizer', 'c')
cc = meson.get_compiler('c')
gnome = import('gnome')

dep_virtqemu = dependency('libvirt-qemu') # TODO Make optionnal dependency

gresources = gnome.compile_resources(
	'guest_networkizer', 'gresource.xml',
	c_name: 'guest_networkizer_gres'
)

srcs = [
	'src/gn-main-window.c',
	'src/gn-main-window-add-link-dialog.c',
	'src/gn-net.c',
	'src/gn-node.c',
	'src/gn-ports.c',
	'src/main.c',
	'src/vde-slirp.c',
	'src/vde-switch.c',
	'src/vir-domain-list.c',
	'src/vir-node.c',
	gresources,
]

deps = [
	dependency('gtk+-3.0'),
	dependency('libvirt-gobject-1.0'),
	cc.find_library('m', required: false),
	dep_virtqemu,
]

executable('guest-networkizer', srcs, export_dynamic: true, dependencies: deps)
