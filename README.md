# print_music

An openFrameworks app which generates a mesh from the FFT of an audio file.

The FFT spectrum values are calculated periodically and a new line of vertices is stitched onto the previous line. The current spectrum line turns about a central point resulting in the disc shape whilst making sound and time into a tangible volume.

To run this app clone it first and build with openFrameworks. Then copy the ````settings.xml```` file to a file called ````local.settings.xml```` and enter the path to a source audio file. The length of the track in seconds should also be entered in the specified field.

Pressing the 'm' key whilst the visualisation is rendering will join up the mesh into a watertight whole and write a ````.ply```` file into the data directory.

[www.thingsbymatt.com/projects/print-music/](http://www.thingsbymatt.com/projects/print-music/)

## Images
[mttholl.tumblr.com](http://mttholl.tumblr.com/)

![printed noise](https://41.media.tumblr.com/40947fc44f13d6c8cabe33f62c8a9452/tumblr_np2yimSaH41tbt3dko1_540.png)

![Image from tumblr](https://40.media.tumblr.com/22be5c05aed0c2811b6aca0c16264bc6/tumblr_nion4bSsLF1rxgdjxo9_1280.png)

![Image from tumblr](https://41.media.tumblr.com/4da61fd6f5a29dff20f7d57d7c7b1ae1/tumblr_nion4bSsLF1rxgdjxo8_1280.png)

![Image from tumblr](https://41.media.tumblr.com/b81723ed93e7b5879d8999b8bb755851/tumblr_nion4bSsLF1rxgdjxo5_1280.png)

![Image from tumblr](https://36.media.tumblr.com/32f63435a58e2bd5e40f7b333d4beddd/tumblr_nion4bSsLF1rxgdjxo4_1280.png)

![Image from tumblr](https://41.media.tumblr.com/26e85c1a4960c6103a597deaa4eff6d1/tumblr_nion4bSsLF1rxgdjxo3_1280.png)

![Image from tumblr](https://36.media.tumblr.com/ced1b13c84188f2fa47e0d6c014bb853/tumblr_nion4bSsLF1rxgdjxo2_1280.png)

