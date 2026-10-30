# Kannada Nudi Linux Keyboard

This project brings the Kannada Nudi keyboard to Ubuntu through IBus. Once
installed, it works as a normal desktop input method in supported GTK and Qt
applications.

## License

Kannada Nudi is released under the GNU Affero General Public License version 3
only (AGPL-3.0-only). See [LICENSE](LICENSE) for the project notice and source
publication requirements.

## Download and install

1. Open the latest GitHub Release.
2. Download the package for your distribution:
	- Debian, Ubuntu, and Linux Mint: `kannada-nudi_1.0.9_amd64.deb`
	- Fedora and other RPM-based distributions: `kannada-nudi-1.0.9_x86_64.rpm`
3. If an older `kannada-nudi` package is still installed, remove it first:

```sh
sudo apt purge -y kannada-nudi
```

4. Install it:

```sh
sudo apt install ./kannada-nudi_1.0.9_amd64.deb
```

On Fedora or another RPM-based distribution, install the RPM with:

```sh
sudo dnf install ./kannada-nudi-1.0.9_x86_64.rpm
```

5. Ensure the user IBus daemon is running:

```sh
ibus-daemon --panel disable --xim --daemonize
```

If you already have a session running, you can also use:

```sh
ibus restart
```

Select Nudi in the current IBus session:

```sh
ibus engine nudi
```

6. Confirm the engine is available:

```sh
ibus list-engine | grep -i nudi
```

You should see output similar to:

```text
nudi - Nudi Kannada
```

## Switch between Nudi and English

The Nudi Editor starts Nudi automatically. While the editor is focused, press
**Win+Space** to switch between Nudi Kannada and the US English keyboard.
The same toggle is available from the Engine menu. This changes the active
IBus engine; it does not stop the shared IBus session. When the last editor
window closes, the editor restores the engine that was active before launch.

On Linux desktops that reserve Win+Space for switching input sources, the
desktop may handle the shortcut before the editor sees it. Ubuntu GNOME
reserves it by default. Change the “Switch to next input source” shortcut in
Settings → Keyboard → Keyboard Shortcuts → Typing to a different key
combination (or disable it) to use Win+Space in the editor.

To select Nudi manually outside the editor:

```sh
ibus-daemon --panel disable --xim --daemonize
ibus engine nudi
```

To select US English manually:

```sh
ibus engine xkb:us::eng
```

To stop the IBus session completely:

```sh
ibus exit
```

Runtime diagnostics are written to the per-user XDG state directory at
`~/.local/state/kannada-nudi/YYYY-MM-DD/`. Each desktop user has a separate
`engine-UID.log` file; the logs record engine activity and key metadata, but
not composed Kannada text.

To inspect the latest log:

```sh
ls -ltr ~/.local/state/kannada-nudi/"$(date +%F)"
tail -f ~/.local/state/kannada-nudi/"$(date +%F)"/engine-"$(id -u)".log
```

## Use it in Ubuntu

Open:

Settings → Keyboard → Input Sources

Then:

1. Click the `+` button
2. Choose `Kannada`
3. Select `Nudi Kannada`
4. Start typing in Kannada

This works in supported Linux apps as a standard IBus input method.

## Reboot and session restore

The Nudi Editor is a normal desktop application, not a system service or
login autostart program. When the editor opens, it automatically tries to
start/select the Nudi engine. If IBus is still starting, it retries in the
background. Win+Space toggles between Nudi and US English. When the last
editor window closes, it restores the IBus engine that was active before Nudi;
it does not stop the shared IBus daemon.

If Ubuntu offers to relaunch the editor after a crash or restores it after
reboot, let the desktop finish starting. To stop a repeated crash loop, clear
the crash dialog's “Relaunch this application” option, dismiss it, and start
`Kannada Nudi Editor` again from the application menu when needed.

If the editor still fails to open, collect the session diagnostics and start
it from a terminal:

```sh
ibus status
G_MESSAGES_DEBUG=all nudi-editor 2>nudi-editor.log
```

Do not run the editor with `sudo`; IBus belongs to the logged-in desktop user.

## Local package install

If you are testing a local build instead of a GitHub release, install it the same way:

```sh
sudo apt install ./kannada-nudi_1.0.9_amd64.deb
ibus-daemon --panel disable --xim --daemonize
```

## Launch the built-in editor

You can also launch the included editor for testing the keyboard logic directly:

```sh
nudi-editor
```

The editor starts Nudi automatically and provides a tabbed UTF-8 text editor
for validating keyboard behavior.

For the complete user walkthrough and troubleshooting steps, see [docs/USER_GUIDE.md](docs/USER_GUIDE.md).
For release, RPM, dependency, and artifact guidance, see [docs/PACKAGING.md](docs/PACKAGING.md).

## Developer build

Install build dependencies and build the package:

```sh
sudo apt install build-essential cmake debhelper-compat \
	libglib2.0-dev libgtk-3-dev libibus-1.0-dev pkg-config ibus
./build-linux-package.sh
```

The script also supports the following development commands:

```sh
./build-linux-package.sh test       # build from the current tree and run CTest
./build-linux-package.sh clean      # remove build/ and output/
./build-linux-package.sh uninstall  # purge the installed Debian package
./build-linux-package.sh reinstall  # uninstall, clean, rebuild, test, install
```

The `test` command runs every test registered with CTest, including the
composer tests and the IBus registration contract check.

On Windows, use PowerShell to build and test the console executable:

```powershell
.\build-windows.ps1 -Configuration Release -Clean
```

The Windows script writes `build-artifacts/kannada-nudi-windows-Release.zip`,
containing `nudi-windows.exe` and the Windows composer test executable.

`win-nudi-keyboard.ahk` remains the behavior reference. The Linux port currently
covers the base consonants, independent vowels, vowel signs, virama/conjunct
composition, backspace, separator handling, lock-key layers, Kannada digits,
special signs, and the main conjunct forms. The remaining context-sensitive
shifted `f` variants and special `x` sequences should be ported as focused
tests before declaring full parity.
