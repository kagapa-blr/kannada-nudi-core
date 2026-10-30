# Packaging and publishing Kannada Nudi

This project publishes versioned Debian packages for Ubuntu/Debian, RPM
packages for Fedora-compatible distributions, and a Windows archive through
GitHub Releases. Linux packages contain the IBus engine, component metadata,
the `nudi-editor` application, its desktop entry, and icons. Build tools and
generated build directories are not runtime dependencies.

## License and source publication

This project is released under the GNU Affero General Public License version 3
only (AGPL-3.0-only). See [LICENSE](../LICENSE) for the project notice and the
Free Software Foundation's complete license text.

Every published source archive must contain the corresponding source for the
binary package, the license notice, copyright and attribution notices, and any
modifications needed to reproduce the build. Do not publish a binary-only
release. Anyone receiving a covered binary must be given a practical way to
obtain its corresponding source under the AGPL terms. Keep the exact source
commit, package version, build instructions, and checksums attached to each
release.

## Package behavior

The editor is an ordinary GUI application and is deliberately not installed as
a systemd service or desktop autostart entry. When launched, it checks for the
user's IBus session, starts IBus if necessary, and selects the Nudi engine. If
the session is still initializing, it retries automatically. When the last
editor window closes, it restores the engine that was active before Nudi; it
does not stop the shared IBus daemon. The standalone IBus engine is otherwise
started by the user's IBus session on demand.

The Debian package declares conflicts/replacements for legacy Nudi packages,
including `nudi-kacheri-suite`, so apt removes an old package that owns the
same IBus executable during migration. The `build-linux-package.sh` installer
upgrades an older `kannada-nudi` version noninteractively and skips installation
when the same or a newer version is already present.

Engine diagnostics use the XDG state directory:

```text
~/.local/state/kannada-nudi/YYYY-MM-DD/engine-<uid>.log
```

This keeps logs private to the user and avoids creating writable directories
under `/var/log` from package maintainer scripts.

## Release checklist

Before publishing a release:

1. Build and run all tests with CMake/CTest.
2. Check the installed desktop entry and IBus component paths.
3. Inspect package metadata and dependencies with `dpkg-deb -I` or `rpm -qip`.
4. Install the package in a clean user session and verify that reboot/login
   does not launch the editor unexpectedly, while opening the editor starts
   Nudi automatically.
5. Test upgrade from the previous release and confirm that the old package
   name is replaced cleanly.
6. Publish checksums and release notes with the package artifacts.
7. Confirm that the source archive and license notice are attached alongside
   every binary release.

The GitHub Actions workflow increments the patch version, builds all release
artifacts, runs the Debian tests, builds the RPM in Fedora, and attaches the
artifacts to a tagged GitHub Release. Do not manually reuse an existing tag.

## Local RPM validation

On Fedora or in a Fedora container, install `rpm-build`, CMake, the GTK/IBus
development packages, and `rpmlint`, then run:

```sh
rpmbuild -bb --define "nudi_version 1.0.9" packaging/kannada-nudi.spec
rpmlint ~/rpmbuild/RPMS/*/kannada-nudi-1.0.9-*.rpm
rpm -qip ~/rpmbuild/RPMS/*/kannada-nudi-1.0.9-*.rpm
rpm -qlp ~/rpmbuild/RPMS/*/kannada-nudi-1.0.9-*.rpm
```

The spec runs CTest during `%check`, declares runtime dependencies, includes
the README, user/developer documentation, and mapping reference, refreshes the
hicolor icon cache, and uses `%autosetup -n %{name}-%{version}` so the
versioned source archive directory is validated consistently.

## User-facing release notes

Every release should state the supported distribution family, architecture,
installation command, IBus restart guidance, known limitations, and how to
report a crash. Mention that `nudi-editor` is optional and that users should
not run it as root. Keep the release notes aligned with [README.md](../README.md)
and [USER_GUIDE.md](USER_GUIDE.md).
