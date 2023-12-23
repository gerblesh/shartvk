project(
    'shartvk',
    ['c'],
    version : '0.0.1',
    license : 'MIT',
    meson_version : '>= 0.56',
    default_options : [
        'c_std=gnu11',
        'buildtype=debug',
        'default_library=static'
    ]
)

cc = meson.get_compiler('c')

# Add the cglm library

cglm_dep = dependency('cglm')
sdl_dep = dependency('sdl2')
vulkan_dep = dependency('vulkan')

# Add the executable
executable_name = 'shartvk'
executable = executable(
  executable_name,
  'src/main.c',
  dependencies : [cglm_dep, sdl_dep, vulkan_dep],
  install : true
)

# Set the output directory to ${PROJECTROOT}/target
install_dir = join_paths(meson.source_root(), 'target')

