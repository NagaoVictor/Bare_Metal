savedcmd_flying_fish_drv.mod := printf '%s\n'   flying_fish_drv.o | awk '!x[$$0]++ { print("./"$$0) }' > flying_fish_drv.mod
