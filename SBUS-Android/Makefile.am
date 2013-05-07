ACLOCAL_AMFLAGS = -I gl/m4
if USE_COMPAT
CPPFLAGS += -I../compat
endif
SUBDIRS = library wrapper gl tools utils idl #compat
if HAVE_JAVAC
SUBDIRS += java
endif
if HAVE_LATEX
SUBDIRS += docs
endif
