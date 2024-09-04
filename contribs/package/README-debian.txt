How to build Debian packages (e.g. for fpart version 1.1.0) :
*************************************************************

1) Firt, install required tools:

# apt-get install build-essential curl debhelper automake dh-autoreconf

2) Clone fpart repository:

$ git clone 'https://github.com/martymac/fpart.git'
$ cd fpart

3) Checkout desired tag :

$ git checkout fpart-1.1.0
$ curl 'https://contribs.martymac.org/fpart/fpart-1.1.0.tar.gz' > ../fpart_1.1.0.orig.tar.gz

4) Get latest package files from master branch :

$ git checkout master contribs/package/debian
$ mv contribs/package/debian .

5) Build source and binary packages:

$ dpkg-source -b .
$ dpkg-buildpackage -us -b
