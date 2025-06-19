To build RPM packages:

dnf install rpmdevtools autoconf automake gcc
git clone https://github.com/martymac/fpart.git
cp -f fpart/contribs/package/rpm/fpart.spec rpmbuild/SPECS/
spectool -g -R rpmbuild/SPECS/fpart.spec
rpmbuild -ba rpmbuild/SPECS/fpart.spec

Packages are available in Fedora and EPEL repositories but may not be up to date
