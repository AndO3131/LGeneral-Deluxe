#! /bin/sh

echo "*** Running aclocal ***"
aclocal
echo "*** Running autoheader ***"
autoheader
echo "*** Running automake ***"
automake --add-missing
echo "*** Running autoconf ***"
autoconf
echo
echo "Don't forget to run ./configure if you haven't done so for a while"
