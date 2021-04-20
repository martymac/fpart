
![Fpsync](img/Fpsync.png)

# What is Fpsync ?

To demonstrate fpart possibilities, a program called 'fpsync' is provided within
the tools/ directory. This tool is a shell script that wraps fpart(1) and
rsync(1) (or cpio(1)) to launch several synchronization jobs in parallel as
presented in the previous section, but while the previous example used GNU
Parallel to schedule transfers, fpsync provides its own -embedded- scheduler.
It can execute several synchronization processes locally or launch them on
several nodes (workers) through SSH.

Despite its initial "proof of concept" status, fpsync has quickly evolved into
a powerful (yet simple to use) migration tool and has been successfully used
to boost migration of several hundreds of TB of data (initially at $work but it
has also been tested by several organizations such as UCI, Intel and Amazon ;
see the 'See also' section at the end of this document).

In addition to being very fast (as transfers start during FS crawling and are
parallelized), fpsync is able to resume or replay synchronization "runs" (see
options -r and -R) and presents an overall progress status. It also has a small
memory footprint compared to rsync itself when migrating filesystems with a big
number of files.

Last but not least, fpsync is very easy to set up and only requires a few
(common) software to run: fpart, rsync and/or cpio, a POSIX shell, sudo and ssh.

See fpsync(1) to learn more about that tool and get a list of all supported
options.

---

**Here is a simple representation of how it works :**

    fpsync [args] /data/src/ /data/dst/
      |
      +-- fpart (live mode) crawls /data/src/, generates parts.[1] + sync jobs ->
      |    \    \    \
      |     \    \    +___ part. #n + job #n
      |      \    \
      |       \    +______ part. #1 + job #1
      |        \
      |         +_________ part. #0 + job #0
      |
      +-- fpsync scheduler, executes jobs either locally or remotely ----------->
           \    \    \
            \    \    +___ sync job #n... --------------------------------------> +
             \    \                                                               |
              \    +______ sync job #1 ---------------------------------->        |
               \                                                                  |
                +_________ sync job #0 ----------------------------->             +
                                                                                 /
                                                                                /
                  Filesystem tree rebuilt and synchronized! <------------------+

    [1] Either containing file lists (default mode) or directory lists (option -E)

---

# Examples

