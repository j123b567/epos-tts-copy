libtoolize --install --copy --force --automake
aclocal
autoconf --force
autoheader
automake --add-missing --copy --foreign --force-missing
./configure --enable-charsets --disable-portaudio YACC=byacc
make

# cleanup
# make maintainer-clean
