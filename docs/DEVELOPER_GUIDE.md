# Nudi Developer Guide

## Architecture

The project has three deliberately small layers:

- `src/core/nudi_composer.hpp/.cpp` contains the Unicode mapping and composition
  state machine. It has no GTK, Qt, Wayland, or IBus dependency.
- `src/platform/linux/ibus_engine.cpp` is the IBus adapter. It translates key events into
  composer calls, updates preedit text, and commits Unicode to the focused
  application.
- `src/platform/linux/nudi_editor.cpp` is the GTK editor. It starts/selects
  Nudi when opened and restores the previous IBus engine when its last window
  closes.
- `com.kagapa.Nudi.xml` registers the engine with the user's IBus session.

IBus is the integration boundary for ONLYOFFICE and other GTK/Qt applications.
The engine must not install a global keyboard hook or inject synthetic key
events: those approaches are unreliable or blocked under Wayland.

## Mapping source of truth

`docs/win-nudi-keyboard.ahk` is the reference implementation. When porting a rule:

1. Record the AHK hotkey, condition, emitted code points, and state changes.
2. Add the smallest equivalent mapping/state transition in
   `src/core/nudi_composer.cpp`.
3. Add a C++ assertion in `tests/nudi_composer_test.cpp` or a case in
   `tests/nudi_test_cases.cpp` for the sequence.
4. Verify the result with code-point-aware tools, not only visual glyph output.

Important control characters are ZWNJ (`U+200C`), ZWJ (`U+200D`), and Kannada
virama (`U+0CCD`). Keep them explicit in tests because font shaping can make
different sequences look similar.

## Dependencies

### End users

End users do not need C++, CMake, headers, or development tools. Debian-based
users download the `.deb` release and install it with Ubuntu's package manager:

```sh
sudo apt install ./kannada-nudi_1.0.9_amd64.deb
ibus-daemon --panel disable --xim --daemonize
```

Fedora and other RPM-based users download the `.rpm` release artifact and install it with:

```sh
sudo dnf install ./kannada-nudi-1.0.9_x86_64.rpm
ibus-daemon --panel disable --xim --daemonize
```

The package declares only runtime requirements. Ubuntu resolves the linked
system libraries automatically, and `ibus` supplies the user-session service.

### Developers

Developers need the compiler, CMake, GTK3 and IBus headers, GLib headers,
packaging tools, and the IBus runtime:

```sh
sudo apt update
sudo apt install build-essential cmake debhelper-compat \
  libglib2.0-dev libgtk-3-dev libibus-1.0-dev pkg-config ibus
```

## Build and test

Install the native dependencies:

```sh
sudo apt install build-essential cmake libglib2.0-dev libgtk-3-dev \
  libibus-1.0-dev pkg-config
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Build a local package:

```sh
dpkg-buildpackage -us -uc -b
```

For local development, use the Linux build script below. It builds the project,
runs the tests, creates and installs the `.deb`, restarts IBus, waits for the
engine to register, and selects Nudi:

```sh
./build-linux-package.sh
```

The script also supports focused commands:

```sh
./build-linux-package.sh test
./build-linux-package.sh clean
./build-linux-package.sh uninstall
./build-linux-package.sh reinstall
```

The package and related Debian artifacts are written to the repository's
`output/` directory. The script requires the current user to have `sudo` access
and must not be run with `sudo` itself.
Inspect the package's runtime dependencies if needed:

```sh
dpkg-deb -I output/kannada-nudi_1.0.9_amd64.deb | sed -n '/Package:/p;/Version:/p;/Architecture:/p;/Depends:/p'
```

## Updating the keyboard

1. Update `src/core/nudi_composer.cpp` using `docs/win-nudi-keyboard.ahk` as the source of
  truth. Keep Unicode code points explicit for ZWNJ, ZWJ, virama, and combining
  marks.
2. Add a regression assertion in `tests/nudi_composer_test.cpp` or a case in
   `tests/nudi_test_cases.cpp` for every new key
  sequence or modifier layer.
3. For normal development pushes to `main`, no version edits are required. The
  GitHub Actions release workflow increments the patch number, updates
  `debian/changelog`, `CMakeLists.txt`, and `com.kagapa.Nudi.xml`, then creates
  the matching tag and `.deb` release asset.
4. Run the complete local checks, or use `./build-linux-package.sh` for the full local
  build-and-run flow:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
git diff --check
```

Do not commit the `build/` directory or generated `.deb` files.

## Publishing for Ubuntu users

The repository workflow builds on Ubuntu 24.04 and publishes a release for
every push to `main`. Before pushing:

