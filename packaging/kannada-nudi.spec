Name:           kannada-nudi
Version:        %{nudi_version}
Release:        1%{?dist}
Summary:        Kannada Nudi Linux keyboard input method
License:        Unknown
URL:            https://github.com/kagapa-blr/linux-nudi-keyboard
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(ibus-1.0)
Requires:       ibus
Requires:       glib2
Requires:       gtk3
Requires:       ibus-libs

%description
A system-wide Kannada Nudi keyboard input method for Linux desktops using IBus.

%prep
%setup -q -n %{name}-%{version}

%build
cmake -S . -B build-rpm \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBEXECDIR=%{_libexecdir}
cmake --build build-rpm --parallel %{?_smp_build_ncpus}

%install
DESTDIR=%{buildroot} cmake --install build-rpm

%files
%{_bindir}/nudi-editor
%{_libexecdir}/ibus-engine-nudi
%{_datadir}/ibus/component/com.example.Nudi.xml
%{_datadir}/applications/nudi.desktop
%{_datadir}/icons/hicolor/48x48/apps/nudi.png
%{_datadir}/icons/hicolor/64x64/apps/nudi.png
%{_datadir}/icons/hicolor/128x128/apps/nudi.png

%changelog
* Thu Sep 03 2026 Nudi Linux contributors <nudi@example.invalid> - 1.0.0-1
- Initial RPM package.
