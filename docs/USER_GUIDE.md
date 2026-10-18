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

## 4. Stop Nudi

To stop using Nudi, switch back to your normal keyboard layout from the
desktop input-source switcher. To stop the IBus session completely:

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

You can now switch keyboard layouts with the normal GNOME input-source switcher.

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

This is useful for checking composition behavior before typing in other apps.

## 8. Troubleshooting

The editor is not a login autostart application. If Ubuntu restores it after
reboot or shows a crash dialog, let the desktop finish starting IBus, clear
“Relaunch this application”, dismiss the dialog, and open the editor from the
application menu. Do not run it with `sudo`.

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

## 9. Notes

This project is released as a Debian package for Ubuntu. The latest release asset is the easiest way to install and use it. The local `.deb` flow is mainly for development and testing.
