#!/usr/bin/env sh

make -j8
sudo setcap cap_net_raw,cap_net_admin+eip deskfit
