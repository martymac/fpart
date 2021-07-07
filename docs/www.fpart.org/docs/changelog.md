# Next: 1.3.1
```nohighlight
    2021/??/??, 1.3.1 ('???') :
    - fpsync: allow special characters and white spaces within fpart options
      Allows something like: fpsync ... -O "-y|foo bar|-y|*.txt" /src/ /dst/
      (fixes GH issue #33)
    - fpsync: fix useless double call to sudo(1) in cpio(1) jobs
```
# Current: 1.3.0
```nohighlight
    2021/05/08, 1.3.0 ('Cleaning room') :
    - fpart: align get_size() and init_file_entries() behaviour and take
      exclusions into account when computing a directory size.
    - fpart: fix directory size computation when using options -y/-Y.
      Before that change, -D and -E would have returned a size of 0 for a
      directory included through options -y/-Y.
    - fpart: fix detection of an unknown option
    - fpart: various fixes on Darwin
    - fpsync: mask broken pipe errors when dequeueing jobs (fixes GH issue #22)
    - fpsync: fix bug where a job could remain resumable forever under certain
      conditions.
    - fpsync: fix detection of fpart exiting with errors
    - fpsync: add prepare run option (-p)
    - fpsync: add list runs options (-l)
    - fpsync: add replay run option (-R)
    - fpsync: add archive run option (-a)
      Note for packagers: adds a new dependency to tar(1)
    - fpsync: add delete run option (-D)
    - fpsync: rework help and man page for clarification
    - fpsync: add check for incompatible rsync option (--delete)
    - fpsync: add send mail option (-M)
      Note for packagers: adds a new dependency to mail(1)
```
# 1.2.0
```nohighlight
    2020/01/10, 1.2.0 ('Joking') :
    - fpsync: ability to set an unlimited number of files or an unlimited size
      per sync job (fixes GH issue #9)
    - fpart: exclude files earlier (fixes GH issue #14)
      Before that change, excluded files would not make a directory empty and
      still account for its size; excluded files are now considered absent.
      That change does not apply when a directory size is computed after
      a certain depth (option -d) has been reached (in that case, once the
      maximum depth has been reached, every single file accounts for a directory
      size).
    - fpart: allow to specify paths with options '-y', '-Y', '-x' and '-X'
      (fixes GH issue #17)
    - fpart: raise limits for different internal types: the size of a file or
      partition, the number of files inside a partition and the number of
      partitions. That will make fpart(1) ready to handle bigger filesystems.
    - fpart: accept human-friendly size formats for options -s, -p, -q and -r
    - fpsync: accept human-friendly size formats for option -s
    - fpsync: use 'command' instead of 'which' to find binaries
      (fixes GH issue #20)
```
# 1.1.0
```nohighlight
    2018/11/15, 1.1.0 ('Reading') :
    - fpart: options -D and -E now pack files if they are explicitly passed as
      arguments
    - fpsync: simplify and fix detection of remote URLs (fixes GH issue #2)
    - fpsync: implement simple ETA when displaying status
    - fts.h: portability fix for Solaris 10 and earlier (GH PR #4)
    - fpart: fix build on Alpine Linux (GH PR #5)
    - fpsync: add timestamps and part numbers to logs
    - fpsync: add support for cpio copy tool (option -m)
    - fpart: option -Z becomes option -zz (incremental -z)
    - fpart: add option -zzz to add all directories to file lists (as empty)
    - other: lower required autoconf version to 2.63 (GH issue #7)
    - other: update RPM .spec file for CentOS and Feroda (GH issue #7)
```
# 1.0.0
```nohighlight
    2017/11/10, 1.0.0 ('Dancing'):
    - fpart: update embedded fts(3) using FreeBSD 12-CURRENT's version
    - fpart: add option -E to pack directories instead of files
    - fpsync: add option -E to work with directories (enables rsync(1)'s --delete)
      That option finally enables fpsync(1) to parallelize the so-called final
      rsync(1) pass, needed to remove extra files with the --delete option!
      (of course it can also be used on a regular basis instead of the usual
      'file' mode: you will get a perfect mirror without needing any extra
      --delete pass)
    - fpart: add option -0 to end filenames with a null (\0) character when
      using option -o
    - fpsync: use fpart's new option -0 in conjunction with rsync's --from0 option
      to avoid failing on filenames with special characters
    - fpsync: improve documentation
```
# 0.9.3
```nohighlight
    2017/04/27, 0.9.3 ('Funny words'):
    - fpsync: add support for rsync URL as target
      Suggested and tested by Harry Mangalam. Thanks, Harry!
```
# 0.9.2
```nohighlight
    2015/02/19, 0.9.2 ('What's that?'):
    - fpsync: add option '-S' to use sudo(1) for filesystem crawling and synchronizations
    - fpsync: add option '-O' to override default fpart(1) options
    - add RPM .spec file (see the contribs/package/rpm directory)
```
# 0.9.1
```nohighlight
    2015/02/06, 0.9.1 ('Let's play together'):
    - add tools/fpsync: a tool to sync directories in parallel using fpart and
      rsync. See fpsync(1) for more details.
    - fpart: print the number of files found in verbose mode only
```
# 0.9-1
```nohighlight
    2013/11/13, 0.9-1:
    (this minor change only impacts tarball users, fpart code itself has not
    changed)
    - Backport the following patch to config.guess:
      http://git.savannah.gnu.org/gitweb/?p=config.git;a=commitdiff;h=29900d3bff1ce445087ece5cb2cac425df1c2f74
      it adds support for ppcle and ppc64le architectures.
    Submitted by: Madhu Pavan <kmp@linux.vnet.ibm.com>
```
# 0.9
```nohighlight
    2013/09/10, 0.9 ('I bite!'):
    - Code cleanup
    - Fix Debian bug #719338 (fix build on 32bit architectures)
```
# 0.8
```nohighlight
    2013/06/25, 0.8 ('Moving around'):
    - Keep environ(7) when forking hooks
    - Add sections about live mode and data migration in README
    - Use autotools and get rid of manual Makefiles
```
# 0.7
```nohighlight
    2013/02/18, 0.7 ('Can I take it ?'):
    - Added option -D to group leaf directories as single file entries
    - Big options and doc update/cleanup
    - Sync'd fts(3) code with current FreeBSD version (svn rev 245505)
    - Embedded fts(3) can now be used on GNU/Linux
    - Renamed option '-x' to '-b'
    - Added options '-y', '-Y', '-x' and '-X' to include or exclude files
```
# 0.6
```nohighlight
    2013/01/21, 0.6 ('Very funny'):
    - Options handling cleanup and various bugfixes
    - Added FPART_PID and FPART_HOOKTYPE hook variables
    - Added -z and -Z options to display empty directories
```
# 0.5
```nohighlight
    2013/01/09, 0.5 ('Cassounette'):
    - Added option '-L' (live mode)
    - Added options '-w' and '-W' (hooks to be used with live mode)
    - Fixed build on Solaris 9
    - Removed option '-m' and associated code
```
# 0.4
```nohighlight
    2012/06/12, 0.4:
    - Now builds on Solaris (using FreeBSD's fts(3))
    - Fix stack overflow when allocating file entry pointers' array
    - Added preloading/rounding options -p, -q and -r. See fpart(1)
    - Added more verbose messages (option '-v')
    - Better error handling when unable to add file entries (mostly when
      running out of memory)
    - Verbose mode now accepts several levels
    - Added option '-m' (disabled by default), that tries to lower physical
      memory usage (at least during FS crawling) by using temporary file(s)
      and mmap(2) facility
```
# 0.3
```nohighlight
    2011/12/06, 0.3:
    - Switch to fts(3)
    - Replaced getline(3) calls with fgets(3) for compatibility
    - New "handle arbitrary values" (-a) option
    - Strings handling cleanup (stop using FILENAME_MAX)
    - Various smaller changes
```
# 0.2
```nohighlight
    2011/11/24, 0.2:
    - New "follow symbolic links" (-l) option
    - Ending slash (if present in input path) is now left to allow following an
      initial symbolic link (without using option -l)
    - New "do not cross file system boundaries" (-x) option
    - Fpart now reads on stdin by default if no path is given
    - File size are now written to stdout when displaying partition contents
    - Various smaller changes
```
# 0.1
```nohighlight
    2011/11/18, 0.1:
    - Initial version
```
