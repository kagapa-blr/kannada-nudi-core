# Kannada Nudi Linux Keyboard User Guide

This guide explains how to download, install, enable, and use the Kannada Nudi keyboard on Ubuntu Linux.

## 1. Download the package

Go to the project's GitHub Releases page and download the package for your distribution. For example:

```sh
kannada-nudi_1.0.0_amd64.deb
```

For Fedora and other RPM-based distributions, download the corresponding:

```sh
kannada-nudi-1.0.0_x86_64.rpm
```

You can also use a local `.deb` file if you are testing a build on your machine.

## 2. Install the package

If an older legacy package called `kannada-nudi` is still installed, remove it first:

```sh
sudo apt purge -y kannada-nudi
```

Then install the downloaded file:

```sh
sudo apt install ./kannada-nudi_1.0.0_amd64.deb
```

On Fedora or another RPM-based distribution:

```sh
sudo dnf install ./kannada-nudi-1.0.0_x86_64.rpm
```

If the file is in another folder, use the full path instead:

```sh
sudo apt install /path/to/kannada-nudi_1.0.0_amd64.deb
```

## 3. Start or restart IBus

After installing, make sure the user IBus daemon is running in your session:

```sh
ibus-daemon --panel disable --xim --daemonize
```

If the daemon is already up, this is also safe:

```sh
ibus restart
```

Select Nudi in the current IBus session:

```sh
ibus engine nudi
```

Check that the engine is available:

```sh
ibus list-engine | grep -i nudi
```

You should see something like:

```text
nudi - Nudi Kannada
```

## 4. Switch between Nudi and English

The built-in editor starts Nudi automatically. While the editor is focused,
press **Win+Space** to switch between Nudi Kannada and the US English
keyboard. You can also use the toggle in the Engine menu. This changes the
active IBus engine, but keeps the shared IBus session running.

Some Linux desktops reserve Win+Space for their global input-source switcher
and may consume it before the editor receives it. Ubuntu GNOME reserves it by
default. To use Win+Space in the editor, change “Switch to next input source”
in Settings → Keyboard → Keyboard Shortcuts → Typing to a different key
combination, or disable that desktop shortcut.
To select US English manually:

```sh
ibus engine xkb:us::eng
```

To stop the IBus session completely:

```sh
ibus exit
```

Runtime diagnostics are written to `~/.local/state/kannada-nudi/YYYY-MM-DD/`.
Each desktop user has a separate `engine-UID.log` file; the logs record engine
activity and key metadata, but not composed Kannada text.

To inspect the latest log:

```sh
ls -ltr ~/.local/state/kannada-nudi/"$(date +%F)"
tail -f ~/.local/state/kannada-nudi/"$(date +%F)"/engine-"$(id -u)".log
```

## 5. Enable the keyboard in Ubuntu

Open:

Settings → Keyboard → Input Sources

Then:

1. Click the `+` button
2. Choose `Kannada`
3. Select `Nudi Kannada`
4. Confirm it is active

You can switch keyboard layouts with the normal GNOME input-source switcher.

## 6. Start typing in Kannada

Once `Nudi Kannada` is selected, you can type in Kannada in supported applications such as:

- GTK applications
- Qt applications
- many text editors and browser fields

The keyboard works as a standard IBus input method and does not require a global keyboard hook.

## 7. Open the built-in test editor

You can also launch the included editor for testing the composer directly:

```sh
nudi-editor
```

The editor automatically starts/selects Nudi when it opens. Press Win+Space
to toggle Nudi and US English while it is focused. When the last editor window
closes, it restores the IBus engine that was active previously. This is useful
for checking composition behavior before typing in other apps.

## 8. Troubleshooting

The editor is not a login autostart application. If Ubuntu restores it after
reboot or shows a crash dialog, let the desktop finish starting, clear
“Relaunch this application”, dismiss the dialog, and open the editor from the
application menu. The editor starts Nudi automatically and retries if IBus is
temporarily unavailable. Do not run it with `sudo`.

If the input method is not visible:

```sh
ibus restart
ibus list-engine | grep -i nudi
```

Then add it again from Settings → Keyboard → Input Sources.

If it still does not appear, reinstall the package:

```sh
sudo apt install --reinstall ./kannada-nudi_1.0.0_amd64.deb
ibus-daemon --panel disable --xim --daemonize
```

### Linux Mint: Nudi is selected but typing does not work

The editor needs the IBus GTK3 input-method bridge in addition to the Nudi
engine. The updated Nudi Kacheri Suite Debian package declares `ibus-gtk3` as
a dependency. If you installed an older package, install the bridge and
restart the application:

```sh
sudo apt install ibus-gtk3
```

Close every Nudi Kacheri Suite window, reopen it, and click **Start Nudi
Engine** again. The diagnostics should show `GTK=ibus` and
`GTK3 IBus bridge package: installed`. Then focus the document's text area
before typing. The “selected in IBus” status confirms engine selection only;
the engine log path in the dialog can help distinguish missing editor input
from an engine startup problem.

## 9. Notes

This project is released as a Debian package for Ubuntu. The latest release asset is the easiest way to install and use it. The local `.deb` flow is mainly for development and testing.
