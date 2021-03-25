# nm-toggle

The purpose of this tool is to be able to, through NetworkManager, completely shutdown all network devices --- and bring them back up when needed. I use ArchLinux with the i3 window manager, so the program is somewhat tied to them (particularly concerning `systemd`, and the shell, `bash` in my case).

(If you are wondering why would I want to toggle between on and offline like this, instead of being always online, it is because the internet is transformed into a gigantic weapon of mass distraction, being on the cross-hairs of which I usually prefer to avoid.)

### How it works

Needed packages:

~~~ {.text .numberLines}
# pacman -S networkmanager
# pacman -S network-manager-applet
~~~

The source consists of a a C source file, which produces an executable named `nm-toggle`. It takes two arguments `on|off`, the latter being the default (for when either no argument is provided, or an invalid argument is provided). For `off`, it disables wired and wireless interfaces, stops `NetworkManager`, **and then masks its service files**. This is to prevent it going up in the next reboot. For `on`, it does the opposite: unmask, start `NetworkManager`, and enable wired and wireless interfaces.

The executable should be stored in `/usr/local/bin`, which is virtually always a) on `$PATH` (put it there if it's not); and b) only writable by `root`. It only takes two short arguments -- so I allow it to run with password-less `sudo` privileges. If a remote attacker gains non-root access to my machine, this program only buys him the possibility of closing the network, which doesn't really help his case... (if he gains *root* access, I am toast anyway, `sudo` privileges or not).

For convenience, two `.bashrc` alias are provided (see below), and a `systemd` service file to ensure `NetworkManager` will not be automagically brought up in the next reboot.

### Setup

1. Setup/install `sudo` if you don't have it already.

1. Run `$ make`.

2. `$ sudo cp nm-toggle /usr/local/bin`

3. Run `visudo` and write this line:

~~~ {.text .numberLines}
your_username ALL=(ALL) NOPASSWD: /usr/local/bin/nm-toggle
~~~

If your default editor is `vi(m)`, save and exit by hitting `<Esc>:wq<Enter>`. Now the command can be run without requiring a password.

4. The following aliases might come useful (`bash` only, sorry!):

~~~ {.shell .numberLines}
alias online="sudo /usr/local/bin/nm-toggle on"
alias offline="sudo /usr/local/bin/nm-toggle off"
~~~

Remember to source `.bashrc` before using it.

5. To avoid `NetworkManager` automagically starting up on the next reboot, put the `NetworkManager-stop-n-mask.service` file in `/usr/lib/systemd/system`, and then do:

~~~ {.text .numberLines}
systemctl enable NetworkManager-stop-n-mask.service
~~~

You can additionally also do `start` (after doing `enable`) to test the service --- `NetworkManager` should be totally brought down.

Now at a terminal, just type `offline` to kill all connectivity. `$ online` brings everything back up. Note that it may take a few seconds for a wireless link to be (re-)established (ethernet is usually near-instantaneous).

And that's it. Enjoy the minimisation of your online time!

