[xde-desktop -- read me first file.  @DATE]: #

xde-desktop
===============

Package `xde-desktop-0.2` was released under GPLv3 license 2016-07-09.

This package provides a desktop for the XDE (_X Desktop Environment_).
As with other XDE utilities, this package is intended on working with
the wide range of lightweight window managers supported by the XDE.
This desktop was originally written in `perl(1)` and was part of the
`xde-tools` package, then later in "C" in the `xde-ctools` package.  It
has now been split off to a separate package for those that just want
to run the desktop.


Release
-------

This is the `xde-desktop-0.2` package, released 2016-07-09.  This
release, and the latest version, can be obtained from the [GitHub
repository][1], using a command such as:

    $> git clone https://github.com/bbidulock/xde-desktop.git

Please see the [NEWS][2] file for release notes and history of user
visible changes for the current version, and the [ChangeLog][3]
file for a more detailed history of implementation changes.  The
[TODO][4] file lists features not yet implemented and other
outstanding items.

Please see the [INSTALL][5] file for installation instructions.

When working from `git(1)`, please use this file.  An abbreviated
installation procedure that works for most applications appears below.

This release is published under GPLv3.  Please see the license in
the file [COPYING][6].


Quick Start
-----------

The quickest and easiest way to get `xde-desktop` up and running
is to run the following commands:

    $> git clone https://github.com/bbidulock/xde-desktop.git
    $> cd xde-desktop
    $> ./autogen.sh
    $> ./configure --prefix=/usr --sysconfdir=/etc
    $> make V=0
    $> make DESTDIR="$pkgdir" install

This will configure, compile and install `xde-desktop` the quickest.
For those who would like to spend the extra 15 seconds reading
the output of `./configure --help`, some compile options can be
turned on and off before the build.

For general information on GNU's `./configure`, see the file
[INSTALL][5].


Issues
------

Report problems at GitHub [here][7].



[1]: https://github.com/bbidulock/xde-desktop
[2]: NEWS
[3]: ChangeLog
[4]: TODO
[5]: INSTALL
[6]: COPYING
[7]: https://github.com/bbidulock/xde-desktop/issues

[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #
