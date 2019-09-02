[xde-desktop -- read me first file.  2019-09-02]: #

xde-desktop
===============

Package `xde-desktop-0.3` was released under GPLv3 license 2019-09-02.

This package provides a desktop for the XDE (_X Desktop Environment_).
As with other XDE utilities, this package is intended on working with
the wide range of lightweight window managers supported by the XDE.
This desktop was originally written in `perl(1)` and was part of the
`xde-tools` package, then later in "C" in the `xde-ctools` package.  It
has now been split off to a separate package for those that just want to
run the desktop.


Release
-------

This is the `xde-desktop-0.3` package, released 2019-09-02.  This
release, and the latest version, can be obtained from [GitHub][1], using
a command such as:

    $> git clone https://github.com/bbidulock/xde-desktop.git

Please see the [NEWS][3] file for release notes and history of user
visible changes for the current version, and the [ChangeLog][4] file for
a more detailed history of implementation changes.  The [TODO][5] file
lists features not yet implemented and other outstanding items.

Please see the [INSTALL][7] file for installation instructions.

When working from `git(1)`, please use this file.  An abbreviated
installation procedure that works for most applications appears below.

This release is published under GPLv3.  Please see the license in the
file [COPYING][9].


Quick Start
-----------

The quickest and easiest way to get `xde-desktop` up and running is to run
the following commands:

    $> git clone https://github.com/bbidulock/xde-desktop.git
    $> cd xde-desktop
    $> ./autogen.sh
    $> ./configure
    $> make
    $> make DESTDIR="$pkgdir" install

This will configure, compile and install `xde-desktop` the quickest.  For
those who like to spend the extra 15 seconds reading `./configure
--help`, some compile time options can be turned on and off before the
build.

For general information on GNU's `./configure`, see the file
[INSTALL][7].


Running
-------

Read the manual page after installation:

    man xde-desktop


Issues
------

Report issues on GitHub [here][2].



[1]: https://github.com/bbidulock/xde-desktop
[2]: https://github.com/bbidulock/xde-desktop/issues
[3]: https://github.com/bbidulock/xde-desktop/blob/0.3/NEWS
[4]: https://github.com/bbidulock/xde-desktop/blob/0.3/ChangeLog
[5]: https://github.com/bbidulock/xde-desktop/blob/0.3/TODO
[6]: https://github.com/bbidulock/xde-desktop/blob/0.3/COMPLIANCE
[7]: https://github.com/bbidulock/xde-desktop/blob/0.3/INSTALL
[8]: https://github.com/bbidulock/xde-desktop/blob/0.3/LICENSE
[9]: https://github.com/bbidulock/xde-desktop/blob/0.3/COPYING

[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #
