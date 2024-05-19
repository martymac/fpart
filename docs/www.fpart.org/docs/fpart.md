
![Fpart](img/Fpart.png)

# What is fpart ?

Fpart is a UNIX CLI tool that splits a list of directories and file trees into
a certain number of partitions, trying to produce partitions with the same size
and number of files.
It can also produce partitions with a given number of files or of a limited
size. Fpart uses a bin packing algorithm to optimize space utilization amongst
partitions.

Once generated, partitions are either printed as file lists to stdout (default)
or to files. Those lists can then be used by third party programs.

Fpart also includes a live mode, which allows it to crawl very large filesystems
and produce partitions in live. Hooks are available to act on those partitions
(e.g. immediately start a transfer using rsync(1) or cpio(1)) without having to
wait for the filesystem traversal job to be finished. Used that way, fpart can
be seen as a powerful basis for a data migration tool.

Fpart can also generate lists of directories instead of files. That mode can
be useful to enable usage of options requiring overall knowledge of directories
such as rsync's --delete.

As a demonstration of fpart possibilities, a tool called fpsync is provided in
the tools/ directory (see related documentation for more details).

# Does it modify my data ?

No. Fpart does *NOT* split or modify your data in any way. It only crawls your filesystem and produces file (or directory) lists. Those lists are called "partitions" because they are a subset of an entire file tree. Fpart only reads your data, never modifies it.

# Examples

The following will produce 3 partitions, with (approximatively) the same size
and number of files. Three files: "var-parts.[1-3]", are generated as output :

    $ fpart -n 3 -o var-parts /var

    $ ls var-parts*
    var-parts.1 var-parts.2 var-parts.3

    $ head -n 2 var-parts.1
    /var/some/file1
    /var/some/file2

The following will produce partitions of 4.3 GB, containing music files ready
to be burnt to a DVD (for example). Files "music-parts.[0-n]", are generated
as output :

    $ fpart -s 4617089843 -o music-parts /path/to/my/music

The following will produce partitions containing 10000 files each by examining
/usr first and then /home and display only partition 1 on stdout :

    $ find /usr ! -type d | fpart -f 10000 -i - /home | grep '^1 '

The following will produce two partitions by re-using du(1) output. Fpart will
not examine the filesystem but instead re-use arbitrary values printed by du(1)
and sort them :

    $ du * | fpart -n 2 -a

# Live mode

By default, fpart will wait for FS crawling to terminate before generating and
displaying partitions. If you use the live mode (option -L), fpart will display
each partition as soon as it is complete. You can combine that option with
hooks; they will be triggered just before (pre-part hook, option -w) or after
(post-part hook, option -W) partitions' completion.

Hooks provide several environment variables (see fpart(1)); they are a
convenient way of getting information about fpart's and partition's current
states. For example, ${FPART_PARTFILENAME} will contain the name of the output
file of the partition that has just been generated; using that variable within a
post-part hook permits starting manipulating the files just after partition
generation.

See the following example :

    $ mkdir foo && touch foo/{bar,baz}
    $ fpart -L -f 1 -o /tmp/part.out -W \
        'echo == ${FPART_PARTFILENAME} == ; cat ${FPART_PARTFILENAME}' foo/
    == /tmp/part.out.1 ==
    foo/bar
    == /tmp/part.out.2 ==
    foo/baz

This example crawls foo/ in live mode (option -L). For each file (option -f,
1 file per partition), it generates a partition into /tmp/part.out.<n>
(option -o; <n> is the partition index and will be automatically added by fpart)
and executes the following post-part hook (option -W) :

    echo == ${FPART_PARTFILENAME} == ; cat ${FPART_PARTFILENAME}

This hook will display the name of current partition's output file name as well
as display its contents.

# Migrating data (without fpsync)

Here is a more complex example that will show you how to use fpart, GNU Parallel
and Rsync to split up a directory and immediately schedule data synchronization
of smaller lists of files, while FS crawling goes on. We will be synchronizing
data from /data/src to /data/dest.

First, go to the source directory (as rsync's --files-from option takes a file
list relative to its source directory) :

    $ cd /data/src

Then, run fpart from here :

    $ fpart -L -f 10000 -x '.snapshot' -x '.zfs' -zz -o /tmp/part.out -W \
      '/usr/local/bin/sem -j 3
        "/usr/local/bin/rsync -av --files-from=${FPART_PARTFILENAME}
          /data/src/ /data/dest/"' .

This command will start fpart in live mode (option -L), making it generate
partitions during FS crawling. Fpart will produce partitions containing at most
10000 files each (option -f), will skip files and folders named '.snapshot' or
'.zfs' (option -x) and will list empty and non-accessible directories (option
-zz; that option is necessary when working with rsync to make sure the whole file
tree will be re-created within the destination directory). Last but not least,
each partition will be written to /tmp/part.out.<n> (option -o) and used within
the post-part hook (option -W), run immediately by fpart once the partition is
complete :

    /usr/local/bin/sem -j 3
        "/usr/local/bin/rsync -av --files-from=${FPART_PARTFILENAME} /data/src/ /data/dest/"

This hook is itself a nested command. It will run GNU Parallel's sem scheduler
(any other scheduler would do) to run at most 3 rsync jobs in parallel.

The scheduler will finally trigger the following command :

    /usr/local/bin/rsync -av --files-from=${FPART_PARTFILENAME} /data/src/ /data/dest/

where ${FPART_PARTFILENAME} will be part of rsync's environment when it runs
and contains the file name of the partition that has just been generated.

That's all, folks ! Pretty simple, isn't it ?

In this example, FS crawling and data transfer are run from the same -local-
machine, but you can use it as the basis of a much sophisticated solution: at
$work, by using a cluster of machines connected to our filers through NFS and
running Open Grid Scheduler, we successully migrated over 400 TB of data.

Note: several successive fpart runs can be launched using the above example;
you will perform incremental synchronizations. That is, deleted files from the
source directory will not be removed from destination unless rsync's --delete
option is used. Unfortunately, this option cannot be used with a list of files
(files that do not appear in the list are just ignored). To use the --delete
option in conjunction with fpart, you *have* to provide rsync's --files-from
option a list of directories (only); that can be performed using fpart's -E
option.

# Limitations

* Again, Fpart will *NOT* modify data, it will *NOT* split your files !

    As a consequence, if you have a directory containing several small files
    and a huge one, it will be unable to produce partitions with the same size.
    Fpart does magic, but not that much ;-)

* Fpart will not deduplicate paths !

    If you provide several paths to fpart, it will examine all of them. If those
    paths overlap or if the same path is specified more than once, same files
    will appear more than once within generated partitions. This is not a bug,
    fpart does not deduplicate FS crawling results.
