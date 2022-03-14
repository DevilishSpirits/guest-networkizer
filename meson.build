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
	'src/gn-net-load.c',
	'src/gn-net-save.c',
	'src/gn-node.c',
	'src/gn-node-panel.c',
	'src/gn-ports.c',
	'src/vde-ns.c',
	'src/vde-plug-url.c',
	'src/vde-plug-url-widget.c',
	'src/vde-slirp.c',
	'src/vde-slirp-widget.c',
	'src/vde-switch.c',
	'src/vir-domain-list.c',
	'src/vir-node.c',
	'src/vir-node-widget.c',
	gresources,
]

deps = [
	dependency('gtk+-3.0'),
	dependency('libvirt-gobject-1.0'),
	cc.find_library('m', required: false),
	dep_virtqemu,
]

# Static library to avoid rebuilding full source for each tests
exe_lib = static_library('guest-networkizer-lib', srcs, install: false, dependencies: deps)

executable('guest-networkizer', 'src/main.c', export_dynamic: true, install: true, link_whole: exe_lib, dependencies: deps)

test_lib = static_library('test_lib', 'tests/main.c', link_whole: exe_lib, install: false, dependencies: deps, include_directories: include_directories('src'), build_by_default: false)

test_dep = declare_dependency(link_whole: test_lib, dependencies: deps, include_directories: include_directories('src'))

basic_tests = [
	'node-detached-panel-ref-check',
]

test_env = {
	'G_DEBUG': 'fatal-criticals,fatal-warnings',
}
foreach test_unit: basic_tests
	test_prog = executable(test_unit, 'tests/'+test_unit+'.c', export_dynamic: true, install: false, dependencies: test_dep, build_by_default: false)
	test(test_unit, test_prog, env: test_env, protocol: 'tap')
endforeach

# Data files
install_data('data/me.d_spirits.guest_networkizer.desktop',install_dir: get_option('datadir')/'applications')
