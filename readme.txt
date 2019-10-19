Bone Graft version 0.01
=======================

Bone Graft is a NetHack bones parser. As the current version number indicates,
it is not even close to being finished. Expect bugs, crashes, and other
frustrating behaviour.

Bone Graft is a derivative work of NetHack and falls under the NetHack's
copyright license. See the file license.txt for details.

Bone Graft should be able to parse any bones file produced by any unpatched
version of NetHack since 3.2.0. Patched versions which retain bones
compatibility with vanilla NetHack should also work. Of course, this is all
"in theory." In practice, don't expect very much from old bones files.

Bone Graft is a command line program. Don't try to run it by double clicking.
Bone Graft requires several command line arguments to describe the layout of
bones files. These arguments depend on the NetHack version, platform, and
compile time options. The following have worked for me:

Windows: graft -yu4
Unix/Linux/DOS: -yf4
Linux on RISC: -ybmf4

At various times during the development, -is1a2 worked for OS/2 and real mode
DOS, but those platforms seem problematic right now. There is a companion
program called Guess which will try to guess the proper command line arguments
for a given bones file, but it doesn't work very well. Did I mention that this
is a work in progress?

Up to this point my focus has been on reading the bones files, not on doing
anything with the parsed data. Right now the only output is an interpretation
of the magic numbers and a map of the level. In later versions more
information about bones files will be available.

Please send feedback to mlehotay@gmail.com

Please also send me bones files. I need lots of bizarre combinations of
versions, platforms, and compile time options for testing. The only
requirement is that the bones must be compatible with some unpatched version
of NetHack (not necessarily the official version).

--
CVS $Id: readme.txt,v 1.1 2005/08/16 21:36:46 Michael Exp $
