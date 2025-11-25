#
# Makefile
#
ULI78HOME = ../..
ULI783RD = $(ULI78HOME)/vendor
MRUBYDIR = $(ULI783RD)/mruby
CIRCLESTDLIB = $(ULI783RD)/circle-stdlib
CIRCLEHOME = $(CIRCLESTDLIB)/libs/circle
NEWLIBDIR = $(CIRCLESTDLIB)/install/arm-none-circle
ULI78LIB = $(ULI78HOME)/build/lib
ULI78SRC = $(ULI78HOME)/src
ULI78BUILD = $(ULI78HOME)/build
ULI78STUDIOSRC = $(ULI78SRC)/studio
BAREMETALSYSTEM = $(ULI78SRC)/system/baremetalpi

OBJS    = \
    $(BAREMETALSYSTEM)/main.o $(BAREMETALSYSTEM)/kernel.o $(BAREMETALSYSTEM)/customchargenerator.o \
    $(BAREMETALSYSTEM)/customscreen.o $(BAREMETALSYSTEM)/utils.o $(BAREMETALSYSTEM)/gamepads.o

include $(CIRCLEHOME)/Rules.mk

CFLAGS += -I "$(NEWLIBDIR)/include" -I $(STDDEF_INCPATH) -I $(CIRCLESTDLIB)/include -I $(ULI78HOME)/include -I $(ULI78SRC) \
      -I $(ULI78STUDIOSRC) -I $(ULI78BUILD) -D BAREMETALPI

LIBS :=  \
    $(ULI78LIB)/libuli78studio.a \
    $(ULI78LIB)/libuli78core.a \
    $(ULI78LIB)/libgiflib.a \
    $(ULI78LIB)/liblpeg.a \
    $(ULI78LIB)/libluaapi.a \
    $(ULI78LIB)/liblua.a \
    $(ULI78LIB)/libmoon.a \
    $(ULI78LIB)/libyuescript.a \
    $(ULI78LIB)/libfennel.a \
    $(ULI78LIB)/libzip.a \
    $(ULI78LIB)/libblipbuf.a \
    $(ULI78LIB)/libsquirrel.a \
    $(ULI78LIB)/libpython.a \
    $(ULI78LIB)/libpocketpy.a \
    $(ULI78LIB)/libscheme.a \
    $(ULI78LIB)/libwren.a \
    $(ULI78LIB)/libwasm.a \
    $(ULI78LIB)/libjanet.a \
    $(ULI78LIB)/libargparse.a \
    $(ULI78LIB)/libwave_writer.a \
    $(ULI78LIB)/libjs.a \
    $(ULI78LIB)/libquickjs.a \
    $(ULI78LIB)/libzlib.a \
    $(ULI78LIB)/libpng.a \
    $(ULI78LIB)/libruby.a \
    $(MRUBYDIR)/build/target/lib/libmruby.a \
    $(NEWLIBDIR)/lib/libc.a \
    $(NEWLIBDIR)/lib/libcirclenewlib.a \
    $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
    $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
    $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
    $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
    $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
    $(CIRCLEHOME)/lib/sound/libsound.a \
    $(CIRCLEHOME)/lib/usb/libusb.a \
    $(CIRCLEHOME)/lib/input/libinput.a \
    $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
    $(CIRCLEHOME)/lib/fs/libfs.a \
    $(CIRCLEHOME)/lib/net/libnet.a \
    $(CIRCLEHOME)/lib/sched/libsched.a \
    $(CIRCLEHOME)/lib/libcircle.a \
    $(NEWLIBDIR)/lib/libm.a
