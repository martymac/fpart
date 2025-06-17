Name:    fpart
Version: 1.7.0
Release: 1%{?dist}
License: BSD
Summary: a tool that sorts files and packs them into bags
URL:     http://contribs.martymac.org

Source0:  https://github.com/martymac/%{name}/archive/%{name}-%{version}.tar.gz

%if 0%{?fedora}
Suggests: sudo rsync cpio
%endif
BuildRequires: gcc autoconf automake

%description
Fpart is a tool that helps you sort file trees and pack them into bags (called
"partitions"). It is developed in C and available under the BSD license.

It splits a list of directories and file trees into a certain number of
partitions, trying to produce partitions with the same size and number of
files. It can also produce partitions with a given number of files or a limited
size.

%prep
# The name macro is used twice due to the way the upstream project includes the project name in its release tags
%setup -q -n %{name}-%{name}-%{version}

%build
autoreconf --install
%configure
make %{?_smp_mflags}

%install
%make_install

%files
%license COPYING
%doc Changelog.md README.md TODO
%{_mandir}/man1/fpart.1*
%{_mandir}/man1/fpsync.1*
%{_bindir}/fpart
%{_bindir}/fpsync

%changelog
* Fri Dec 06 2019 Sam P <survient@fedoraproject.org> - 1.2.0-2
- Revised rpmbuild instructions
- Added comment about git release tag naming
- Added mass Fedora 31 rebuild commit history entry

* Tue Oct 29 2019 Christopher Voltz <christopher.voltz@hpe.com> - 1.2.0-1
- Version 1.2.0
- Added instructions for building RPMs
- Fixed build directory name

* Thu Jul 25 2019 Fedora Release Engineering <releng@fedoraproject.org> - 1.1.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Mon Nov 19 2018 Sam P <survient@fedoraproject.org> - 1.1.0-2
- cleaned up prep and build sections
- trued up changelog entries
- corrected desciption
- added missing weak dependencies

* Fri Nov 16 2018 Sam P <survient@fedoraproject.org> - 1.1.0-1
- repackaged to stable release version 1.1.0

* Fri Nov 16 2018 Ganael Laplanche <ganael.laplanche@martymac.org> - 1.1.0
- Version 1.1.0

* Mon Nov 12 2018 Sam P <survient@fedoraproject.org> - 1.0.0-4.20181109git57f49f5
- pulled down latest snapshot which merged patch

* Wed Oct 31 2018 Sam P <survient@fedoraproject.org> - 1.0.0-3.20181022git130f8fd
- added patch for autoconf version for EL6 compatibility

* Wed Oct 31 2018 Sam P <survient@fedoraproject.org> - 1.0.0-3.20181022git130f8fd
- updated to snapshot 130f8fdadf2bbcc3cdaad479a356e8d0e3f6f041

* Thu Apr 19 2018 Sam P <survient@fedoraproject.org> - 1.0.0-2
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
