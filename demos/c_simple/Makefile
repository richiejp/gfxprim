TOPDIR=../..

CSOURCES=$(shell echo *.c)

INCLUDE=
LDLIBS+=-lGP -lGP_backends -lSDL -L$(TOPDIR)/build/

APPS=backend_example loaders_example loaders filters_symmetry gfx_koch\
     virtual_backend_example meta_data meta_data_dump

include $(TOPDIR)/pre.mk
include $(TOPDIR)/app.mk
include $(TOPDIR)/post.mk