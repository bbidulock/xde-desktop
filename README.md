
## xde-desktop

Package xde-desktop-0.2 was released under GPL license 2016-07-09.

This package provides a desktop for the XDE (X Desktop Environment).
As with other XDE utilities, this package is intended on working with
the wide range of lightweight window managers supported by the XDE.
This desktop was originally written in perl(1) and was part of the
xde-tools package, then later in "C" in the xde-ctools package.  It
has now been split off to a separate package for those that just want
to run the desktop.


### Release

This is the `xde-desktop-0.2` package, released 2016-07-09.  This release,
and the latest version, can be obtained from the GitHub repository at
https://github.com/bbidulock/xde-desktop, using a command such as:

```bash
git clone https://github.com/bbidulock/xde-desktop.git
```

Please see the [NEWS](NEWS) file for release notes and history of user visible
changes for the current version, and the [ChangeLog](ChangeLog) file for a more
detailed history of implementation changes.  The [TODO](TODO) file lists
features not yet implemented and other outstanding items.

Please see the [INSTALL](INSTALL) file for installation instructions.

When working from `git(1)', please see the [README-git](README-git) file.  An
abbreviated installation procedure that works for most applications
appears below.

This release is published under the GPL license that can be found in
the file [COPYING](COPYING).

### Quick Start:

The quickest and easiest way to get xde-desktop up and running is to run
the following commands:

```bash
git clone https://github.com/bbidulock/xde-desktop.git xde-desktop
cd xde-desktop
./autogen.sh
./configure --prefix=/usr
make V=0
make DESTDIR="$pkgdir" install
```

This will configure, compile and install xde-desktop the quickest.  For
those who would like to spend the extra 15 seconds reading `./configure
--help`, some compile time options can be turned on and off before the
build.

For general information on GNU's `./configure`, see the file [INSTALL](INSTALL).

### Running xde-desktop

Read the manual pages after installation:

    man xde-desktop

### Issues

Report issues to https://github.com/bbidulock/xde-desktop/issues.

