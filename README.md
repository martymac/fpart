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

See the [documentation](http://www.fpart.org) for more details and examples.

# Key features

## Fpart

* Blazing fast crawling!
* Generates partitions on a number/file number/size basis
* Provides a live mode with hooks to act immediately on generated file lists
* Supports generating partitions from arbitrary input (i.e. du's output)

## Fpsync

* Parallelizes rsync(1) or cpio(1) jobs
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

* [Martymac.org](http://contribs.martymac.org)
* [Github](https://github.com/martymac/fpart)
* [Sourceforge](http://www.sourceforge.net/projects/fpart)

Thanks to Jean-Baptiste Denis for having given me the idea of this program !

# Donation

If fpart is useful to you or your organization, you can make a donation here:

[![paypal](https://www.paypalobjects.com/en_US/FR/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=HSL25ZED2PS62&source=url)

That will help me not running out of tea :)

# Contributions

Member of Apple's Open Source Developer Program (thanks to Apple for their support to OpenSource software !)

<img src="https://uploads-ssl.webflow.com/5ac3c046c82724970fc60918/5c019d917bba312af7553b49_MacStadium-developerlogo.png" alt="MacStadium" width="180"/>
