---
        _______ ____   __         _      __
       / /  ___|  _ \ / /_ _ _ __| |_   / /
      / /| |_  | |_) / / _` | '__| __| / /
     / / |  _| |  __/ / (_| | |  | |_ / /
    /_/  |_|   |_| /_/ \__,_|_|   \__/_/

---

# What is fpart ?

Fpart is a **F**ilesystem **part**itioner. It helps you sort file trees and
pack them into bags (called "partitions"). It is developed in C and available
under the BSD license.

Fpsync, a powerful file-migration tool is also provided.

See the [documentation](https://www.fpart.org) for more details and examples.

# Key features

## Fpart

* Blazing fast crawling!
* Generates partitions on a number/file number/size basis
* Provides a live mode with hooks to act immediately on generated file lists
* Supports generating partitions from arbitrary input (e.g. du's output)

## Fpsync

* Parallelizes rsync(1), cpio(1), pax(1) or tar(1) jobs
* Supports using a SSH cluster for efficient data migrations
* Starts transfers while FS crawling still goes on
* Supports remote target URLs when using rsync(1)
* Parallelizes your final rsync(1) pass too!
* Provides transfer runs' status/resume/replay
* Nearly no dependencies (mostly shell and common tools)

# Author / Licence

Fpart has been written by [Ganael LAPLANCHE](mailto:ganael.laplanche@martymac.org)
and is available under the BSD license (see COPYING for details).

Source code is hosted on :

* [Martymac.org](https://contribs.martymac.org)
* [Github](https://github.com/martymac/fpart)
* [Sourceforge](https://www.sourceforge.net/projects/fpart)

Documentation is available on :

* [Fpart.org](https://www.fpart.org)

Thanks to Jean-Baptiste Denis for having given me the idea of this program !

# Third-party code

fts(3) code originally comes from FreeBSD :

    lib/libc/gen/fts.c -> src/fts.c
    include/fts.h      -> src/fts.h

It has been slightly modified for portability and is available under the BSD
license.

# Supporting fpart

If fpart (or fpsync) is useful to you or your organization, do not hesitate to
contribute back! You can follow ideas in the [TODO](https://github.com/martymac/fpart/blob/master/TODO)
file or just fix a bug, any kind of help is always welcome!

You can also make a donation via Paypal:

[![Paypal](https://www.paypalobjects.com/en_US/FR/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=HSL25ZED2PS62&source=url)

or [Github](https://github.com/sponsors/martymac?o=esb).

That will help me not running out of tea :)
