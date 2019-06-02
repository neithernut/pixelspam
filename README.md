# Pixelspam

Small PoC spammer for [pixelflut](https://github.com/defnull/pixelflut)
compatible servers.

Usually, people write python scripts for spamming the server. Those scripts
usually open a connection and start sending commands to the server, which are
formatted on the fly. Even for static images, most don't even buffer commands
albeit they are, well static. How incredibly inefficient.

Thus, at [GPN 2018](https://entropia.de/GPN18) I started writing `pixelspam`.
In ugly C. Without even trying to compile it after the event was over. Why?
Because I couldn't motivate myself to do something more productive. The next,
year, at [GPN 2019](https://entropia.de/GPN19) I wrote `pngspam`, which lets
you spam a static PNG.


## Building

The `Makefile` provides the `pixelspam` and `pngspam` targets for the spammers.
The default rule builds both of them.

`pixelspam` requires pthreads and the standard math library. `pngspam` requires
`libpng` (1.6 or something). I also used some gcc-extensions. Probably.


## Usage

    pixelspam host port [x [y [w [h]]]]
    pngspam host port file [x y]

