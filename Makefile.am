SUBDIRS = intl util lgc-pg lged src tools po

EXTRA_DIST = AUTHORS COPYING ChangeLog README.lgeneral \
README.lgc-pg README.redit TODO \
lgeneral48.png lgeneral32.png lgeneral16.png lgeneral.desktop.in \
lgeneral-redit

desktopdir = $(datadir)/applications
desktop_DATA = lgeneral.desktop
icondir = $(datadir)/icons
icon_DATA = lgeneral48.png

AUTOMAKE_OPTIONS = foreign

# We can't use static desktop file because of icon location. Is this 
# the right way to handle it? At least it works... as long as $datadir
# does not contain plus signs. :-)
lgeneral.desktop:
	@sed -e "s+Icon=+Icon=$(datadir)/icons/lgeneral48.png+" \
	< lgeneral.desktop.in > lgeneral.desktop

package-translations:
	test x != "x$(LINGUAS)" || { echo "LINGUAS not set" ; exit 1 ; }
	cwd=`pwd` ; cd $(top_srcdir) ; \
	for l in $(LINGUAS) ; do \
	  tar -cjf $$cwd/$(PACKAGE).$${l}.`date '+%Y-%m-%d'`.tar.bz2 po/*/$$l.po ; \
	done
	
install-data-hook:
	@echo 
	@echo "================================================================"
	@echo 
	@echo "Congratulations, LGeneral has been successfully installed to"
	@echo 
	@echo "    "$(inst_dir)
	@echo 
	@echo "Now mount your Panzer General CD (e.g., to /mnt/cdrom) and run"
	@echo 
	@echo "    lgc-pg -s /mnt/cdrom/DAT"
	@echo 
	@echo "Alternatively you can download the PG data package from "
	@echo "http://lgames.sf.net, extract it (e.g., to /tmp/pg-data) and run"
	@echo 
	@echo "    lgc-pg -s /tmp/pg-data"
	@echo 
	@echo "For more information see README.lgc-pg."
	@echo 
	@echo "================================================================"
	@echo
