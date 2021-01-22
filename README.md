# Turnips-Save-Manager

A save manager for Animal Crossing: New Horizons.

DISLAIMER: This reads and writes data from your save. I am not responsible for any data loss, consider backing up before use.

# Compiling

Building needs a working devkitA64 environment, with packages `libnx`,`deko3d` and `switch-glm` installed (`sudo (dkp-)pacman -S switch-dev`).

```
$ git clone --recursive https://github.com/jeliebig/Turnips-Save-Manager.git
$ cd Turnips
$ make -j$(nproc)
```

Output will be located in out/.

# Credits

- [ReSwitched Discord](https://discord.gg/ZdqEhed) for helping me a lot with debugging and answering my (noob) questions.
- [averne](https://github.com/averne/Turnips) for Turnips, the application all of this is based on.
- The [NHSE](https://github.com/kwsch/NHSE) project for save decrypting/parsing and offsets.
- The [FTPD](https://github.com/mtheall/ftpd) project for the deko3d imgui backend.
- [u/Kyek](https://reddit.com/u/Kyek) for the background and logo assets.
