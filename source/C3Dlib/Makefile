#
# Makefile for C3Dlib
# © Julian Brown 1999
#

CC	= cc
LINK	= link
AS	= objasm
# CFLAGS	= -Wall -pedantic -ansi -O2 -DNO_UNALIGNED_XS -DUSE_ANSI_LIBS
CFLAGS	= -g -throwback -Otime -zc -D__riscos__ -IC: -IC3Dlib:
ASFLAGS = -g -throwback -CPU ARM7M -NoCache
# LDFLAGS	= -lC:o.clib
LIBS	= C:o.stubs C3Dlib:C3Dlib
LDFLAGS	= -debug
AR = libfile -c -o


OBJS =	o.c3d o.filing o.intmath o.list o.object o.transform o.vbuf \
	o.bucket o.fmath o.globals o.perspect o.texture o.texture24 \
	o.texture3 o.utils o.texture2

TESTOBJS = o.test

CSRC =	c.c3d c.filing c.intmath c.list c.object c.transform c.vbuf c.test \
	c.texture2 c.test2

all:	C3Dlib C3Dtest C3Dtest2

clean:
	wipe o.* ~V~CF
	wipe C3Dlib ~V~CF

depend:
	$(CC) $(CFLAGS) -M $(CSRC) > !Depend

C3Dlib:	$(OBJS)
	$(AR) C3Dlib $(OBJS)
	
C3Dtest: o.test C3Dlib
	$(LINK) $(LDFLAGS) $(LIBS) o.test -o C3Dtest

C3Dtest2: o.test2 C3Dlib
	$(LINK) $(LDFLAGS) $(LIBS) o.test2 -o C3Dtest2

o.%:	s.%
	$(AS) $(ASFLAGS) -from $< -to $@

include !Depend
