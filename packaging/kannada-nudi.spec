Name:           kannada-nudi
Version:        %{nudi_version}
Release:        1%{?dist}
Summary:        Kannada Nudi Linux keyboard input method
License:        AGPL-3.0-only
URL:            https://github.com/kagapa-blr/kannada-nudi-core
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(ibus-1.0)
BuildRequires:  pkgconfig(gdk-pixbuf-2.0)
Requires:       ibus
Requires:       glib2
Requires:       gtk3
Requires:       ibus-libs
Requires:       hicolor-icon-theme
Requires(post): gtk3
Requires(postun): gtk3

%description
A Kannada Nudi transliteration input method for Linux desktops using IBus.
The package installs the IBus engine and an optional Kannada Nudi Editor. The
editor is not an autostart application and does not take ownership of the
desktop IBus daemon during login.

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S . -B build-rpm \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBEXECDIR=%{_libexecdir}
cmake --build build-rpm --parallel %{?_smp_build_ncpus}

%install
DESTDIR=%{buildroot} cmake --install build-rpm

%check
ctest --test-dir build-rpm --output-on-failure

%files
%{_bindir}/nudi-editor
%{_libexecdir}/ibus-engine-nudi
%{_datadir}/ibus/component/com.kagapa.Nudi.xml
%{_datadir}/applications/nudi.desktop
%{_datadir}/icons/hicolor/48x48/apps/nudi.png
%{_datadir}/icons/hicolor/64x64/apps/nudi.png
%{_datadir}/icons/hicolor/128x128/apps/nudi.png
%doc README.md docs/USER_GUIDE.md docs/DEVELOPER_GUIDE.md docs/nudi-mapping-reference.txt
%license LICENSE

%post
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q %{_datadir}/icons/hicolor || :
fi

%postun
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q %{_datadir}/icons/hicolor || :
fi

%changelog
* Thu Sep 03 2026 Nudi Linux contributors <kagapa@gmail.com> - 1.0.0-1
- Initial RPM package.
