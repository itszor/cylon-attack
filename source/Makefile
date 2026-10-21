#
# Makefile for Arc Cylon Attack
# © Julian Brown 1999
#

CC	= cc
LINK	= link
AS	= objasm
# CFLAGS	= -Wall -pedantic -ansi -O2 -DNO_UNALIGNED_XS -DUSE_ANSI_LIBS
# CFLAGS	= -g -throwback -Otime -zc -D__riscos__ -IC: -IC3Dlib:
CFLAGS	= -throwback -Otime -Ospace -IC: -IC3Dlib:
ASFLAGS = -throwback -CPU ARM7M -NoCache
# LDFLAGS	= -lC:o.clib
LIBS	= C:o.stubs C3Dlib:C3Dlib
LDFLAGS	= 
AR = libfile -c -o


OBJS =	o.smoothpoint o.smoothptinit o.test o.scene o.fmath o.game \
	o.list o.scale o.enemy o.rectangle o.levels o.console o.fsndcode \
	o.preemptglue o.title o.hiscore

CSRC =	c.smoothptinit c.test c.scene c.game c.list c.enemy c.levels \
	c.title c.hiscore

all:	!RunImage

clean:
	ifthere o.* then wipe o.* ~V~CF
	ifthere Cylon then wipe Cylon ~V~CF
	ifthere !RunImage then wipe !RunImage ~V~CF

depend:
	$(CC) $(CFLAGS) -M $(CSRC) > !Depend

!RunImage:	$(OBJS) C3Dlib:C3Dlib
	$(LINK) $(LDFLAGS) $(LIBS) $(OBJS) -o !RunImage

o.%:	s.%
	$(AS) $(ASFLAGS) -from $< -to $@

include !Depend
