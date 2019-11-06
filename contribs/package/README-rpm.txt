To build RPM packages:

wget https://github.com/martymac/fpart/archive/master.tar.gz
tar xvfz master.tar.gz
rm master.tar.gz
version=$(awk '/^Version/ { print $2 }' fpart-master/contribs/package/rpm/fpart.spec)
mv fpart-master fpart-${version}
tar cvfz ~/rpmbuild/SOURCES/fpart-${version}.tar.gz fpart-${version}
cp fpart-${version}/contribs/package/rpm/fpart.spec ~/rpmbuild/SPECS/
cd ~/rpmbuild/SPECS
rpmbuild -ba fpart.spec
