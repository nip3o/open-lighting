libdir = $(plugindir)
EXTRA_DIST = BaseInflator.h CID.h \
             E131Header.h E131Inflator.h E131PDU.h HeaderSet.h \
             PDU.h PDUTest.h PDUTestCommon.h \
             RootHeader.h RootInflator.h RootPDU.h \
             UDPTransport.h # RootLayer.h  E131Core.h 

INCLUDES = -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolae131core.la
libolae131core_la_SOURCES = BaseInflator.cpp CID.cpp \
                            E131Inflator.cpp E131PDU.cpp PDU.cpp \
                            RootInflator.cpp RootPDU.cpp \
                            UDPTransport.cpp #E131Node.cpp RootLayer.h
libolae131core_la_LIBADD = @LIBS@

# Test programs
TESTS = E131Tester
check_PROGRAMS = $(TESTS)
E131Tester_SOURCES = E131Tester.cpp \
                     BaseInflatorTest.cpp \
                     CIDTest.cpp \
                     E131InflatorTest.cpp \
                     E131PDUTest.cpp \
                     HeaderSetTest.cpp \
                     PDUTest.cpp \
                     RootPDUTest.cpp \
                     RootInflatorTest.cpp \
                     UDPTransportTest.cpp
E131Tester_CXXFLAGS = $(CPPUNIT_CFLAGS)
E131Tester_LDFLAGS = $(CPPUNIT_LIBS)
E131Tester_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf \
                   ./libolae131core.la \
                   ../../../common/libolacommon.la