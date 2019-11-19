
.PHONY: auto
auto:
	make -C common
	make -C client
	make -C relais
	make -C noeud
