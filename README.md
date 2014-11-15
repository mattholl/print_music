# print_music

A small openFrameworks app which generates a mesh from the FFT of an audio file.

The FFT spectrum values are calculated periodically and a new line of vertices is stitched onto the previous, the line turns about a central point resulting in the disc shape.

The settings.xml file needs to be copied to a settings.local.xml file. The path to a music file can then be specified along with the length of the file in seconds.

A .ply file of the mesh can be dumped by pressing the 'm' key whilst the app is running. Images can be dumped with 's'.

# Todo
- [ ] Add images to this readme
- [ ] Add camera controls to readme
- [ ] Make the mesh watertight ready for printing