In its default mode, fpsync uses rsync(1) and works with file lists to perform
incremental (only) synchronizations. You can choose to use cpio(1) instead of rsync(1) with option '-m' (see [Cpio support](#cpio-support) below).

The following examples show two typical usage.

The command :

    $ fpsync -n 4 -f 1000 -s $((100 * 1024 * 1024)) \
        /data/src/ /data/dst/

will synchronize /data/src/ to /data/dst/ using 4 local workers, each one
transferring at most 1000 files and 100 MB per synchronization job.

The command :

    $ fpsync -n 8 -f 1000 -s $((100 * 1024 * 1024)) \
        -w login@machine1 -w login@machine2 -d /mnt/nfs/fpsync \
        /data/src/ /data/dst/

will synchronize /data/src/ to /data/dst/ using the same transfer limits, but
through 8 concurrent synchronization jobs spread over two machines (machine1 and
machine2). Those machines must both be able to access /data/src/ and /data/dst/,
as well as /mnt/nfs/fpsync, which is fpsync's shared working directory.

As previously mentioned, those two examples work with file lists and will
perform *incremental* synchronizations. As a consequence, they will require a
final -manual- 'rsync --delete' pass to delete extra files from the /data/dst/
directory.

# The final pass

(A.K.A "Directory mode")

If you want to avoid that final pass, use fpsync's option -E (only compatible
with rsync tool). That option will make fpsync work with a list of *directories*
(instead of files) and will (forcibly) enable rsync's --delete option with each
synchronization job. The counterpart of using that mode is that directory lists
are coarse-grained and will probably be less balanced than file lists. The best
option is probably to run several incremental jobs and keep the -E option to
speed up the final pass only.

(you can read the file
[Solving_the_final_pass_challenge.txt](https://github.com/martymac/fpart/blob/master/docs/Solving_the_final_pass_challenge.txt)
in the docs/ directory for more details about fpsync's option -E)

# Cpio support

Fpsync's option '-m' allows you to use cpio(1) instead of rsync(1) to copy
files. Cpio(1) is much faster than rsync(1) but there is a catch: when
re-creating a complex file tree, missing parent directories are created
on-the-fly. In that case, original directory metadata (e.g. timestamps) are
*not* copied from source.

To overcome that limitation, fpsync uses fpart's -zzz option to ask fpart to
also pack every single directory (0-sized) with file lists. Making directories
appear in file lists will ask cpio to copy their metadata when the directory is
processed (of course, fpart ensures that a parent directory entry appears after
files beneath. If the parent directory is missing it is first created on the
fly, then the directory entry makes cpio update its metadata).

This works fine with a single cpio process (fpsync's option -n 1) but not with 2
or more parallel processes which can treat partitions out-of-order. Indeed, if
several workers copy files to the same directory at the same time, it is
possible that the parent directory's original metadata gets re-applied while
another worker is still adding files to that directory. That can occur if a
directory list spreads over more than one partition. In such a situation,
original metadata (here, mtime) gets overwritten while new files get added to
the directory.

That race condition is un-avoidable (fpart would have to guarantee the
directory entry belongs to the *same* partition as its files beneath, that
would probably lead to un-balanced partitions as well as increased -and useless-
complexity).

You've been warned. Anyway, maybe you do not care about copying original
directory mtimes. If this is the case, you can ignore that situation. If you
care about them, running a second pass of fpsync will fix the timestamps.

# Notes about GNU cpio

Developments have been made with BSD cpio (FreeBSD version). Fpsync will work
with GNU cpio too but there are small behaviour differences you must be aware
of :

- for an unknown reason, GNU cpio will not apply mtime to the main target
directory (AKA './' when received by cpio).

- when using GNU cpio, you will get the following warnings when performing a
second pass :

    <file> not created: newer or same age version exists

You can ignore those warnings as that second pass will fix directory timestamps
anyway.

Warning: if you pass option '-u' to cpio (trough fpsync's option '-o') to get
rid of those messages, you will possibly re-touch directory mtimes (loosing
original ones). Also, be aware of what that option implies: re-transferring
every single file.

# Notes about hard links

Rsync can detect and replicate hard links with option -H but that will NOT work
with fpsync because rsync collects hard links' information on a per-run basis.

So, as for directory metadata (see above), being able to propagate hard links
with fpsync would require from fpart the guarantee that all related links belong
to the same partition.

Unfortunately, this is not something fpart can do because, in live mode (used by
fpsync to start synchronization as soon as possible), it crawls the filesystem
as it comes. As a consequence, there is no mean to know if a hard link connected
to a file already written to a partition (and probably already synchronized
through an independent rsync process) will appear later or not. Also, in
non-live mode, trying to group related hardlinks into the same partitions would
propably lead to un-balanced partitions as well as complexify code.

If you need to propagate hard links, you have 3 options:

* Re-create hard links on the target, but this is not optimal as you may not
  want to link 2 files together, even if they are similar

* Pre-copy hard linked files together (using find's '-type f -links +1' options)
  before running fpsync. That will work but linked files that have changed
  since your first synchronization will be converted back to regular files when
  running fpsync

* Use a final -monolithic- rsync pass with option -H that will re-create them

# SSH options

When dealing with SSH options and keys, keep in mind that fpsync uses SSH for
two kinds of operations :

* data synchronization (when ssh is forked by rsync),
  can occur locally or on remote workers (if using any)
* communication with workers (when ssh is forked by fpsync),
  only occurs locally (on the scheduler)

If you need specific options for the first case, you can pass ssh options by
using rsync's option '-e' (through fpsync's option '-o') and triple-escaping
the quote characters :

    $ fpsync [...] -o "-lptgoD -v --numeric-ids -e \\\"ssh -i ssh_key\\\"" \
        /data/src/ login@remote:/data/dst/

The key will have to be present and accessible on all workers.

Fpsync does not offer options to deal with the second case. You will have to
tune your ssh config file to enable passwordless communication with workers.
Something like :

    $ cat ~/.ssh/config
    Host remote
    IdentityFile /path/to/the/passwordless/key

should work.

# Limitations

* Fpsync only synchronizes directory contents !

    Contrary to rsync, fpsync enforces the final '/' on the source directory. It
    means that directory *contents* are synchronized, not the source directory
    itself (i.e. you will *not* get a subdirectory of the name of the source
    directory in the target directory after synchronization).

# Portability considerations

On OpenIndiana, if you need to use fpsync(1), the script will need adjustments :

* Change shebang from /bin/sh to a more powerful shell that understands local
  variables, such as /bin/bash.
* Adapt fpart(1) and grep(1) paths (use ggrep(1) instead of grep(1) as default
  grep(1) doesn't know about -E flag).
* Remove -0 and --quiet options from cpio call (they are not supported). As a
  consequence, also remove -0 from fpart options.

On Alpine Linux, you will need the 'fts-dev' package to build fpart(1).
