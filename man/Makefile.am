
xde_man1_files = \
	xde-desktop.man

nodist_man1_MANS = $(xde_man1_files)
CLEANFILES = $(xde_man1_files)

EXTRA_DIST = \
	xde-desktop.pod

$(xde_man1_files): %.man: %.pod
	$(AM_V_GEN)pod2man --center="User Commands" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="1" \
		$< >$@

#$(xde_man5_files): %.man: %.pod
#	$(AM_V_GEN)pod2man --center="Standards, Environments and Macros" \
#		--date="$(DATE)" \
#		--errors="none" \
#		--fixed="CW" \
#		--fixedbold="CB" \
#		--fixeditalic="CI" \
#		--fixedbolditalic="CX" \
#		--nourls \
#		--release="$(PACKAGE) $(VERSION)" \
#		--section="5" \
#		$< >$@

