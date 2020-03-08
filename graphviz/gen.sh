#!/bin/bash
set -x
while [ 1 ] ; do \
	sleep 3;
	for i in ovs_kernel_full;\
		do
		for filetype in png ps; do
			dot -T$filetype $i.gv > /var/www/html/$i.$filetype
		done
	done
done
