#/bin/sh

svnversion=`LC_ALL=C svn info | awk '/^Revision:/ {print $2}'`
svndate=`LC_ALL=C svn info | awk '/^Last Changed Date:/ {print $4,$5}'`
gendate=`LC_ALL=C date`
cat <<EOF > version.h

// Do not edit!
//
// generated from $0 at $gendate

#define DENGON_SVNVERSION "$svnversion"
#define DENGON_SVNDATE "$svndate"

EOF
