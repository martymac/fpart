Name:    fpart
Version: 0.9.2
Release: 1%{?dist}
Group:   Applications/System
License: BSD
Summary: Fpart is a tool that sorts files and packs them into bags.
URL:     http://contribs.martymac.org

BuildRoot:     %{_tmppath}/%{name}-%{version}-%{release}-build-%(%{__id_u} -n)
Source0:  http://contribs.martymac.org/fpart/%{name}-%{version}.tar.gz

%description
Fpart is a tool that helps you sort file trees and pack them into bags (called
"partitions"). It is developped in C and available under the BSD license.

It splits a list of directories and file trees into a certain number of
partitions, trying to produce partitions with the same size and number of
files. It can also produce partitions with a given number of files or a limited
size.

%prep
%setup -q
%configure

%build
%{__make}

%install
%{__make} install DESTDIR=${RPM_BUILD_ROOT}
%{__rm} -rf %{RPM_BUILD_ROOT}
mkdir -p %{RPM_BUILD_ROOT}%{_docdir}
%{__install} -p -m 0644 Changelog COPYING README TODO  %{RPM_BUILD_ROOT}%{_docdir}/

%clean
%{__rm} -rf %{RPM_BUILD_ROOT}

%files
%defattr(-,root,root,0755)
%doc Changelog COPYING README TODO
%{_mandir}/man1/fpart.1*
%{_mandir}/man1/fpsync.1*
%{_bindir}/fpart
%{_bindir}/fpsync

%changelog
* Tue Feb 17 2015 Ganael Laplanche <ganael.laplanche@martymac.org> - 0.9.2
- Version 0.9.2

* Mon Feb 16 2015 Tru Huynh <tru@pasteur.fr> - 0.9.1
- Initial build of the package.
