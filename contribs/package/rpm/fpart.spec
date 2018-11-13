%global commit 57f49f5c558cefe98c0194c220e8db8b272a1632
%global shortcommit %(c=%{commit}; echo ${c:0:7})
%global snapshotdate 20181109

Name:    fpart
Version: 1.0.0
Release: 4.%{snapshotdate}git%{shortcommit}%{?dist}
License: BSD
Summary: Fpart is a tool that sorts files and packs them into bags.
URL:     http://contribs.martymac.org

Source0:  https://github.com/martymac/fpart/archive/%{commit}/%{name}-%{version}-%{shortcommit}.tar.gz

BuildRequires: gcc autoconf automake

%description
Fpart is a tool that helps you sort file trees and pack them into bags (called
"partitions"). It is developed in C and available under the BSD license.

It splits a list of directories and file trees into a certain number of
partitions, trying to produce partitions with the same size and number of
files. It can also produce partitions with a given number of files or a limited
size.

%prep
%setup -q -n fpart-%{commit}

autoreconf --install
%configure

%build
make %{?_smp_mflags}

%install
%make_install

%files
%license COPYING
%doc Changelog README TODO
%{_mandir}/man1/fpart.1*
%{_mandir}/man1/fpsync.1*
%{_bindir}/fpart
%{_bindir}/fpsync

%changelog
* Wed Nov 12 2018 samuel - 1.0.0-4.20181109git57f49f5
- pulled down latest snapshot which merged patch

* Wed Oct 31 2018 samuel - 1.0.0-3.20181022git130f8fd
- added patch for autoconf version for EL6 compatibility

* Wed Oct 31 2018 samuel - 1.0.0-3.20181022git130f8fd
- updated to snapshot 130f8fdadf2bbcc3cdaad479a356e8d0e3f6f041

* Thu Apr 19 2018 samuel - 1.0.0-2
- Used %%buildroot macro
- Correctly marked license
- Other small packaging corrections

* Fri Nov 10 2017 Ganael Laplanche <ganael.laplanche@martymac.org> - 1.0.0
- Version 1.0.0

* Thu Apr 27 2017 Ganael Laplanche <ganael.laplanche@martymac.org> - 0.9.3
- Version 0.9.3

* Tue Feb 17 2015 Ganael Laplanche <ganael.laplanche@martymac.org> - 0.9.2
- Version 0.9.2

* Mon Feb 16 2015 Tru Huynh <tru@pasteur.fr> - 0.9.1
- Initial build of the package.
