DESCRIPTION = "The Network Time Protocol (NTP) is used to \
synchronize the time of a computer client or server to \
another server or reference time source, such as a radio \
or satellite receiver or modem."
HOMEPAGE = "http://ntp.isc.org/bin/view/Main/WebHome"
SECTION = "console/network"
PRIORITY = "optional"
LICENSE = "ntp"
PR = "r10"

SRC_URI = "http://www.eecis.udel.edu/~ntp/ntp_spool/ntp4/ntp-4.2/${P}.tar.gz \
	file://ntpdc.Makefile.am.maybe-layout.patch;patch=1 \
	file://ipv6only-workaround.patch;patch=1 \
        file://gcc4.patch;patch=1 \
	file://ntpd \
	file://ntp.conf \
	file://ntpdate"


INITSCRIPT_PACKAGES = "ntp ntpdate"

INITSCRIPT_NAME_ntp = "ntpd"
INITSCRIPT_PARAMS_ntp = "defaults 21"

# ntpdate should be started before ntpd
INITSCRIPT_NAME_ntpdate = "ntpdate"
INITSCRIPT_PARAMS_ntpdate = "defaults 20"

inherit autotools update-rc.d

# The ac_cv_header_readline_history is to stop ntpdc depending on either
# readline or curses
EXTRA_OECONF = "--without-openssl --without-crypto ac_cv_header_readline_history_h=no"
CFLAGS_append = " -DPTYS_ARE_GETPT -DPTYS_ARE_SEARCHED"

PACKAGES += "ntpdate ntp-bin ntp-tickadj"
# NOTE: you don't need ntpdate, use "ntpdc -q -g -x"
PROVIDES = "ntpdate-${PV} ntpdate-${PV}-${PR} ntpdate"

# This should use rc.update
FILES_ntpdate = "${bindir}/ntpdate ${sysconfdir}/init.d/ntpdate"
#This is too painful - perl is only needed for ntp-wait and ntptrace, which are
#perl scripts, and installing perl is an enormous overhead for a user who only
#needs ntpq
#RDEPENDS_ntp-bin = perl
# ntp originally includes tickadj. It's split off for inclusion in small firmware images on platforms 
# with wonky clocks (e.g. OpenSlug)
RDEPENDS_${PN} = ${PN}-tickadj
FILES_${PN}-bin = "${bindir}/ntp-wait ${bindir}/ntpdc ${bindir}/ntpq ${bindir}/ntptime ${bindir}/ntptrace"
FILES_${PN} = "${bindir}/ntpd ${sysconfdir}/ntp.conf ${sysconfdir}/init.d/ntpd"
FILES_${PN}-tickadj = "${bindir}/tickadj"

do_install_append() {
	install -d ${D}/${sysconfdir}/init.d
	install -m 644 ${WORKDIR}/ntp.conf ${D}/${sysconfdir}
	install -m 755 ${WORKDIR}/ntpdate ${D}/${sysconfdir}/init.d
	install -m 755 ${WORKDIR}/ntpd ${D}/${sysconfdir}/init.d
}