```sh
git status
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
git diff --check
git add .
git commit -m "Describe the Nudi change"
git push origin main
```

The workflow finds the latest `vMAJOR.MINOR.PATCH` tag, increments `PATCH`,
updates all release metadata, builds `kannada-nudi_<version>_amd64.deb`, creates
the matching tag, and attaches the package to the GitHub Release. Do not create
or push the release tag manually.

For a manual release trigger, use GitHub Actions → Linux Debian Package Release
→ Run workflow. A manually triggered workflow only builds the package; the
release publication job runs for pushes to `main`.

## Publishing to an APT repository

GitHub Releases do not provide an APT repository. To let Ubuntu users install
Nudi with `sudo apt install kannada-nudi`, publish the package through a PPA or
another APT repository. A Launchpad PPA is the simplest option for Ubuntu.

### One-time Launchpad setup

1. Create a PPA on Launchpad, for example `ppa:YOUR_LAUNCHPAD_ID/nudi`.
2. Configure a GPG key for uploading packages to Launchpad.
3. Install the packaging tools:

```sh
sudo apt update
sudo apt install devscripts debhelper dput
```

### Upload each release

The package version in `debian/changelog` must be higher than the version
already in the PPA. The GitHub workflow generates its changelog in the build
runner and does not commit that temporary file to `main`, so prepare the PPA
source package from the matching release tag:

```sh
git pull origin main
git fetch --tags
git checkout <release-tag>
version=<version>
dch --newversion "$version" --distribution unstable "Release v$version for PPA."
sed -i -E "s/(project\\(kannada-nudi VERSION )[0-9]+\\.[0-9]+\\.[0-9]+/\\1${version}/" CMakeLists.txt
sed -i -E "s#(<version>)[0-9]+\\.[0-9]+\\.[0-9]+(</version>)#\\1${version}\\2#" com.kagapa.Nudi.xml
dpkg-buildpackage -S -sa
dput ppa:YOUR_LAUNCHPAD_ID/nudi ../kannada-nudi_${version}_source.changes
```

Replace `<release-tag>` and `<version>` with the actual release values, such
as `v1.0.2` and `1.0.2`. These commands modify the detached local checkout only;
do not commit or push those generated PPA metadata changes. Wait for Launchpad
to finish building the package for the required Ubuntu series before asking
users to install it. Check the build status on the PPA page and confirm the
package is published.

### User installation from the PPA

Users add the PPA once, then install or upgrade Nudi normally:

```sh
sudo add-apt-repository ppa:YOUR_LAUNCHPAD_ID/nudi
sudo apt update
sudo apt install kannada-nudi
```

Future releases only require:

```sh
sudo apt update
sudo apt upgrade kannada-nudi
```

Users can verify the repository and available version with:

```sh
apt-cache policy kannada-nudi
```

For a private or non-Launchpad deployment, publish the `.deb` and its APT
metadata through a signed repository, then give users the repository setup
command followed by `sudo apt install kannada-nudi`. Do not distribute the
build directory or compiler dependencies to end users.

## Runtime debugging

Check that IBus sees the component:

```sh
ibus list-engine | grep -i nudi
ibus restart
```

Use `ibus engine nudi` to select it temporarily. Confirm the active desktop
session is using IBus before debugging the engine itself:

```sh
echo "$XDG_SESSION_TYPE"
echo "$GTK_IM_MODULE/$QT_IM_MODULE"
```

The runtime default is `ಅ`, which supports compact input such as `knk` for
`ಕನಕ`. The editor automatically starts/selects Nudi on open. To test the
explicit-vowel reference mode, start the engine or editor
with `NUDI_DEFAULT_VOWEL=virama`; then `kanaka` produces `ಕನಕ`:

```sh
NUDI_DEFAULT_VOWEL=virama ./build/nudi-editor
NUDI_DEFAULT_VOWEL=virama ibus-daemon --panel disable --xim --replace --daemonize
```

The package installs the engine in `/usr/libexec`, the editor in `/usr/bin`,
component metadata in `/usr/share/ibus/component`, the desktop entry in
`/usr/share/applications`, and icons in the hicolor icon directories. IBus
launches the engine process on demand as a user-session background service.

## Adding full parity

The remaining context-sensitive AHK behavior is concentrated in shifted `f`
variants and special `x` sequences. The direct CapsLock, ScrollLock, Kannada
digit, shifted-letter, vowel-sign, and core conjunct mappings are represented in
the native composer. Implement further rules as explicit composer modes instead
of reproducing the AHK global flag list. Keep
commit boundaries observable: separators should commit pending preedit and then
be passed through to the application.
