SUMMARY="Image updater for Yocto projects"
DESCRIPTION = "Application for automatic software update"
SECTION="swupdate"
DEPENDS = "libconfig"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=0636e73ff0215e8d672dc4c32c317bb3"

inherit cml1 update-rc.d systemd pkgconfig

SRC_URI = "git://github.com/sbabic/swupdate.git;protocol=https \
     file://defconfig \
     file://swupdate \
     file://swupdate.service \
     file://swupdate-usb.rules \
     file://swupdate-usb@.service \
     file://swupdate-progress.service \
     "

PACKAGES =+ "${PN}-www ${PN}-lua ${PN}-usb ${PN}-tools"

INSANE_SKIP_${PN} = "ldflags"
FILES_${PN} = " \
    ${bindir}/swupdate \
    ${bindir}/swupdate-progress \
    ${systemd_unitdir}/swupdate-progress.service \
    "

INSANE_SKIP_${PN}-tools = "ldflags"
FILES_${PN}-tools = "${bindir}/swupdate-client \
                    ${bindir}/swupdate-hawkbitcfg \
                    ${bindir}/swupdate-sendtohawkbit"

FILES_${PN}-lua += "${libdir}/lua/"

FILES_${PN}-usb = " \
    /lib/systemd/system/swupdate-usb@.service \
    /etc/udev/rules.d/swupdate-usb.rules \
    "

FILES_${PN}-www = " \
    /lib/systemd/system/swupdate.service \
    /www/* \
    "

S = "${WORKDIR}/git/"

EXTRA_OEMAKE += " HOSTCC="${BUILD_CC}" HOSTCXX="${BUILD_CXX}" LD="${CC}" DESTDIR="${D}" V=1 ARCH=${TARGET_ARCH} CROSS_COMPILE=${TARGET_PREFIX} SKIP_STRIP=y"

DEPENDS += "kern-tools-native"
RDEPENDS_${PN}-usb += "${PN}-www"

# returns all the elements from the src uri that are .cfg files
def find_cfgs(d):
    return [s for s in src_patches(d, True) if s.endswith('.cfg')]

python () {
    try:
        defconfig = bb.fetch2.localpath('file://defconfig', d)
    except bb.fetch2.FetchError:
        return

    try:
        configfile = open(defconfig)
    except IOError:
        return

    features = configfile.readlines()
    configfile.close()

    if 'CONFIG_REMOTE_HANDLER=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' zeromq')

    if 'CONFIG_ENCRYPTED_IMAGES=y\n' in features or 'CONFIG_MONGOOSESSL=y\n' in features or 'CONFIG_HASH_VERIFY=y\n' in features or 'CONFIG_SURICATTA_SSL=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' openssl')

    if 'CONFIG_JSON=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' json-c')

    if 'CONFIG_SYSTEMD=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' systemd')

    if 'CONFIG_ARCHIVE=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' libarchive')

    if 'CONFIG_LUA=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' lua')

    if 'CONFIG_UBOOT=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' u-boot-fw-utils')

    if 'CONFIG_DOWNLOAD=y\n' in features or 'CONFIG_SURICATTA=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' curl')

    if 'CONFIG_MTD=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' mtd-utils')

    if 'CONFIG_CFI=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' mtd-utils')

    if 'CONFIG_UBIVOL=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' mtd-utils')

    if 'CONFIG_UCFWHANDLER=y\n' in features:
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' libgpiod')

    if 'CONFIG_MONGOOSE_WEB_API_V2=y\n' in features:
        d.setVar('SWUPDATE_WWW', 'webapp')
}

do_configure () {
  cp ${WORKDIR}/defconfig ${S}/.config
  merge_config.sh -m .config ${@" ".join(find_cfgs(d))}
  cml1_do_configure
}

do_compile() {
  unset LDFLAGS
  oe_runmake
}

do_install () {
  install -m 0755 -d ${D}/www
  if [ -d ${S}/web-app ];then
	cp -R --no-dereference --preserve=mode,links -v ${S}/examples/www/v2/* ${D}/www
  else
	install -m 0755 ${S}/www/* ${D}/www
  fi

  install -d ${D}${sysconfdir}/init.d
  install -m 755 ${WORKDIR}/swupdate ${D}${sysconfdir}/init.d

  install -d ${D}${systemd_unitdir}/system
  install -m 644 ${WORKDIR}/swupdate.service ${D}${systemd_unitdir}/system
  install -m 644 ${WORKDIR}/swupdate-usb@.service ${D}${systemd_unitdir}/system
  install -m 644 ${WORKDIR}/swupdate-progress.service ${D}${systemd_unitdir}/system


  if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
    install -d ${D}${sysconfdir}/udev/rules.d
    install -m 0644 ${WORKDIR}/swupdate-usb.rules ${D}${sysconfdir}/udev/rules.d/
  fi

  install -d ${D}${bindir}
  install -m 0755 swupdate_unstripped ${D}${bindir}/swupdate
  install -m 0755 tools/client ${D}${bindir}/swupdate-client
  install -m 0755 tools/progress ${D}${bindir}/swupdate-progress
  install -m 0755 tools/hawkbitcfg ${D}${bindir}/swupdate-hawkbitcfg
  install -m 0755 tools/sendtohawkbit ${D}${bindir}/swupdate-sendtohawkbit
}

INITSCRIPT_PACKAGES = "${PN} ${PN}-www"
INITSCRIPT_NAME_${PN} = "swupdate-progress"
INITSCRIPT_PARAMS_${PN} = "defaults 70"
INITSCRIPT_NAME_${PN}-www = "swupdate"
INITSCRIPT_PARAMS_${PN}-www = "defaults 70"

SYSTEMD_SERVICE_${PN} = "swupdate-progress.service"
SYSTEMD_SERVICE_${PN}-www = "swupdate.service"
SYSTEMD_SERVICE_${PN}-usb = "swupdate-usb@.service"
