executable(
  'run_hscroll',
  'run_hscroll.cxx',
  objects: ncmpc.extract_objects(
    'src/BasicMarquee.cxx',
    'src/charset.cxx',
  ),
  include_directories: inc,
  dependencies: [
    glib_dep,
  ],
)
