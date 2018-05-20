# Pixelspam

Small PoC spammer for [pixelflut](https://github.com/defnull/pixelflut)
compatible servers.

Usually, people write python scripts for spamming the server. Those scripts
usually open a connection and start sending commands to the server, which are
formatted on the fly. Even for static images, most don't even buffer commands
albeit they are, well static. How incredibly inefficient.

Thus, at the [GPN 2018](https://entropia.de/GPN18) I started writing this little
thingy. In ugly C. Withough even trying to compile it after the event was over.
Why? Because I couldn't motivate myself to do something more productive.


## Building

Build like

    gcc -pthread -lm -lrt pixelspam.c -o pixelspam

Yeah, I used some gcc-extensions. Probably.


## Usage

    pixelspam host port [x [y [w [h]]]]

