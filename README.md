# Pixelspam

Small PoC spammer for [pixelflut](https://github.com/defnull/pixelflut)
compatible servers.

Usually, people write python scripts for spamming the server. Those scripts
usually open a connection and start sending commands to the server, which are
formatted on the fly. Even for static images, most don't even buffer commands
albeit they are, well, static. How incredibly inefficient.

Thus, at [GPN 2018](https://entropia.de/GPN18) I started writing `pixelspam`.
In ugly C. Without even trying to compile it after the event was over. Why?
Because I couldn't motivate myself to do something more productive. The next,
year, at [GPN 2019](https://entropia.de/GPN19) I wrote `pngspam`, which lets
you spam a static PNG. At a private session at the end of 2021, I wrote
`pixelflame`, yet another spammer.

The overall "theme" for the spammers is filling a buffer and sending that out
multiple times via `writev`. We also want to keep it simple and bare-bones.


## Building

The `Makefile` provides the `pixelspam`, `pngspam` and `pixelflame` targets for
the spammers. The default rule builds them all.

`pixelspam` and `pixelflame` require pthreads and the standard math library.
`pngspam` requires pthreads and `libpng` (1.6 or something). I also used some
gcc-extensions. Probably.


## Usage

    pixelspam host port [x [y [w [h]]]]
    pngspam host port file [x y]
    pixelflame host port [particles]

